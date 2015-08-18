#ifndef XXXML_XXXML_HH
#define XXXML_XXXML_HH

#include <string>
#include <stdexcept>
#include <memory>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xmlstring.h>
#include <libxml/xmlschemas.h>
#include <libxml/relaxng.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

/* ## General libxml2 Notes
 
   Generally, libxml2 uses camel case. The global 'namespace'
   prefix is `xml`.  Allocator functions are prefixed with `New`,
   deallocator ones with `Free` (after the namespace; with a few
   exceptions). Submodules like xpath extend the namespace prefix
   (e.g. `xmlXPath`).
  
   Examples: `xmlNewChild()` or `xmlFreeDoc()`
  
   For each struct there is a typedef for its pointer type, e.g.
   
       typedef xmlNode* xmlNodePtr;
  
   The main node type of libxml2 is `xmlNode`.
   If a node is reachable from a document/other node that is freed
   it is recursively freed as well.
   A node must be explicitly freed after it is (explicitly) unlinked
   from a document (cf. `xmlUnlinkNode()`).
  
   An `xmlNode` is tagged, e.g. it can be a text node, element node etc.
   (cf. xmlNode::type).
   
   To iterate over all element childs of a node
   (i.e. excluding e.g. text nodes):
  
       xmlNodePtr xmlFirstElementChild(xmlNodePtr parent)
       xmlNodePtr xmlPreviousElementSibling(xmlNodePtr node)
       xmlNodePtr xmlNextElementSibling(xmlNodePtr node)
       xmlNodePtr xmlLastElementChild(xmlNodePtr parent)
      
       unsigned long xmlChildElementCount(xmlNodePtr parent)
      
       xmlNodePtr xmlDocGetRootElement(const xmlDoc *doc)
  
   To iterate over all children nodes of a `xmlNode`
   (i.e. including e.g. text nodes):
  
       xmlNode::children
       xmlNode::children->next
       xmlNode::last
       xmlNode::{doc, parent, children, prev, next, name, ns, ...}
  
  
   global library init code:

       LIBXML_TEST_VERSION
       xmlInitParser();
  
   global cleanup code:

       xmlCleanupParser();
       xmlSchemaCleanupTypes();
      
   (e.g. in `main()`, before/after all threads are started/finished)


   2015, Georg Sauthoff <mail@georg.so>

*/

namespace xxxml {

  // {{{ Errors

