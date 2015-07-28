#include "util.hh"

#include <deque>
#include <string.h>

#if defined(__GNUC__)
// unfortunately, even with gcc 4.9, the regex implementation is not complete,
// e.g. sub-expression references are not understood
// also, undefined-behavior sanitizer complaints about a invalid bool value
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

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;

namespace xxxml {

  namespace util {

    void remove(doc::Ptr &doc, const std::string &xpath)
    {
      // making sure that removed_nodes is destroyed after the
      // xpath::Object_Ptr, since those node are accessed during
      // the destruction of the xpath object
      deque<Node_Ptr> removed_nodes;
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(doc);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval(xpath, c);
        if (o.get()->type != XPATH_NODESET)
          throw std::runtime_error("xpath must produce a nodeset for removal");
        if (!o.get()->nodesetval)
          return;

        for (unsigned i = 0; i < unsigned(o.get()->nodesetval->nodeNr); ++i) {
          xmlNode *node = o.get()->nodesetval->nodeTab[i];
          removed_nodes.push_back(unlink_node(node));
        }
      }
    }

    void replace(doc::Ptr &doc, const std::string &xpath,
        const std::string &regex_str, const std::string &subst)
    {
      xpath::Context_Ptr c = xpath::new_context(doc);
      xpath::Object_Ptr o = xpath::eval(xpath, c);
      if (o.get()->type != XPATH_NODESET)
        throw std::runtime_error("xpath must produce a nodeset for replacement");
      if (!o.get()->nodesetval)
        return;

      const REGEX re(regex_str);

      for (unsigned i = 0; i < unsigned(o.get()->nodesetval->nodeNr); ++i) {
        xmlNode *node = o.get()->nodesetval->nodeTab[i];
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
      for (xmlNode *i = node->children; i;) {
        i = i->next;
        unlink_node(i);
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
      xpath::Context_Ptr c = xpath::new_context(doc);
      xpath::Object_Ptr o = xpath::eval(xpath, c);
      if (o.get()->type != XPATH_NODESET)
        throw std::runtime_error("xpath must produce a nodeset for replacement");
      if (!o.get()->nodesetval)
        return;
      for (unsigned i = 0; i < unsigned(o.get()->nodesetval->nodeNr); ++i) {
        xmlNode *node = o.get()->nodesetval->nodeTab[i];
        add(node, path, value, replace_value);
      }
    }

  }

}
