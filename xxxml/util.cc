#include "util.hh"

#include <deque>

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
        node_add_content(node, t.c_str(), t.size());
      }
    }

  }

}
