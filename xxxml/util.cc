#include "util.hh"

#include <deque>
#include <string.h>

#if defined(__GNUC__)
// unfortunately, even with gcc 4.9, the regex implementation is not complete,
// e.g. sub-expression references are not understood
// also, undefined-behavior sanitizer complaints about a invalid bool value
// GCC 5.3.1: similar story
//
//#if defined(__GNUC__) && (__GNUC__ <= 4) && (__GNUC_MINOR__ < 9)
// use this one with gcc < 4.9
// cf. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53631
//     http://stackoverflow.com/questions/12530406/is-gcc-4-7-and-gcc-4-8-buggy-about-regular-expressions
#include <boost/regex.hpp>
#define REGEX boost::regex
#define REGEX_REPLACE boost::regex_replace
#else
#include <regex>
#define REGEX std::regex
#define REGEX_REPLACE std::regex_replace
#endif

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;

namespace xxxml {

  namespace util {

    DF_Traverser::DF_Traverser(const doc::Ptr &doc)
      : node_(doc::get_root_element(doc))
    {
    }
    const xmlNode *DF_Traverser::operator*() const
    {
      if (eot())
        throw logic_error("DF_Traverser *: stack is empty");

      return node_;
    }
    void DF_Traverser::advance()
    {
      if (eot())
        throw logic_error("DF_Traverser advance: stack is empty");

      auto child = first_element_child(node_);
      if (child) {
        node_ = child;
        ++height_;
      } else {
        while (node_) {
          auto next = next_element_sibling(node_);
          if (next) {
            node_ = next;
            break;
          }
          node_ = node_->parent;
          --height_;
        }
      }
    }
    void DF_Traverser::skip_children()
    {
      if (eot())
        throw logic_error("DF_Traverser skip: stack is empty");

      auto next = next_element_sibling(node_);
      if (next)
        node_ = next;
      else {
        node_ = node_->parent;
        --height_;
        advance();
      }
    }
    size_t DF_Traverser::height() const
    {
      if (eot())
        throw logic_error("DF_Traverser skip: stack is empty");

      return height_;
    }
    bool DF_Traverser::eot() const
    {
      return !node_;
    }


    Node_Set::Node_Set(doc::Ptr &doc, const std::string &xpath)
      :
        c_(xxxml::xpath::new_context(doc)),
        o_(xxxml::xpath::eval(xpath, c_))
    {
      if (o_.get()->type != XPATH_NODESET)
        throw std::runtime_error("xpath must produce a nodeset");
    }
    Node_Set::iterator Node_Set::begin()
    {
      if (!o_.get()->nodesetval)
        return nullptr;
      return o_.get()->nodesetval->nodeTab;
    }
    Node_Set::iterator Node_Set::end()
    {
      if (!o_.get()->nodesetval)
        return nullptr;
      return o_.get()->nodesetval->nodeTab + o_.get()->nodesetval->nodeNr;
    }

    void remove(doc::Ptr &doc, const std::string &xpath)
    {
      // making sure that removed_nodes is destroyed after the
      // xpath::Object_Ptr, since those node are accessed during
      // the destruction of the xpath object
      deque<Node_Ptr> removed_nodes;
      {
        Node_Set node_set(doc, xpath);
        for (auto node : node_set)
          removed_nodes.push_back(unlink_node(node));
      }
    }

    void replace(doc::Ptr &doc, const std::string &xpath,
        const std::string &regex_str, const std::string &subst)
    {
      const REGEX re(regex_str);

      Node_Set node_set(doc, xpath);
      for (auto node : node_set) {
        if (first_element_child(node))
          continue; // ignore constructed tags
        string s;
        {
          xmlNode *child = node->children;
          if (child  && child->type == XML_TEXT_NODE) {
            s = content(child);
            unlink_node(child);
          }
        }
        string t(REGEX_REPLACE(s, re, subst));
        node_add_content(node, t.data(), t.size());
      }
    }
    xmlNode *optional_get_child(xmlNode *parent, const char *child_name)
    {
      for (xmlNode *i = first_element_child(parent); i; i = next_element_sibling(i))
        if (!strcmp(name(i), child_name))
          return i;
      return nullptr;
    }
    const xmlNode *optional_get_child(const xmlNode *parent, const char *child_name)
    {
      return optional_get_child(const_cast<xmlNode*>(parent), child_name);
    }
    void set_content(xmlNode *node, const std::string &value)
    {
      for (xmlNode *i = node->children; i; ) {
        xmlNode *t = i;
        i = i->next;
        unlink_node(t);
      }
      node_add_content(node, value);
    }
    void add_content(xmlNode *node, const std::string &value, bool replace)
    {
      if (replace)
        set_content(node, value);
      else
        node_add_content(node, value);
    }
    void add(xmlNode *node,
        std::string path, const std::string &value, bool replace_value)
    {
      if (path.empty() || path == ".") {
        add_content(node, value, replace_value);
      } else {
        xmlNode *x = node;
        using si = boost::algorithm::split_iterator<char*>;
        pair<char*, char*> inp(&*path.begin(), &*path.begin() + path.size());
        char q_s[1] = {'/'};
        pair<char*, char*> q(q_s, q_s + 1);
        for (si i = boost::algorithm::make_split_iterator(inp,
              boost::algorithm::first_finder(q, boost::algorithm::is_equal()));
            i != si(); ++i) {
          if (boost::empty(*i))
            continue;
          char *e = (*i).end();
          *e = '\0';
          if (!strcmp((*i).begin(), "+"))
            continue;
          bool append_child = false;
          const char *s  = (*i).begin();
          if ((*i).front() == '+') {
            ++s;
            append_child = true;
          }
          if (append_child) {
            x = new_child(x, s);
          } else {
            xmlNode *a = optional_get_child(x, s);
            if (a) {
              x = a;
            } else {
              x = new_child(x, s);
            }
          }
        }
        add_content(x, value, replace_value);
      }
    }
    void add(doc::Ptr &doc, const std::string &xpath,
        const std::string &path, const std::string &value, bool replace_value)
    {
      Node_Set node_set(doc, xpath);
      for (auto node : node_set)
        add(node, path, value, replace_value);
    }

