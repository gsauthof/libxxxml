#include "xxxml.hh"

#include <string.h>
#include <sstream>

#include <libxml/xpathInternals.h>


using namespace std;

namespace xxxml {

  namespace xpath {

    Object_Ptr new_cstring(const char *value)
    {
      Object_Ptr r(xmlXPathNewCString(value), xmlXPathFreeObject);
      if (!r)
        throw Logic_Error("could not allocate xpath string object");
      return std::move(r);
    }
    Object_Ptr new_cstring(const std::string &value)
    {
      return new_cstring(value.c_str());
    }

    Comp_Expr_Ptr compile(const char *expr)
    {
      Comp_Expr_Ptr r(xmlXPathCompile(
            reinterpret_cast<const xmlChar*>(expr)
            ), xmlXPathFreeCompExpr);
      if (!r)
        throw Runtime_Error("could not compile xpath expr: "
            + string(expr));
      return std::move(r);
    }
    Comp_Expr_Ptr compile(const std::string &expr)
    {
      return compile(expr.c_str());
    }

    Comp_Expr_Ptr ctxt_compile(Context_Ptr &ctxt, const char *expr)
    {
      Comp_Expr_Ptr r(xmlXPathCtxtCompile(
            ctxt.get(),
            reinterpret_cast<const xmlChar*>(expr)
            ), xmlXPathFreeCompExpr);
      if (!r)
        throw Runtime_Error("could not context compile xpath expr: "
            + string(expr));
      return std::move(r);
    }
    Comp_Expr_Ptr ctxt_compile(Context_Ptr &ctxt, const std::string &expr)
    {
      return ctxt_compile(ctxt, expr.c_str());
    }

    Context_Ptr new_context(doc::Ptr &doc)
    {
      Context_Ptr r(xmlXPathNewContext(doc.get()), xmlXPathFreeContext);
      if (!r)
        throw Runtime_Error("Could not create xpath context");
      return std::move(r);
    }

    void register_ns(Context_Ptr &context, const char *prefix, const char *ns)
    {
      int r = xmlXPathRegisterNs(context.get(),
          reinterpret_cast<const xmlChar*>(prefix),
          reinterpret_cast<const xmlChar*>(ns) );
      if (r == -1)
        throw Runtime_Error("Could not register namespace " + string(ns)
            + " with prefix " + string(prefix));
    }
    void register_ns(Context_Ptr &context, const std::string &prefix, const std::string &ns)
    {
      register_ns(context, prefix, ns);
    }

    void register_variable(Context_Ptr &c, const char *name, Object_Ptr value)
    {
      // Note that libxml takes ownership of the object pointer
      // as part of the registration, thus, we have to release it
      int r = xmlXPathRegisterVariable(c.get(),
          reinterpret_cast<const xmlChar*>(name),
          value.release());
      if (r == -1)
        throw Runtime_Error("could not register variable: " + string(name));
    }
    void register_variable(Context_Ptr &c, const char *name, const char *value)
    {
      register_variable(c, name, new_cstring(value));
    }
    void register_variable(Context_Ptr &c, const char *name,
        const std::string &value)
    {
      register_variable(c, name, value.c_str());
    }

    Object_Ptr eval(const char *expr, Context_Ptr &context)
    {
      Object_Ptr r(xmlXPathEval(reinterpret_cast<const xmlChar*>(expr),
            context.get()),
          xmlXPathFreeObject);
      if (!r)
        throw Eval_Error("Could not evaluate xpath: " + string(expr));
      return std::move(r);
    }
    Object_Ptr eval(const std::string &expr, Context_Ptr &context)
    {
      return eval(expr.c_str(), context);
    }

    Object_Ptr node_eval(const char *expr, const xmlNode *node,
        Context_Ptr &context)
    {
      Object_Ptr r(xmlXPathNodeEval(const_cast<xmlNode*>(node),
            reinterpret_cast<const xmlChar*>(expr),
            context.get()),
          xmlXPathFreeObject);
      if (!r)
        throw Eval_Error("Could not evaluate xpath: " + string(expr));
      return std::move(r);
    }
    Object_Ptr node_eval(const std::string &expr, const xmlNode *node,
        Context_Ptr &context)
    {
      return node_eval(expr.c_str(), node, context);
    }
    Object_Ptr compiled_eval(Comp_Expr_Ptr &expr, Context_Ptr &context)
    {
      Object_Ptr r(xmlXPathCompiledEval(expr.get(), context.get()),
          xmlXPathFreeObject);
      if (!r)
        throw Eval_Error("Could not evaluate compiled xpath expression");
      return std::move(r);
    }

  }

  namespace schema {