  class Logic_Error : public std::logic_error {
    public:
      using std::logic_error::logic_error;
  };
  class Runtime_Error : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
  };
  class Parse_Error : public Runtime_Error {
    public:
      using Runtime_Error::Runtime_Error;
  };
  class Eval_Error : public Runtime_Error {
    public:
      using Runtime_Error::Runtime_Error;
  };
  class Validate_Error : public Runtime_Error {
    public:
      using Runtime_Error::Runtime_Error;
  };

  // }}}

  using Char_Ptr = std::unique_ptr<xmlChar, void (*)(void*)>;

  namespace doc {

    using Ptr = std::unique_ptr<xmlDoc, void (*)(xmlDoc*)>;

    xmlNode *get_root_element(const Ptr &doc);
    xmlNode *set_root_element(Ptr &doc, xmlNode *root);
    std::pair<Char_Ptr, size_t> dump_format_memory(Ptr &doc, int format = 1);

    unsigned format_dump(FILE *f, const Ptr &doc, bool format = true);

  }

  doc::Ptr new_doc();

  void elem_dump(FILE *f, const doc::Ptr &doc, const xmlNode *node);

  using Parser_Ctxt_Ptr
    = std::unique_ptr<xmlParserCtxt, void(*)(xmlParserCtxt*)>;

  Parser_Ctxt_Ptr new_parser_ctxt();

  // Unless the parser context is needed, use the read_*()
  // family of functions
  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const char *begin, const char *end,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const char *s,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const std::string &s,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr ctxt_read_file(Parser_Ctxt_Ptr &parser_context,
      const char * filename, 
      const char *encoding = nullptr,
      int options = 0);
  doc::Ptr ctxt_read_file(Parser_Ctxt_Ptr &parser_context,
      const std::string &filename, 
      const char *encoding = nullptr,
      int options = 0);

  doc::Ptr read_memory(
      const char *begin, const char *end,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr read_memory(
      const char *s,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr read_memory(
      const std::string &s,
      const char *URL = nullptr, const char *encoding = nullptr,
      int options = 0);
  doc::Ptr read_file(
      const char *filename,
      const char *encoding = nullptr,
      int options = 0);
  doc::Ptr read_file(
      const std::string &filename,
      const char *encoding = nullptr,
      int options = 0);

  // The `xmlParse*` family of functions are deliberately not
  // mapped because they don't support the use of `xmlDict`
  // dictionaries nor options. They even behave differently than
  // the other functions with options set to zero.
  // Use the `xmlCtxtRead*` and `xmlRead*` families instead.

  namespace dict {

    using Ptr = std::unique_ptr<xmlDict, void(*)(xmlDict*)>;

    // when assigning it to xmlDoc::dict, the pointer has to be released!
    // e.g. doc->dict = p.release()
    Ptr create();

    // check if the pointer value is from that dictionary
    bool owns(xmlDict *dict, const xmlChar *s);
    bool owns(Ptr &dict, const xmlChar *s);

    // check if the string content is already contained in the dictionary
    xmlChar *exists(xmlDict *dict, const char *s, unsigned length);
    xmlChar *exists(xmlDict *dict, const char *s);
    xmlChar *exists(xmlDict *dict, const std::string &s);
    xmlChar *exists(xmlDict *dict, const char *begin, const char *end);
    xmlChar *exists(Ptr &dict, const char *s, unsigned length);
    xmlChar *exists(Ptr &dict, const char *s);
    xmlChar *exists(Ptr &dict, const std::string &s);
    xmlChar *exists(Ptr &dict, const char *begin, const char *end);

    xmlChar *lookup(xmlDict *dict, const char *s, unsigned length);
    xmlChar *lookup(xmlDict *dict, const char *s);
    xmlChar *lookup(xmlDict *dict, const std::string &s);
    xmlChar *lookup(xmlDict *dict, const char *begin, const char *end);
    xmlChar *lookup(Ptr &dict, const char *s, unsigned length);
    xmlChar *lookup(Ptr &dict, const char *s);
    xmlChar *lookup(Ptr &dict, const std::string &s);
    xmlChar *lookup(Ptr &dict, const char *begin, const char *end);

    // XXX
    const char *q_lookup(Ptr &dict, const char *prefix, const char *name);
    const char *q_lookup(Ptr &dict,
        const std::string &prefix, const std::string &name);
  }


  const char *name(const xmlNode *node);
  const char *name(const xmlAttr *node);
  const char *content(const xmlNode *node);
  size_t child_element_count(const xmlNode *node);

  const xmlNode *first_element_child(const xmlNode *node);
  const xmlNode *last_element_child(const xmlNode *node);
  const xmlNode *previous_element_sibling(const xmlNode *node);
  const xmlNode *next_element_sibling(const xmlNode *node);

  xmlNode *new_doc_node(doc::Ptr &doc, const char *name);
  xmlNode *new_doc_node(doc::Ptr &doc, const std::string &name);
  xmlNode *new_doc_node(doc::Ptr &doc, const char *name, const char *content);
  xmlNode *new_doc_node(doc::Ptr &doc, const std::string &name,
      const std::string &content);
  // XXX add namespace overload
  xmlNode *new_doc_text(doc::Ptr &doc);
  xmlNode *new_doc_text(doc::Ptr &doc, const char *text);
  xmlNode *new_doc_text(doc::Ptr &doc, const std::string &text);

  xmlNode *new_child(xmlNode *parent, const char *name);
  xmlNode *new_child(xmlNode *parent, const std::string &name);
  xmlNode *new_child(xmlNode *parent, const char *name, const char *content);
  xmlNode *new_child(xmlNode *parent, const std::string &name,
      const std::string &content);
  // XXX add namespace overload

  xmlNode *add_child(xmlNode *parent, xmlNode *node);

  void node_add_content(xmlNode *node, const char *text, unsigned len);

  using Node_Ptr = std::unique_ptr<xmlNode, void(*)(xmlNode*)>;

  Node_Ptr unlink_node(xmlNode *node);

  xmlAttr *new_prop(xmlNode *node, const char *name, const char *value);
  xmlAttr *new_prop(xmlNode *node,
      const std::string &name, const std::string &value);
  // XXX add ns version

  const char *get_prop(const xmlNode *node, const char *name);

  using Output_Buffer_Ptr
    = std::unique_ptr<xmlOutputBuffer, int (*)(xmlOutputBuffer*)>;

  Output_Buffer_Ptr alloc_output_buffer(
      xmlCharEncodingHandler *encoder = nullptr);

  namespace xpath {

    using Context_Ptr
      = std::unique_ptr<xmlXPathContext, void (*)(xmlXPathContext*)>;

    using Comp_Expr_Ptr
      = std::unique_ptr<xmlXPathCompExpr, void (*)(xmlXPathCompExpr*)>;


    Comp_Expr_Ptr compile(const char *expr);
    Comp_Expr_Ptr compile(const std::string &expr);
    Comp_Expr_Ptr ctxt_compile(Context_Ptr &ctxt, const char *expr);
    Comp_Expr_Ptr ctxt_compile(Context_Ptr &ctxt, const std::string &expr);

    using Object_Ptr
      = std::unique_ptr<xmlXPathObject, void (*)(xmlXPathObject*)>;

    Object_Ptr new_cstring(const char *value);
    Object_Ptr new_cstring(const std::string &value);

    Context_Ptr new_context(doc::Ptr &doc);

    void register_ns(Context_Ptr &, const std::string &prefix, const std::string &ns);
    void register_ns(Context_Ptr &, const char *prefix, const char *ns);
    template <typename Container>
      void register_ns(Context_Ptr &context, const Container &c)
      {
        for (auto &p : c)
          register_ns(context, p.first, p.second);
      }

    void register_variable(Context_Ptr &, const char *name, Object_Ptr value);
    void register_variable(Context_Ptr &c, const char *name, const char *value);
    void register_variable(Context_Ptr &c, const char *name,
        const std::string &value);

    Object_Ptr eval(const std::string &expr, Context_Ptr &context);
    Object_Ptr eval(const char *expr, Context_Ptr &context);

    // context node is changed after the call!, thus, the context
    // should not be re-used for eval()
    //
    // expr must also be relative, e.g. start with a './'
    // otherwise the expression also matches above the subtree's root
    // (cf. http://stackoverflow.com/questions/27627357/how-to-restrict-an-xpath-via-xmlxpathnodeeval-to-a-subtree)
    Object_Ptr node_eval(const char *expr, const xmlNode *node,
        Context_Ptr &context);
    Object_Ptr node_eval(const std::string &expr, const xmlNode *node,
        Context_Ptr &context);

    Object_Ptr compiled_eval(Comp_Expr_Ptr &expr, Context_Ptr &context);
  }

  namespace schema {

    using Parser_Ctxt_Ptr
      = std::unique_ptr<xmlSchemaParserCtxt, void (*)(xmlSchemaParserCtxt*)>;
    using Ptr
      = std::unique_ptr<xmlSchema, void (*)(xmlSchema*)>;
    using Valid_Ctxt_Ptr
      = std::unique_ptr<xmlSchemaValidCtxt , void (*)(xmlSchemaValidCtxt*)>;

    Parser_Ctxt_Ptr new_parser_ctxt(const char *filename);
    Parser_Ctxt_Ptr new_parser_ctxt(const std::string &filename);

    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *begin, const char *end);
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *s);
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const std::string &s);

    Ptr parse(Parser_Ctxt_Ptr &parser_context);
    Valid_Ctxt_Ptr new_valid_ctxt(Ptr &schema);

    void validate_doc(Valid_Ctxt_Ptr &valid_ctxt, const doc::Ptr &doc);

    void set_valid_structured_errors(Valid_Ctxt_Ptr &v, xmlStructuredErrorFunc f,
        void *user_ptr);
  }

  namespace relaxng {

    using Parser_Ctxt_Ptr
      = std::unique_ptr<xmlRelaxNGParserCtxt, void (*)(xmlRelaxNGParserCtxt*)>;

    Parser_Ctxt_Ptr new_parser_ctxt(const char *url);
    Parser_Ctxt_Ptr new_parser_ctxt(const std::string &filename);

    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *begin, const char *end);
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *s);
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const std::string &s);

    using Ptr = std::unique_ptr<xmlRelaxNG, void(*)(xmlRelaxNG*)>;

    Ptr parse(Parser_Ctxt_Ptr &parser_context);

    using Valid_Ctxt_Ptr
      = std::unique_ptr<xmlRelaxNGValidCtxt, void(*)(xmlRelaxNGValidCtxt*)>;

    Valid_Ctxt_Ptr new_valid_ctxt(Ptr &schema);


    void validate_doc(Valid_Ctxt_Ptr &context, doc::Ptr &doc);

    // return 0: call validate_full_element
    // return 1: push ok
    int validate_push_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem);
    void validate_full_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem);
    void validate_pop_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem);
    void validate_push_cdata(Valid_Ctxt_Ptr &context,
					 const char *begin, const char *end);


    void set_valid_errors(Valid_Ctxt_Ptr &context,
        xmlRelaxNGValidityErrorFunc err,
        xmlRelaxNGValidityWarningFunc warn, 
        void *user_data);

    void set_valid_structured_errors(Valid_Ctxt_Ptr &context, 
        xmlStructuredErrorFunc serror, 
        void *user_data);

  }

  namespace text_reader {

    using Ptr = std::unique_ptr<xmlTextReader, void(*)(xmlTextReader*)>;

    Ptr for_memory(const char *begin, const char *end,
        const char *url = nullptr,
        const char *encoding = nullptr, int options = 0);
    Ptr for_memory(const char *s,
        const char *url = nullptr,
        const char *encoding = nullptr, int options = 0);
    Ptr for_memory(const std::string &s,
        const char *url = nullptr,
        const char *encoding = nullptr, int options = 0);
    Ptr for_file(const char *filename, const char *encoding = nullptr,
        int options = 0);
    Ptr for_file(const std::string &filename, const char *encoding = nullptr,
        int options = 0);

    bool read(Ptr &reader);
    bool read_attribute_value(Ptr &reader);
    bool move_to_first_attribute(Ptr &reader);
    bool move_to_next_attribute(Ptr &reader);

    const char *const_value(Ptr &reader);
    const char *const_local_name(Ptr &reader);
    const char *const_name(Ptr &reader);
    const char *const_prefix(Ptr &reader);
    // returns nullptr if not available
    const char *const_namespace_uri(Ptr &reader);
    int node_type(Ptr &reader);
    bool has_attributes(Ptr &reader);
    bool has_value(Ptr &reader);
    bool is_empty_element(Ptr &reader);
    bool is_valid(Ptr &reader);
    unsigned depth(Ptr &reader);

    void set_parser_prop(Ptr &reader, int prop, int value);

    void relaxng_validate(Ptr &reader);
    void relaxng_validate(Ptr &reader, const char *filename);
    void relaxng_validate(Ptr &reader, const std::string &filename);
    void relaxng_validate_ctxt(Ptr &reader);
    void relaxng_validate_ctxt(Ptr &reader, relaxng::Valid_Ctxt_Ptr &context);
    void relaxng_set_schema(Ptr &reader);
    void relaxng_set_schema(Ptr &reader, relaxng::Ptr &schema);

    // XXX add xsd methods

  }

  namespace text_writer {

    using Ptr = std::unique_ptr<xmlTextWriter, void(*)(xmlTextWriter*)>;

    void set_indent(Ptr &writer, bool b);
    void set_quote_char(Ptr &writer, char c);

    void start_document(Ptr &writer);
    void start_document(Ptr &writer, const char *version);
    void start_document(Ptr &writer, const char *version, const char *encoding);
    void start_document(Ptr &writer, const char *version, const char *encoding,
        bool standalone);
    void end_document(Ptr &writer);

    void start_element(Ptr &writer, const char *name);
    void end_element(Ptr &writer);

    void start_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *namespace_uri = nullptr);
    //void end_element(Ptr &writer);

    void start_attribute(Ptr &writer, const char *name);
    void end_attribute(Ptr &writer);

    void write_string(Ptr &writer, const char *content);
    void write_raw(Ptr &writer, const char *begin, const char *end);

    void write_comment(Ptr &writer, const char *comment);

    void write_element(Ptr &writer, const char *name, const char *content);

    void write_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *namespace_uri, const char *content);
    void write_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *content);

    void write_attribute(Ptr &writer, const char *name, const char *content);

    void flush(Ptr &writer);

  }

  text_writer::Ptr new_text_writer(Output_Buffer_Ptr o);
  text_writer::Ptr new_text_writer_filename(const char *filename,
      bool compression = false);
  text_writer::Ptr new_text_writer_filename(const std::string &s,
      bool compression = false);


}


#endif
