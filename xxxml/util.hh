#ifndef XXXML_UTIL_HH
#define XXXML_UTIL_HH

#include <string>
#include <xxxml/xxxml.hh>

// XXX add function to insert XML snippets (index to specify before/after/child ...)

namespace xxxml {

  namespace util {

    class Node_Set {
      private:
        xpath::Context_Ptr c_;
        xpath::Object_Ptr o_;
      public:
        Node_Set(doc::Ptr &doc, const std::string &xpath);

        using iterator = xmlNode**;

        iterator begin();
        iterator end();
    };

    void remove(doc::Ptr &doc, const std::string &xpath);
    void replace(doc::Ptr &doc, const std::string &xpath,
        const std::string &regex, const std::string &subst);

    xmlNode *optional_get_child(xmlNode *parent, const char *child_name);
    const xmlNode *optional_get_child(const xmlNode *parent, const char *child_name);
    void set_content(xmlNode *node, const std::string &value);

    void add(xmlNode *node,
        std::string path, const std::string &value, bool replace_value = true);
    void add(doc::Ptr &doc, const std::string &xpath,
        const std::string &path, const std::string &value,
        bool replace_value = true);

    void set_attribute(doc::Ptr &doc, const std::string &xpath,
        const std::string &name, const std::string &value);

  }


}



#endif