    Parser_Ctxt_Ptr new_parser_ctxt(const char *filename)
    {
      Parser_Ctxt_Ptr r(xmlSchemaNewParserCtxt(filename),
          xmlSchemaFreeParserCtxt);
      if (!r)
        throw Parse_Error("could not create schema parser context for: "
            + string(filename));
      return std::move(r);
    }
    Parser_Ctxt_Ptr new_parser_ctxt(const std::string &filename)
    {
      return new_parser_ctxt(filename.c_str());
    }

    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *begin, const char *end)
    {
      Parser_Ctxt_Ptr r(xmlSchemaNewMemParserCtxt(begin, end-begin),
          xmlSchemaFreeParserCtxt);
      if (!r)
        throw Parse_Error("could not create schema parser context from memory");
      return std::move(r);
    }
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *s)
    {
      return new_mem_parser_ctxt(s, s + strlen(s));
    }
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const std::string &s)
    {
      return new_mem_parser_ctxt(s.data(), s.data() + s.size());
    }

    Ptr parse(Parser_Ctxt_Ptr &parser_context)
    {
      Ptr r(xmlSchemaParse(parser_context.get()), xmlSchemaFree);
      if (!r)
        throw Parse_Error("schema parse error");
      return std::move(r);
    }
    Valid_Ctxt_Ptr new_valid_ctxt(Ptr &schema)
    {
      Valid_Ctxt_Ptr r(xmlSchemaNewValidCtxt(schema.get()),
          xmlSchemaFreeValidCtxt);
      if (!r)
        throw Logic_Error("could not create schema validation context");
      return r;
    }

    void validate_doc(Valid_Ctxt_Ptr &valid_ctxt, const doc::Ptr &doc)
    {
      int r = xmlSchemaValidateDoc(valid_ctxt.get(), doc.get());
      if (r < 0)
        throw Logic_Error("internal validation error");
      if (r > 0)
        throw Validate_Error("XML document is invalid");
    }

    void set_valid_structured_errors(Valid_Ctxt_Ptr &v, xmlStructuredErrorFunc f,
        void *user_ptr)
    {
      xmlSchemaSetValidStructuredErrors(v.get(), f, user_ptr);
    }

  }

  namespace relaxng {

    Parser_Ctxt_Ptr new_parser_ctxt(const char *filename)
    {
      Parser_Ctxt_Ptr r(xmlRelaxNGNewParserCtxt(filename),
          xmlRelaxNGFreeParserCtxt);
      if (!r)
        throw Parse_Error("could not create relaxng parser context for: "
            + string(filename));
      return std::move(r);
    }
    Parser_Ctxt_Ptr new_parser_ctxt(const std::string &filename)
    {
      return new_parser_ctxt(filename.c_str());
    }

    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *begin, const char *end)
    {
      Parser_Ctxt_Ptr r(xmlRelaxNGNewMemParserCtxt(begin, end-begin),
          xmlRelaxNGFreeParserCtxt);
      if (!r)
        throw Parse_Error("could not create relaxng parser context from memory");
      return std::move(r);
    }
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const char *s)
    {
      return new_mem_parser_ctxt(s, s + strlen(s));
    }
    Parser_Ctxt_Ptr new_mem_parser_ctxt(const std::string &s)
    {
      return new_mem_parser_ctxt(s.data(), s.data() + s.size());
    }

    Ptr parse(Parser_Ctxt_Ptr &parser_context)
    {
      Ptr r(xmlRelaxNGParse(parser_context.get()), xmlRelaxNGFree);
      if (!r)
        throw Parse_Error("rng schema parse error");
      return std::move(r);
    }

    Valid_Ctxt_Ptr new_valid_ctxt(Ptr &schema)
    {
      Valid_Ctxt_Ptr r(xmlRelaxNGNewValidCtxt(schema.get()),
          xmlRelaxNGFreeValidCtxt);
      if (!r)
        throw Logic_Error("could not create schema validation context");
      return r;
    }

    void validate_doc(Valid_Ctxt_Ptr &valid_context, doc::Ptr &doc)
    {
      int r = xmlRelaxNGValidateDoc(valid_context.get(), doc.get());
      if (r < 0)
        throw Logic_Error("internal validation error");
      if (r > 0)
        throw Validate_Error("XML document is invalid");
    }

    int validate_push_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem)
    {
      int r = xmlRelaxNGValidatePushElement(context.get(),
          const_cast<xmlDoc*>(doc.get()),
          const_cast<xmlNode*>(elem));
      if (r == -1)
        throw Validate_Error("rng validate push element error");
      return r;
    }
    void validate_full_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem)
    {
      int r = xmlRelaxNGValidateFullElement(context.get(),
          const_cast<xmlDoc*>(doc.get()),
          const_cast<xmlNode*>(elem));
      if (r == -1)
        throw Validate_Error("rng validate full element error");
    }
    void validate_pop_element(Valid_Ctxt_Ptr &context,
					 const doc::Ptr &doc,
					 const xmlNode *elem)
    {
      int r = xmlRelaxNGValidatePopElement(context.get(),
          const_cast<xmlDoc*>(doc.get()),
          const_cast<xmlNode*>(elem));
      if (r == 0)
        throw Validate_Error("rng validate pop element error");
    }
    void validate_push_cdata(Valid_Ctxt_Ptr &context,
					 const char *begin, const char *end)
    {
      int r = xmlRelaxNGValidatePushCData(context.get(),
          reinterpret_cast<const xmlChar*>(begin), end-begin);
      if (r == -1)
        throw Validate_Error("rng validate push cdata error");
    }

    void set_valid_errors(Valid_Ctxt_Ptr &context,
        xmlRelaxNGValidityErrorFunc err,
        xmlRelaxNGValidityWarningFunc warn,
        void *user_data)
    {
      xmlRelaxNGSetValidErrors(context.get(), err, warn, user_data);
    }

    void set_valid_structured_errors(Valid_Ctxt_Ptr &context,
        xmlStructuredErrorFunc serror,
        void *user_data)
    {
      xmlRelaxNGSetValidStructuredErrors(context.get(), serror, user_data);
    }

  }


  doc::Ptr new_doc()
  {
    doc::Ptr r(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")), xmlFreeDoc);
    if (!r)
      throw Logic_Error("Could not create new document");
    return std::move(r);
  }


  Parser_Ctxt_Ptr new_parser_ctxt()
  {
    Parser_Ctxt_Ptr r(xmlNewParserCtxt(), xmlFreeParserCtxt);
    if (!r)
      throw Logic_Error("Could not allocate parser context");
    return std::move(r);
  }

  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const char *begin, const char *end,
      const char *URL, const char *encoding,
      int options)
  {
    doc::Ptr r(xmlCtxtReadMemory(parser_context.get(), begin, end-begin,
          URL, encoding, options), xmlFreeDoc);
    if (!r)
      throw Parse_Error("Could not parse XML from memory buffer with ctxt");
    return std::move(r);
  }
  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const std::string &s,
      const char *URL, const char *encoding,
      int options)
  {
    return ctxt_read_memory(parser_context, s.data(), s.data() + s.size(),
        URL, encoding, options);
  }
  doc::Ptr ctxt_read_memory(Parser_Ctxt_Ptr &parser_context,
      const char *s,
      const char *URL, const char *encoding,
      int options)
  {
    return ctxt_read_memory(parser_context, s, s + strlen(s),
        URL, encoding, options);
  }
  doc::Ptr ctxt_read_file(Parser_Ctxt_Ptr &parser_context,
      const char * filename,
      const char *encoding,
      int options)
  {
    doc::Ptr r(xmlCtxtReadFile(parser_context.get(), filename,
          encoding, options), xmlFreeDoc);
    if (!r)
      throw Parse_Error("Could not parse XML from file with ctxt: "
          + string(filename));
    return std::move(r);
  }
  doc::Ptr ctxt_read_file(Parser_Ctxt_Ptr &parser_context,
      const std::string &filename,
      const char *encoding,
      int options)
  {
    return ctxt_read_file(parser_context, filename.c_str(), encoding, options);
  }

  doc::Ptr read_memory(
      const char *begin, const char *end,
      const char *URL, const char *encoding,
      int options)
  {
    doc::Ptr r(xmlReadMemory(begin, end-begin,
          URL, encoding, options), xmlFreeDoc);
    if (!r)
      throw Parse_Error("Could not parse XML from memory buffer with ctxt");
    return std::move(r);
  }
  doc::Ptr read_memory(
      const char *s,
      const char *URL, const char *encoding,
      int options)
  {
    return read_memory(s, s + strlen(s),
        URL, encoding, options);
  }
  doc::Ptr read_memory(
      const std::string &s,
      const char *URL, const char *encoding,
      int options)
  {
    return read_memory(s.data(), s.data() + s.size(),
        URL, encoding, options);
  }
  doc::Ptr read_file(
      const char *filename,
      const char *encoding,
      int options)
  {
    doc::Ptr r(xmlReadFile(filename, encoding, options), xmlFreeDoc);
    if (!r)
      throw Parse_Error("Could not parse XML from file: " + string(filename));
    return std::move(r);
  }
  doc::Ptr read_file(
      const std::string &filename,
      const char *encoding,
      int options)
  {
    return read_file(filename.c_str(), encoding, options);
  }

  namespace dict {

    using Ptr = std::unique_ptr<xmlDict, void(*)(xmlDict*)>;

    Ptr create()
    {
      Ptr r(xmlDictCreate(), xmlDictFree);
      if (!r)
        throw Logic_Error("Could not create xml dictionary");
      return std::move(r);
    }

    bool owns(xmlDict *dict, const char *s)
    {
      if (!s)
        throw Logic_Error("xmlDict cannot own a null ptr");
      int r = (xmlDictOwns(dict, reinterpret_cast<const xmlChar*>(s)));
      if (r == -1)
        throw Runtime_Error("owns lookup error for: " + string(s));
      return r;
    }
    bool owns(Ptr &dict, const char *s)
    {
      return owns(dict.get(), s);
    }

    xmlChar *exists(xmlDict *dict, const char *s, unsigned length)
    {
      xmlChar *r = const_cast<xmlChar*>(xmlDictExists(dict,
          reinterpret_cast<const xmlChar*>(s), length));
      return r;
    }
    xmlChar *exists(xmlDict *dict, const char *s)
    {
      xmlChar *r = const_cast<xmlChar*>(xmlDictExists(dict,
          reinterpret_cast<const xmlChar*>(s), -1));
      return r;
    }
    xmlChar *exists(xmlDict *dict, const std::string &s)
    {
      return exists(dict, s.data(), s.size());
    }
    xmlChar *exists(xmlDict *dict, const char *begin, const char *end)
    {
      return exists(dict, begin, end-begin);
    }
    xmlChar *exists(Ptr &dict, const char *s, unsigned length)
    {
      return exists(dict.get(), s, length);
    }
    xmlChar *exists(Ptr &dict, const char *s)
    {
      return exists(dict.get(), s);
    }
    xmlChar *exists(Ptr &dict, const std::string &s)
    {
      return exists(dict.get(), s);
    }
    xmlChar *exists(Ptr &dict, const char *begin, const char *end)
    {
      return exists(dict.get(), begin, end);
    }

    xmlChar *lookup(xmlDict *dict, const char *s, unsigned length)
    {
      xmlChar *r = const_cast<xmlChar*>(xmlDictLookup(dict,
          reinterpret_cast<const xmlChar*>(s), length));
      if (!r)
        throw Logic_Error("xml dict lookup failed: " + string(s, length));
      return r;
    }
    xmlChar *lookup(xmlDict *dict, const char *s)
    {
      xmlChar *r = const_cast<xmlChar*>(xmlDictLookup(dict,
          reinterpret_cast<const xmlChar*>(s), -1));
      if (!r)
        throw Logic_Error("xml dict -1 lookup failed: " + string(s));
      return r;
    }
    xmlChar *lookup(xmlDict *dict, const std::string &s)
    {
      return lookup(dict, s.data(), s.size());
    }
    xmlChar *lookup(xmlDict *dict, const char *begin, const char *end)
    {
      return lookup(dict, begin, end-begin);
    }
    xmlChar *lookup(Ptr &dict, const char *s, unsigned length)
    {
      return lookup(dict.get(), s, length);
    }
    xmlChar *lookup(Ptr &dict, const char *s)
    {
      return lookup(dict.get(), s);
    }
    xmlChar *lookup(Ptr &dict, const std::string &s)
    {
      return lookup(dict.get(), s);
    }
    xmlChar *lookup(Ptr &dict, const char *begin, const char *end)
    {
      return lookup(dict.get(), begin, end);
    }

    const char *q_lookup(Ptr &dict, const char *prefix, const char *name)
    {
      const char *r = reinterpret_cast<const char*>(xmlDictQLookup(dict.get(),
          reinterpret_cast<const xmlChar*>(prefix),
          reinterpret_cast<const xmlChar*>(name)));
      if (!r)
        throw Logic_Error("xml q lookup failed: "
            + string(prefix) + " " + string(name));
      return r;
    }
    const char *q_lookup(Ptr &dict,
        const std::string &prefix, const std::string &name)
    {
      return q_lookup(dict, prefix.c_str(), name.c_str());
    }
  }


  namespace doc {

    xmlNode *get_root_element(const Ptr &doc)
    {
      auto r = xmlDocGetRootElement(doc.get());
      if (!r)
        throw Runtime_Error("document has no root");
      return r;
    }
    xmlNode *set_root_element(Ptr &doc, xmlNode *root)
    {
      return xmlDocSetRootElement(doc.get(), root);
    }
    std::pair<Char_Ptr, size_t> dump_format_memory(Ptr &doc, bool format)
    {
      xmlChar *s = nullptr;
      int n = 0;
      xmlDocDumpFormatMemory(doc.get(), &s, &n, format);
      if (!s || n < 0)
        throw Runtime_Error("could not format dump document");
      return std::pair<Char_Ptr, size_t>(Char_Ptr(reinterpret_cast<char*>(s), xmlFree), size_t(n));
    }

    unsigned format_dump(FILE *f, const Ptr &doc, bool format)
    {
      int r = xmlDocFormatDump(f, const_cast<xmlDoc*>(doc.get()),
          format);
      if (r == -1)
        throw Runtime_Error("doc format dump failed");
      return unsigned(r);
    }
  }

  void elem_dump(FILE *f, const doc::Ptr &doc, const xmlNode *node)
  {
    xmlElemDump(f, const_cast<xmlDoc*>(doc.get()),
        const_cast<xmlNode*>(node));
  }

  const char *name(const xmlNode *node)
  {
    return reinterpret_cast<const char*>(node->name);
  }
  const char *name(const xmlAttr *node)
  {
    return reinterpret_cast<const char*>(node->name);
  }
  const char *content(const xmlNode *node)
  {
    return reinterpret_cast<const char*>(node->content);
  }
  size_t child_element_count(const xmlNode *node)
  {
    return xmlChildElementCount(const_cast<xmlNode*>(node));
  }
  const xmlNode *first_element_child(const xmlNode *node)
  {
    return xmlFirstElementChild(const_cast<xmlNode*>(node));
  }
  xmlNode *first_element_child(xmlNode *node)
  {
    return xmlFirstElementChild(node);
  }
  const xmlNode *next_element_sibling(const xmlNode *node)
  {
    return xmlNextElementSibling(const_cast<xmlNode*>(node));
  }
  xmlNode *next_element_sibling(xmlNode *node)
  {
    return xmlNextElementSibling(node);
  }

  Output_Buffer_Ptr alloc_output_buffer(xmlCharEncodingHandler *encoder)
  {
    Output_Buffer_Ptr r(xmlAllocOutputBuffer(encoder), xmlOutputBufferClose);
    if (!r)
      throw Runtime_Error("Could not alloc output buffer");
    return r;
  }

  xmlNode *new_doc_node(doc::Ptr &doc, const char *name)
  {
    auto r = xmlNewDocNode(doc.get(), nullptr,
        reinterpret_cast<const xmlChar*>(name), nullptr);
    if (!r)
      throw Runtime_Error("Could not allocate new doc node");
    return r;
  }
  xmlNode *new_doc_node(doc::Ptr &doc, const std::string &name)
  {
    return new_doc_node(doc, name.c_str());
  }
  xmlNode *new_doc_node(doc::Ptr &doc, const char *name, const char *content)
  {
    auto r = xmlNewDocNode(doc.get(), nullptr,
        reinterpret_cast<const xmlChar*>(name),
        reinterpret_cast<const xmlChar*>(content)
        );
    if (!r)
      throw Runtime_Error("Could not allocate new doc node");
    return r;
  }
  xmlNode *new_doc_node(doc::Ptr &doc, const std::string &name,
      const std::string &content)
  {
    return new_doc_node(doc, name.c_str(), content.c_str());
  }

  xmlNode *new_doc_text(doc::Ptr &doc)
  {
    xmlNode *r = xmlNewDocText(doc.get(), nullptr);
    if (!r)
      throw Runtime_Error("Could not allocate doc text node");
    return r;
  }
  xmlNode *new_doc_text(doc::Ptr &doc, const char *text)
  {
    xmlNode *r = xmlNewDocText(doc.get(),
        reinterpret_cast<const xmlChar*>(text));
    if (!r)
      throw Runtime_Error("Could not allocate doc text node");
    return r;
  }
  xmlNode *new_doc_text(doc::Ptr &doc, const std::string &text)
  {
    return new_doc_text(doc, text.c_str());
  }


  xmlNode *new_child(xmlNode *parent, const char *name)
  {
    auto r = xmlNewChild(parent, nullptr,
        reinterpret_cast<const xmlChar*>(name), nullptr);
    if (!r)
      throw Runtime_Error("Could not allocate new child node");
    return r;
  }
  xmlNode *new_child(xmlNode *parent, const std::string &name)
  {
    return new_child(parent, name.c_str());
  }
  xmlNode *new_child(xmlNode *parent, const char *name, const char *content)
  {
    auto r = xmlNewChild(parent, nullptr,
        reinterpret_cast<const xmlChar*>(name),
        reinterpret_cast<const xmlChar*>(content)
        );
    if (!r)
      throw Runtime_Error("Could not allocate new child node");
    return r;
  }
  xmlNode *new_child(xmlNode *parent, const std::string &name,
      const std::string &content)
  {
    return new_child(parent, name.c_str(), content.c_str());
  }

  xmlNode *add_child(xmlNode *parent, xmlNode *node)
  {
    xmlNode *r = xmlAddChild(parent, node);
    if (!r)
      throw Runtime_Error("Could not add child");
    return r;
  }
  void node_add_content(xmlNode *node, const char *text, unsigned len)
  {
    xmlNodeAddContentLen(node,
        reinterpret_cast<const xmlChar*>(text), int(len));
  }
  void node_add_content(xmlNode *node, const std::string &text)
  {
    node_add_content(node, text.data(), text.size());
  }

  Node_Ptr unlink_node(xmlNode *node)
  {
    return Node_Ptr((xmlUnlinkNode(node), node), xmlFreeNode);
  }

  xmlAttr *new_prop(xmlNode *node, const char *name, const char *value)
  {
    auto r = xmlNewProp(node,
        reinterpret_cast<const xmlChar*>(name),
        reinterpret_cast<const xmlChar*>(value)
        );
    if (!r)
      throw Runtime_Error("Could not allocate property");
    return r;
  }
  xmlAttr *new_prop(xmlNode *node,
      const std::string &name, const std::string &value)
  {
    return new_prop(node, name.c_str(), value.c_str());
  }

  xmlAttr *set_prop(xmlNode *node, const char *name, const char *value)
  {
    auto r = xmlSetProp(node,
        reinterpret_cast<const xmlChar*>(name),
        reinterpret_cast<const xmlChar*>(value)
        );
    if (!r)
      throw Runtime_Error("Could not allocate property");
    return r;
  }
  xmlAttr *set_prop(xmlNode *node,
      const std::string &name, const std::string &value)
  {
    return set_prop(node, name.c_str(), value.c_str());
  }

  const char *get_prop(const xmlNode *node, const char *name)
  {
    const xmlChar *r = xmlGetProp(const_cast<xmlNode*>(node),
        reinterpret_cast<const xmlChar*>(name));
    if (!r)
      throw Runtime_Error("Attribute not available: " + string(name));
    return reinterpret_cast<const char*>(r);
  }

  namespace text_reader {

    Ptr for_memory(const char *begin, const char *end,
        const char *url,
        const char *encoding, int options)
    {
      Ptr r(xmlReaderForMemory(begin, end-begin, url, encoding, options),
          xmlFreeTextReader);
      if (!r)
        throw Runtime_Error("could not text read memory");
      return std::move(r);
    }
    Ptr for_memory(const char *s,
        const char *url,
        const char *encoding, int options)
    {
      Ptr r(xmlReaderForMemory(s, strlen(s), url, encoding, options),
          xmlFreeTextReader);
      if (!r)
        throw Runtime_Error("could not text read memory");
      return std::move(r);
    }
    Ptr for_memory(const std::string &s,
        const char *url,
        const char *encoding, int options)
    {
      Ptr r(xmlReaderForMemory(s.data(), s.size(), url, encoding, options),
          xmlFreeTextReader);
      if (!r)
        throw Runtime_Error("could not text read memory");
      return std::move(r);
    }
    Ptr for_file(const char *filename, const char *encoding,
        int options)
    {
      Ptr r(xmlReaderForFile(filename, encoding, options),
          xmlFreeTextReader);
      if (!r)
        throw Runtime_Error("could not text read open:" + string(filename));
      return std::move(r);
    }
    Ptr for_file(const std::string &filename, const char *encoding,
        int options)
    {
      return for_file(filename.c_str(), encoding, options);
    }

    bool read(Ptr &reader)
    {
      int r = xmlTextReaderRead(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader read failed");
      return r;
    }

    bool read_attribute_value(Ptr &reader)
    {
      int r = xmlTextReaderReadAttributeValue(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader read attribute failed");
      return r;
    }

    bool move_to_first_attribute(Ptr &reader)
    {
      int r = xmlTextReaderMoveToFirstAttribute(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader move first attribute failed");
      return r;
    }

    bool move_to_next_attribute(Ptr &reader)
    {
      int r = xmlTextReaderMoveToNextAttribute(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader move next attribute failed");
      return r;
    }

    const char *const_value(Ptr &reader)
    {
      return reinterpret_cast<const char*>(xmlTextReaderConstValue(reader.get()));
    }
    const char *const_name(Ptr &reader)
    {
      return reinterpret_cast<const char*>(xmlTextReaderConstName(reader.get()));
    }
    const char *const_local_name(Ptr &reader)
    {
      return reinterpret_cast<const char*>(xmlTextReaderConstLocalName(reader.get()));
    }
    const char *const_prefix(Ptr &reader)
    {
      return reinterpret_cast<const char*>(xmlTextReaderConstPrefix(reader.get()));
    }
    const char *const_namespace_uri(Ptr &reader)
    {
      return reinterpret_cast<const char*>(xmlTextReaderConstNamespaceUri(reader.get()));
    }
    int node_type(Ptr &reader)
    {
      int r = xmlTextReaderNodeType(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader node type failed");
      return r;
    }
    bool has_attributes(Ptr &reader)
    {
      int r = xmlTextReaderHasAttributes(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader has attributes failed");
      return r;
    }
    bool has_value(Ptr &reader)
    {
      int r = xmlTextReaderHasValue(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader has value failed");
      return r;
    }
    bool is_empty_element(Ptr &reader)
    {
      int r = xmlTextReaderIsEmptyElement(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader is empty element failed");
      return r;
    }
    bool is_valid(Ptr &reader)
    {
      int r = xmlTextReaderIsValid(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader is valid failed");
      return r;
    }
    unsigned depth(Ptr &reader)
    {
      int r = xmlTextReaderDepth(reader.get());
      if (r == -1)
        throw Runtime_Error("text reader depth failed");
      return r;
    }

    void set_parser_prop(Ptr &reader, int prop, int value)
    {
      int r = xmlTextReaderSetParserProp(reader.get(), prop, value);
      if (r == -1) {
        ostringstream o;
        o << "text reader set prop error: " << prop << " = " << value;
        throw Runtime_Error(o.str());
      }
    }

    void relaxng_validate(Ptr &reader)
    {
      int r = xmlTextReaderRelaxNGValidate(reader.get(), nullptr);
      if (r == -1)
        throw Runtime_Error("Could not disable rng filename validation");
    }
    void relaxng_validate(Ptr &reader, const char *filename)
    {
      int r = xmlTextReaderRelaxNGValidate(reader.get(), filename);
      if (r == -1)
        throw Runtime_Error("Could enable rng filename validation: "
            + string(filename));
    }
    void relaxng_validate(Ptr &reader, const std::string &filename)
    {
      return relaxng_validate(reader, filename.c_str());
    }
    void relaxng_validate_ctxt(Ptr &reader)
    {
      int r = xmlTextReaderRelaxNGValidateCtxt(reader.get(), nullptr, 0);
      if (r == -1)
        throw Runtime_Error("Could not disable rng context validation");
    }
    void relaxng_validate_ctxt(Ptr &reader, relaxng::Valid_Ctxt_Ptr &context)
    {
      int r = xmlTextReaderRelaxNGValidateCtxt(reader.get(), context.get(), 0);
      if (r == -1)
        throw Runtime_Error("Could not enable rng context validation");
    }
    void relaxng_set_schema(Ptr &reader)
    {
      int r = xmlTextReaderRelaxNGSetSchema(reader.get(), nullptr);
      if (r == -1)
        throw Runtime_Error("Could not disable rng schema validation");
    }
    void relaxng_set_schema(Ptr &reader, relaxng::Ptr &schema)
    {
      int r = xmlTextReaderRelaxNGSetSchema(reader.get(), schema.get());
      if (r == -1)
        throw Runtime_Error("Could not enable rng schema validation");
    }

  }

  namespace text_writer {
    void set_indent(Ptr &writer, bool b)
    {
      int r = xmlTextWriterSetIndent(writer.get(), b);
      if (r == -1)
        throw Runtime_Error("Could not set indent");
    }
    void set_quote_char(Ptr &writer, char c)
    {
      int r = xmlTextWriterSetQuoteChar(writer.get(),
          static_cast<xmlChar>(c));
      if (r == -1)
        throw Runtime_Error("Could not set quote char");
    }

    void start_document(Ptr &writer)
    {
      int r = xmlTextWriterStartDocument(writer.get(),
          nullptr, nullptr, nullptr);
      if (r == -1)
        throw Runtime_Error("could not start document");
    }
    void start_document(Ptr &writer, const char *version)
    {
      int r = xmlTextWriterStartDocument(writer.get(),
          version, nullptr, nullptr);
      if (r == -1)
        throw Runtime_Error("could not start document with version "
            + string(version));
    }
    void start_document(Ptr &writer, const char *version, const char *encoding)
    {
      int r = xmlTextWriterStartDocument(writer.get(),
          version, encoding, nullptr);
      if (r == -1)
        throw Runtime_Error("could not start document with version "
            + string(version) + ", encoding " + string(encoding));
    }
    void start_document(Ptr &writer, const char *version, const char *encoding,
        bool standalone)
    {
      int r = xmlTextWriterStartDocument(writer.get(),
          version, encoding, standalone ? "yes" : "no");
      if (r == -1)
        throw Runtime_Error("could not start document with version "
            + string(version) + ", encoding " + string(encoding)
            + ", standalone " + string(standalone ? "yes" : "no"));
    }
    void end_document(Ptr &writer)
    {
      int r = xmlTextWriterEndDocument(writer.get());
      if (r == -1)
        throw Runtime_Error("could not end document");
    }

    void start_element(Ptr &writer, const char *name)
    {
      int r = xmlTextWriterStartElement(writer.get(),
          reinterpret_cast<const xmlChar*>(name));
      if (r == -1)
        throw Runtime_Error("error starting element: " + string(name));
    }
    void end_element(Ptr &writer)
    {
      int r = xmlTextWriterEndElement(writer.get());
      if (r == -1)
        throw Runtime_Error("error closing element");
    }

    void start_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *namespace_uri)
    {
      int r = xmlTextWriterStartElementNS(writer.get(),
          reinterpret_cast<const xmlChar*>(prefix),
          reinterpret_cast<const xmlChar*>(name),
          reinterpret_cast<const xmlChar*>(namespace_uri));
      if (r == -1)
        throw Runtime_Error("error starting ns element: " + string(name)
            + ", " + string(prefix) + ", " + string(namespace_uri));
    }
    //void end_element(Ptr &writer);

    void start_attribute(Ptr &writer, const char *name)
    {
      int r = xmlTextWriterStartAttribute(writer.get(),
          reinterpret_cast<const xmlChar*>(name));
      if (r == -1)
        throw Runtime_Error("error starting attribute: " + string(name));
    }
    void end_attribute(Ptr &writer)
    {
      int r = xmlTextWriterEndAttribute(writer.get());
      if (r == -1)
        throw Runtime_Error("error closing attribute");
    }

    void write_string(Ptr &writer, const char *content)
    {
      int r = xmlTextWriterWriteString(writer.get(),
          reinterpret_cast<const xmlChar*>(content));
      if (r == -1)
        throw Runtime_Error("error writing string ");
    }
    void write_raw(Ptr &writer, const char *begin, const char *end)
    {
      int r = xmlTextWriterWriteRawLen(writer.get(),
          reinterpret_cast<const xmlChar*>(begin), end-begin);
      if (r == -1)
        throw Runtime_Error("error writing raw string");
    }

    void write_comment(Ptr &writer, const char *comment)
    {
      int r = xmlTextWriterWriteComment(writer.get(),
          reinterpret_cast<const xmlChar*>(comment));
      if (r == -1)
        throw Runtime_Error("error writing comment");
    }

    void write_element(Ptr &writer, const char *name, const char *content)
    {
      int r = xmlTextWriterWriteElement(writer.get(),
          reinterpret_cast<const xmlChar*>(name),
          reinterpret_cast<const xmlChar*>(content));
      if (r == -1)
        throw Runtime_Error("error writing element: " + string(name));
    }

    void write_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *namespace_uri, const char *content)
    {
      int r = xmlTextWriterWriteElementNS(writer.get(),
          reinterpret_cast<const xmlChar*>(prefix),
          reinterpret_cast<const xmlChar*>(name),
          reinterpret_cast<const xmlChar*>(namespace_uri),
          reinterpret_cast<const xmlChar*>(content));
      if (r == -1)
        throw Runtime_Error("error writing ns element: " + string(name));
    }
    void write_element_ns(Ptr &writer, const char *prefix, const char *name,
        const char *content)
    {
      int r = xmlTextWriterWriteElementNS(writer.get(),
          reinterpret_cast<const xmlChar*>(prefix), reinterpret_cast<const xmlChar*>(name),
          nullptr,
          reinterpret_cast<const xmlChar*>(content));
      if (r == -1)
        throw Runtime_Error("error writing ns element: " + string(name));
    }

    void write_attribute(Ptr &writer, const char *name, const char *content)
    {
      int r = xmlTextWriterWriteAttribute(writer.get(),
          reinterpret_cast<const xmlChar*>(name),
          reinterpret_cast<const xmlChar*>(content));
      if (r == -1)
        throw Runtime_Error("error writing attribute: " + string(name));
    }

    void flush(Ptr &writer)
    {
      int r = xmlTextWriterFlush(writer.get());
      if (r == -1)
        throw Runtime_Error("error flush text writer");
    }
  }

  text_writer::Ptr new_text_writer(Output_Buffer_Ptr o)
  {
    text_writer::Ptr r = text_writer::Ptr(xmlNewTextWriter(o.release()),
        xmlFreeTextWriter);
    if (!r)
      throw Runtime_Error("Could not create text writer for output buffer");
    return std::move(r);
  }
  text_writer::Ptr new_text_writer_filename(const char *filename,
      bool compression)
  {
    text_writer::Ptr r = text_writer::Ptr(
        xmlNewTextWriterFilename(filename, compression),
        xmlFreeTextWriter);
    if (!r)
      throw Runtime_Error("Could not open text writer for: "
          + string(filename));
    return std::move(r);
  }
  text_writer::Ptr new_text_writer_filename(const std::string &filename,
      bool compression)
  {
    return new_text_writer_filename(filename.c_str(), compression);
  }

}