    void set_attribute(doc::Ptr &doc, const std::string &xpath,
        const std::string &name, const std::string &value)
    {
      Node_Set node_set(doc, xpath);
      for (auto node : node_set)
        set_prop(node, name, value);
    }

    void insert(doc::Ptr &doc, xmlNode *node, xmlNode *new_node,
        int position)
    {
      if (!node) {
        doc::set_root_element(doc, new_node);
        return;
      }

      enum { FIRST_CHILD = 1, LAST_CHILD = -1, BEFORE_NODE = -2, AFTER_NODE = 2};
      switch (position) {
        case FIRST_CHILD:
          // XXX add variants of add_child, add_next_sibling?
          // i.e. ones that take a Node_Ptr
          {
            xmlNode *first = first_element_child(node);
            if (first) {
              add_prev_sibling(first, new_node);
            } else {
              add_child(node, new_node);
            }
          }
          break;
        case LAST_CHILD:
          add_child(node, new_node);
          break;
        case BEFORE_NODE:
          add_prev_sibling(node, new_node);
          break;
        case AFTER_NODE:
          add_next_sibling(node, new_node);
          break;
        default:
          throw runtime_error("Unknown position: "
              + boost::lexical_cast<string>(position));
      }
    }

    Node_Ptr create_node(doc::Ptr &doc, const char *begin, const char *end)
    {
      doc::Ptr temp_doc = read_memory(begin, end, nullptr, nullptr);
      xmlNode *subtree_root = doc::get_root_element(temp_doc);
      if (!subtree_root)
        throw runtime_error("new document has no root");
      Node_Ptr x = doc::copy_node(subtree_root, doc, 1);
      return x;
    }

    xmlNode *insert(doc::Ptr &doc, xmlNode *node,
        const char *begin, const char *end,
        int position)
    {
      Node_Ptr x = create_node(doc, begin, end);
      insert(doc, node, x.get(), position);
      return x.release();
    }

    void insert(doc::Ptr &doc, const std::string &xpath,
        const char *begin, const char *end,
        int position)
    {
      Node_Set node_set(doc, xpath);
      for (auto node : node_set)
        insert(doc, node, begin, end, position);
    }

    namespace xpath {

      std::string get_string(const doc::Ptr &doc, const std::string &expr)
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(doc);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval(expr, c);
        switch (o.get()->type) {
          case XPATH_NODESET:
            {
              if (!o.get()->nodesetval)
                throw logic_error("nodesetval is null");
              if (!o.get()->nodesetval->nodeNr)
                throw underflow_error("xpath nodeset is empty");
              auto x = xxxml::xpath::cast_node_set_to_string(o);
              return string(x.release());
            }
            break;
          case XPATH_STRING:
            {
              return string(reinterpret_cast<const char*>(o.get()->stringval));
            }
            break;
          default:
            break;
        }
        throw Logic_Error("xpath result type not implemented yet");
        return "";
      }

    }

    bool has_root(const doc::Ptr &doc)
    {
      auto r = xmlDocGetRootElement(doc.get());
      return r;
    }

    std::pair<std::pair<const char*, const char*>, Output_Buffer_Ptr>
      dump(const doc::Ptr &doc, const xmlNode *node)
      {
        xxxml::Output_Buffer_Ptr b = alloc_output_buffer();
        xxxml::node_dump_output(b, doc, node);
        return make_pair(
            make_pair(
              reinterpret_cast<const char*>(xmlBufContent(b.get()->buffer)),
              reinterpret_cast<const char*>(xmlBufEnd(b.get()->buffer))
              ),
            std::move(b));
      }

  } // util


} // xxxml
