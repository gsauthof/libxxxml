#ifndef XXXML_UTIL_HH
#define XXXML_UTIL_HH

#include <xxxml/xxxml.hh>

#include <string>
#include <deque>

namespace xxxml {

  namespace util {

    class DF_Traverser {
      private:
        const xmlNode *node_ {nullptr};
        size_t height_ {0};
      public:
        DF_Traverser(const doc::Ptr &doc);
        const xmlNode *operator*() const;
        void advance();
        void skip_children();
        size_t height() const;
        bool eot() const;
    };

    bool has_root(const doc::Ptr &doc);
    std::deque<const xmlNode*> path(const xmlNode *node);
    std::deque<xmlNode*> path(xmlNode *node);

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

    Node_Ptr create_node(doc::Ptr &doc, const char *begin, const char *end);

    // position: 1 -> first child, -1 -> last child, 2 -> after node, -2 -> before node
    void insert(doc::Ptr &doc, xmlNode *node, xmlNode *new_node,
        int position);
    xmlNode *insert(doc::Ptr &doc, xmlNode *node,
        const char *begin, const char *end,
        int position);
    void insert(doc::Ptr &doc, const std::string &xpath,
        const char *begin, const char *end,
        int position);


    namespace xpath {

      std::string get_string(const doc::Ptr &doc, const std::string &expr);

    }

    std::pair<std::pair<const char*, const char*>, Output_Buffer_Ptr>
      dump(const doc::Ptr &doc, const xmlNode *node);
  }


}



#endif
