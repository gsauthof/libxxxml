#include <boost/test/unit_test.hpp>

#include <iostream>
#include <vector>
#include <array>

#include <libxml/xpathInternals.h>

#include <xxxml/xxxml.hh>

/* libxml2 Examples - Executable Documentation
 
   2015, Georg Sauthoff <mail@georg.so>

*/

using namespace std;

const char rng_schema_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<grammar datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'
         xmlns='http://relaxng.org/ns/structure/1.0'>
         <!-- declaring a default namespace: -->
         <!-- ns='http://example.org/foo' -->
  <start>
    <element name='root'>
      <ref name='root'/>
    </element>
  </start>

  <define name='root'>
    <element name='foo'>
      <data type='string'/>
    </element>
    <oneOrMore>
      <element name='bar'>
        <data type='string'/>
      </element>
    </oneOrMore>
  </define>

</grammar>
)";
const char rng_records_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<grammar datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'
         xmlns='http://relaxng.org/ns/structure/1.0'>
         <!-- declaring a default namespace: -->
         <!-- ns='http://example.org/foo' -->
  <start>
    <element name='root'>
      <ref name='root'/>
    </element>
  </start>
  <define name='root'>
    <element name='records'>
      <ref name='records'/>
    </element>
  </define>
  <define name='records'>
    <oneOrMore>
      <element name='record'>
        <ref name='record'/>
      </element>
    </oneOrMore>
  </define>
  <define name='record'>
    <element name='a'>
      <data type='string'/>
    </element>
    <oneOrMore>
      <element name='b'>
        <data type='string'>
          <param name='pattern'>[0-9]{2,10}</param>
        </data>
      </element>
    </oneOrMore>
  </define>
</grammar>
)"
;

BOOST_AUTO_TEST_SUITE(libxxxml)

  using namespace xxxml;

  BOOST_AUTO_TEST_SUITE(basic)
    // {{{

    BOOST_AUTO_TEST_CASE(basic)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
    }

    BOOST_AUTO_TEST_CASE(unshared_name_ptr)
    {
      doc::Ptr d = read_memory(
          "<root><bar>23</bar><foo>Hello</foo><bar>World</bar></root>",
          nullptr, nullptr, XML_PARSE_NODICT
          );
      BOOST_REQUIRE(d.get());
      BOOST_CHECK(d.get()->dict == nullptr);
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");
      BOOST_CHECK_EQUAL(child_element_count(root), 3u);
      // the pointer values for bar and foo should be different:
      BOOST_CHECK(root->children->name != root->children->next->name);
      // but for the 1st and 3rd child that share the name bar,
      // they could thus share a pointer, but this is not how
      // libxml operates ...
      // ... when the use of dictionaries is disabled
      BOOST_CHECK(
          root->children->name != root->children->next->next->name);
    }

    // XXX add test with XML_PARSE_NONET, for disabling any net access, e.g.
    // during the processing of some advanced XML features

    BOOST_AUTO_TEST_CASE(shared_name_ptr)
    {
      Parser_Ctxt_Ptr parser_context = new_parser_ctxt();
      doc::Ptr d = ctxt_read_memory(parser_context,
          "<root><bar>23</bar><foo>Hello</foo><bar>World</bar><bar>23</bar></root>",
          nullptr, nullptr, 0);
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");
      BOOST_CHECK_EQUAL(child_element_count(root), 4u);
      // the pointer values for bar and foo should be different:
      BOOST_CHECK(root->children->name != root->children->next->name);
      // but for the 1st and 3rd child that share the name bar,
      // they could thus share a pointer, and this is how libxml
      // behaves
      BOOST_CHECK(
          root->children->name == root->children->next->next->name);
      BOOST_CHECK(d.get()->dict != nullptr);

      BOOST_CHECK(
          root->children->name == root->children->next->next->next->name);
      BOOST_CHECK(
          root->children->content == root->children->next->next->next->content);

      // names
      BOOST_CHECK(dict::exists(d.get()->dict, "bar"));
      BOOST_CHECK(dict::exists(d.get()->dict, "foo"));
      // values smaller than 4 are shared
      BOOST_CHECK(dict::exists(d.get()->dict, "23"));
      // larger values are not
      BOOST_CHECK(!dict::exists(d.get()->dict, "Hello"));
      BOOST_CHECK(!dict::exists(d.get()->dict, "World"));
    }

    BOOST_AUTO_TEST_CASE(dump)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");
      Output_Buffer_Ptr b = alloc_output_buffer();

      xmlNodeDumpOutput(b.get(), d.get(), const_cast<xmlNode*>(root),
          0, 1, nullptr);
      const char *begin = (char*)xmlBufContent(b.get()->buffer);
      const char *end = (char*)xmlBufEnd(b.get()->buffer);
      string s(begin, end);
      BOOST_CHECK(s.find("<bar>World</bar>") != s.npos);
      //cout << s << '\n';
      // prints:
      // <root>
      //   <foo>Hello</foo>
      //   <bar>World</bar>
      // </root>
      //
    }

    BOOST_AUTO_TEST_CASE(not_well_formed)
    {
      BOOST_CHECK_THROW(
          read_memory("<root><foo>Hello</foo><bar>World</baz></root>"),
          xxxml::Parse_Error);
    }

  BOOST_AUTO_TEST_SUITE_END() // basic

  BOOST_AUTO_TEST_SUITE(from_scratch)

    BOOST_AUTO_TEST_CASE(basic)
    {
      doc::Ptr d = new_doc();
      BOOST_CHECK(d.get()->children == nullptr);
      xmlNode *root = new_doc_node(d, "root");
      doc::set_root_element(d, root);
      BOOST_CHECK(d.get()->children == root);
      // {_private = 0x0, type = XML_ELEMENT_NODE,
      // name = 0x60200000e4f0 "root", children = 0x0, last = 0x0,
      // parent = 0x0, next = 0x0, prev = 0x0, doc = 0x60f00000eb90,
      // ns = 0x0, content = 0x0, properties = 0x0, nsDef = 0x0, psvi = 0x0, 
      // line = 0, extra = 0}
      BOOST_CHECK_EQUAL(root->type, XML_ELEMENT_NODE);
      xmlNode *foo = new_child(root, "foo", "Hello");
      // {_private = 0x0, type = XML_ELEMENT_NODE,
      // name = 0x60200000e4d0 "foo", children = 0x60c00000afc0,
      // last = 0x60c00000afc0, parent = 0x60c00000b140, next = 0x0,
      // prev = 0x0, doc = 0x60f00000eb90, ns = 0x0, content = 0x0,
      // properties = 0x0, nsDef = 0x0, psvi = 0x0, line = 0, extra = 0}
      BOOST_CHECK(foo->doc == d.get());
      BOOST_CHECK(foo->parent == root);
      xmlNode *bar = new_child(root, "bar", "World");
      BOOST_CHECK(bar->doc == d.get());
      BOOST_CHECK(bar->children == bar->last);
      BOOST_CHECK_EQUAL(bar->children->type, XML_TEXT_NODE);
      BOOST_CHECK_EQUAL(content(bar->children), "World");
      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<foo>Hello</foo>") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(unshared_name)
    {
      doc::Ptr d = new_doc();
      BOOST_CHECK(d.get()->children == nullptr);
      xmlNode *root = new_doc_node(d, "root");
      doc::set_root_element(d, root);

      const char bar_name[] = "bar";
      xmlNode *bar1 = new_child(root, bar_name, "World");
      xmlNode *foo = new_child(root, "foo", "Hello");
      BOOST_CHECK(foo->doc == d.get());
      xmlNode *bar2 = new_child(root, bar_name, "23");
      BOOST_CHECK(bar1->name != (const xmlChar*)bar_name);
      // Could share the pointer, but libxml does not do it
      // by default with documents created by xmlNewDoc()
      // (this is contrary to documents created by `xml*Read*()`
      BOOST_CHECK(bar1->name != bar2->name);

      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<foo>Hello</foo>") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(shared_with_dict)
    {
      doc::Ptr d = new_doc();
      dict::Ptr dictionary = dict::create();
      // important that dictionary is released, since assigning it
      // to an xmlDoc objects transfers ownership
      d.get()->dict = dictionary.release();
      BOOST_CHECK(d.get()->children == nullptr);
      xmlNode *root = new_doc_node(d, "root");
      doc::set_root_element(d, root);

      const char bar_name[] = "bar";
      xmlNode *bar1 = new_child(root, bar_name, "World");
      xmlNode *bar3 = new_child(root, bar_name, "23");
      xmlNode *foo = new_child(root, "foo", "Hello");
      BOOST_CHECK(foo->doc == d.get());
      xmlNode *bar2 = new_child(root, "bar", "23");
      // of course, the passed string was copied
      BOOST_CHECK(bar1->name != (const xmlChar*)bar_name);
      // Now - since we are using a dictionary, name pointers are shared
      BOOST_CHECK(bar1->name == bar2->name);
      BOOST_CHECK(bar1->name == bar3->name);

      BOOST_CHECK(dict::exists(d.get()->dict, "bar"));
      BOOST_CHECK(dict::exists(d.get()->dict, "foo"));
      BOOST_CHECK(!dict::exists(d.get()->dict, "baz"));
      // values are NOT in the dict
      BOOST_CHECK(!dict::exists(d.get()->dict, "23"));
      BOOST_CHECK(!dict::exists(d.get()->dict, "World"));

      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<foo>Hello</foo>") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(shared_content_with_dict)
    {
      doc::Ptr d = new_doc();
      dict::Ptr dictionary = dict::create();
      // important that dictionary is released, since assigning it
      // to an xmlDoc objects transfers ownership
      d.get()->dict = dictionary.release();

      xmlDict* x = d.get()->dict;

      BOOST_CHECK(d.get()->children == nullptr);
      xmlNode *root = new_doc_node(d, "root");
      doc::set_root_element(d, root);

      const char bar_name[] = "bar";
      xmlNode *bar1 = new_child(root, bar_name);
      xmlNode *bar1_text = new_doc_text(d);
      bar1_text->content = dict::lookup(x, "World");
      add_child(bar1, bar1_text);
      xmlNode *bar3 = new_child(root, bar_name);
      xmlNode *bar3_text = new_doc_text(d);
      bar3_text->content = dict::lookup(x, "23");
      add_child(bar3, bar3_text);
      xmlNode *foo = new_child(root, "foo");
      xmlNode *foo_text = new_doc_text(d);
      foo_text->content = dict::lookup(x, "Hello");
      add_child(foo, foo_text);
      BOOST_CHECK(foo->doc == d.get());
      xmlNode *bar2 = new_child(root, "bar");
      xmlNode *bar2_text = new_doc_text(d);
      bar2_text->content = dict::lookup(x, "23");
      add_child(bar2, bar2_text);
      // of course, the passed string was copied
      BOOST_CHECK(bar1->name != (const xmlChar*)bar_name);
      // Now - since we are using a dictionary, name pointers are shared
      BOOST_CHECK(bar1->name == bar2->name);
      BOOST_CHECK(bar1->name == bar3->name);
      BOOST_CHECK(bar3->content == bar2->content);

      BOOST_CHECK(dict::exists(d.get()->dict, "bar"));
      BOOST_CHECK(dict::exists(d.get()->dict, "foo"));
      BOOST_CHECK(!dict::exists(d.get()->dict, "baz"));
      // values are now in the dict
      BOOST_CHECK(dict::exists(d.get()->dict, "23"));
      BOOST_CHECK(dict::exists(d.get()->dict, "World"));

      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<foo>Hello</foo>") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(validate)
    {
      const char *schema_s = rng_records_s;
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = new_doc();
      xmlNode *root = new_doc_node(d, "root");
      int r = relaxng::validate_push_element(v, d, root);
      BOOST_CHECK_EQUAL(r, 1);
      doc::set_root_element(d, root);
      xmlNode *records = new_child(root, "records");
      r = relaxng::validate_push_element(v, d, records);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *record1 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record1);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r1a = new_child(record1, "a", "Hello");
      r = relaxng::validate_push_element(v, d, r1a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1a);
      xmlNode *r1b = new_child(record1, "b", "23");
      r = relaxng::validate_push_element(v, d, r1b);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1b);
      relaxng::validate_pop_element(v, d, record1);
      xmlNode *record2 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record2);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r2a = new_child(record2, "a", "World");
      r = relaxng::validate_push_element(v, d, r2a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r2a);
      xmlNode *r2b = new_child(record2, "b", "42");
      r = relaxng::validate_push_element(v, d, r2b);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r2b);
      relaxng::validate_pop_element(v, d, record2);
      relaxng::validate_pop_element(v, d, records);
      relaxng::validate_pop_element(v, d, root);

      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("      <a>Hello</a>") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(validate_and_free_elements_as_early_as_possible)
    {
      const char *schema_s = rng_records_s;
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = new_doc();
      xmlNode *root = new_doc_node(d, "root");
      int r = relaxng::validate_push_element(v, d, root);
      BOOST_CHECK_EQUAL(r, 1);
      doc::set_root_element(d, root);
      xmlNode *records = new_child(root, "records");
      r = relaxng::validate_push_element(v, d, records);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *record1 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record1);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r1a = new_child(record1, "a", "Hello");
      r = relaxng::validate_push_element(v, d, r1a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1a);
      unlink_node(r1a);
      r1a = nullptr;
      xmlNode *r1b = new_child(record1, "b", "23");
      r = relaxng::validate_push_element(v, d, r1b);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1b);
      unlink_node(r1b);
      r1b = nullptr;
      relaxng::validate_pop_element(v, d, record1);
      unlink_node(record1);
      record1 = nullptr;
      xmlNode *record2 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record2);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r2a = new_child(record2, "a", "World");
      r = relaxng::validate_push_element(v, d, r2a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r2a);
      unlink_node(r2a);
      r2a = nullptr;
      xmlNode *r2b = new_child(record2, "b", "42");
      r = relaxng::validate_push_element(v, d, r2b);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r2b);
      unlink_node(r2b);
      r2b = nullptr;
      relaxng::validate_pop_element(v, d, record2);
      unlink_node(record2);
      record2 = nullptr;
      relaxng::validate_pop_element(v, d, records);
      unlink_node(records);
      records = nullptr;
      relaxng::validate_pop_element(v, d, root);
      unlink_node(root);
      root = nullptr;
    }

    BOOST_AUTO_TEST_CASE(invalid_pattern)
    {
      const char *schema_s = rng_records_s;
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = new_doc();
      xmlNode *root = new_doc_node(d, "root");
      int r = relaxng::validate_push_element(v, d, root);
      BOOST_CHECK_EQUAL(r, 1);
      doc::set_root_element(d, root);
      xmlNode *records = new_child(root, "records");
      r = relaxng::validate_push_element(v, d, records);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *record1 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record1);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r1a = new_child(record1, "a", "Hello");
      r = relaxng::validate_push_element(v, d, r1a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1a);
      unlink_node(r1a);
      r1a = nullptr;
      xmlNode *r1b = new_child(record1, "b", "23a");
      r = relaxng::validate_push_element(v, d, r1b);
      BOOST_CHECK_EQUAL(r, 0);
      BOOST_CHECK_THROW(relaxng::validate_full_element(v, d, r1b),
          xxxml::Validate_Error);
    }

    BOOST_AUTO_TEST_CASE(invalid_element)
    {
      const char *schema_s = rng_records_s;
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = new_doc();
      xmlNode *root = new_doc_node(d, "root");
      int r = relaxng::validate_push_element(v, d, root);
      BOOST_CHECK_EQUAL(r, 1);
      doc::set_root_element(d, root);
      xmlNode *records = new_child(root, "records");
      r = relaxng::validate_push_element(v, d, records);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *record1 = new_child(records, "record");
      r = relaxng::validate_push_element(v, d, record1);
      BOOST_CHECK_EQUAL(r, 1);
      xmlNode *r1a = new_child(record1, "a", "Hello");
      r = relaxng::validate_push_element(v, d, r1a);
      BOOST_CHECK_EQUAL(r, 0);
      relaxng::validate_full_element(v, d, r1a);
      unlink_node(r1a);
      r1a = nullptr;
      xmlNode *r1b = new_child(record1, "a", "23");
      BOOST_CHECK_THROW(relaxng::validate_push_element(v, d, r1b),
          xxxml::Validate_Error);
    }

    BOOST_AUTO_TEST_CASE(create_attribute)
    {
      doc::Ptr d = new_doc();
      BOOST_CHECK(d.get()->children == nullptr);
      xmlNode *root = new_doc_node(d, "root");
      doc::set_root_element(d, root);
      xmlNode *foo = new_child(root, "foo", "Hello");
      BOOST_CHECK(foo->parent == root);
      xmlNode *bar = new_child(root, "bar", "World");
      BOOST_CHECK(bar->parent == root);
      xmlAttr *att = new_prop(bar, "prio", "1");
      BOOST_CHECK(att->parent == bar);
      // (gdb) p *bar->properties
      //   {_private = 0x0, type = XML_ATTRIBUTE_NODE,
      //    name = 0x60200000e350 "prio", children = 0x60c00000a9c0,
      //    last = 0x60c00000a9c0, parent = 0x60c00000ab40, next = 0x0,
      //     prev = 0x0, doc = 0x60f00000eaa0, ns = 0x0, atype = 0, psvi = 0x0}
      // (gdb) p *bar->properties->children
      //   {_private = 0x0, type = XML_TEXT_NODE,
      //    name = 0x7ffff6a00e5e <xmlStringText> "text", children = 0x0,
      //    last = 0x0, parent = 0x60800000b520, next = 0x0, prev = 0x0,
      //    doc = 0x60f00000eaa0, ns = 0x0, content = 0x60200000e330 "1", 
      //    properties = 0x0, nsDef = 0x0, psvi = 0x0, line = 0, extra = 0}
      BOOST_CHECK(bar->properties == att);
      BOOST_CHECK_EQUAL(att->type, XML_ATTRIBUTE_NODE);
      BOOST_CHECK(att->children == att->last);
      BOOST_CHECK_EQUAL(att->children->type, XML_TEXT_NODE);
      pair<Char_Ptr, size_t> dump = doc::dump_format_memory(d);
      string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<bar prio=\"1\">World</bar>") != string::npos);
    }

    // }}}
  BOOST_AUTO_TEST_SUITE_END() // from_scratch

  BOOST_AUTO_TEST_SUITE(xpath_)
    // {{{

    BOOST_AUTO_TEST_CASE(basic)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      xpath::Object_Ptr o = xpath::eval("//bar/text()", c);

      // o:
      //   {type = XPATH_NODESET, nodesetval = 0x60200000e290, boolval = 0,
      //    floatval = 0, stringval = 0x0, user = 0x0, index = 0, user2 = 0x0,
      //    index2 = 0}
      // nodesetval:
      //   {nodeNr = 1, nodeMax = 10, nodeTab = 0x60700000d8b0}
      // nodeTab (xmlNodePtr)
      //   {_private = 0x0, type = XML_TEXT_NODE,
      //     name = 0x7ffff6a00e5e <xmlStringText> "text", children = 0x0,
      //     last = 0x0, parent = 0x60c00000b2c0, next = 0x0, prev = 0x0,
      //     doc = 0x60f00000ec80, ns = 0x0, content = 0x60200000e690 "World", 
      //     properties = 0x0, nsDef = 0x0, psvi = 0x0, line = 0, extra = 0}
      BOOST_CHECK_EQUAL(o.get()->type, XPATH_NODESET);
      BOOST_CHECK_EQUAL(o.get()->nodesetval->nodeNr, 1);
      BOOST_CHECK_EQUAL(name(o.get()->nodesetval->nodeTab[0]), "text");
      BOOST_CHECK_EQUAL(content(o.get()->nodesetval->nodeTab[0]), "World");
    }

    BOOST_AUTO_TEST_CASE(none)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      xpath::Object_Ptr o = xpath::eval("//barz/text()", c);
      BOOST_CHECK_EQUAL(o.get()->type, XPATH_NODESET);
      BOOST_CHECK(o.get()->nodesetval == nullptr);
    }

    BOOST_AUTO_TEST_CASE(throw_malformed)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      BOOST_CHECK_THROW(xpath::eval("//barz/text)", c), xxxml::Eval_Error);
    }

    BOOST_AUTO_TEST_CASE(compiled)
    {
      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar><x><bar>blah</bar></x></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      xpath::Comp_Expr_Ptr e = xpath::ctxt_compile(c, "//bar/text()");
      xpath::Object_Ptr o = xpath::compiled_eval(e, c);
      BOOST_CHECK_EQUAL(o.get()->type, XPATH_NODESET);
      BOOST_CHECK_EQUAL(o.get()->nodesetval->nodeNr, 2);
      BOOST_CHECK_EQUAL(name(o.get()->nodesetval->nodeTab[1]), "text");
      BOOST_CHECK_EQUAL(content(o.get()->nodesetval->nodeTab[1]), "blah");
    }

    BOOST_AUTO_TEST_CASE(compiled_without_ctxt)
    {
      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar><x><bar>blah</bar></x></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      xpath::Comp_Expr_Ptr e = xpath::compile("//bar/text()");
      xpath::Object_Ptr o = xpath::compiled_eval(e, c);
      BOOST_CHECK_EQUAL(o.get()->type, XPATH_NODESET);
      BOOST_CHECK_EQUAL(o.get()->nodesetval->nodeNr, 2);
      BOOST_CHECK_EQUAL(name(o.get()->nodesetval->nodeTab[1]), "text");
      BOOST_CHECK_EQUAL(content(o.get()->nodesetval->nodeTab[1]), "blah");
    }

    struct Proxy {
      string message;
    };
    void local_generic_error_fn(void *ctx, const char * msg, ...)
    {
      Proxy *p = static_cast<Proxy*>(ctx);
      char array[4096] = {0};
      va_list ap;
      va_start(ap, msg);
      vsnprintf(array, sizeof(array), msg, ap);
      va_end(ap);
      p->message = string(array, array+sizeof(array)-1);
    }

    BOOST_AUTO_TEST_CASE(local_error_fn)
    {
      Proxy p;
      xmlSetGenericErrorFunc(&p, local_generic_error_fn);
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      BOOST_CHECK_THROW(xpath::eval("//barz/text)", c), xxxml::Eval_Error);
      BOOST_CHECK(p.message.find("Invalid expression") != string::npos);
      p.message.clear();
      // reset to default fn
      // -> Is not enough! The context needs to be reset, too!
      //    (otherwise, the default handler interprets the p as FILE* ...)
      //initGenericErrorDefaultFunc(nullptr);
      xmlSetGenericErrorFunc(stderr, nullptr);
      BOOST_CHECK_THROW(xpath::eval("//barz/text)", c), xxxml::Eval_Error);
      BOOST_CHECK(p.message.empty());
    }

    BOOST_AUTO_TEST_CASE(register_var)
    {
      doc::Ptr d = read_memory("<root><x>Hello</x><x>x&apos;yz</x><x>World</x></root>");
      xpath::Context_Ptr c = xpath::new_context(d);
      xpath::register_variable(c, "bvar", 
          xpath::new_cstring("x'yz"));
      xpath::Object_Ptr o = xpath::eval("//x[. = $bvar]/text()", c);
      BOOST_CHECK_EQUAL(o.get()->type, XPATH_NODESET);
      BOOST_CHECK(o.get()->nodesetval != nullptr);
      BOOST_CHECK_EQUAL(o.get()->nodesetval->nodeNr, 1);
      BOOST_CHECK_EQUAL(name(o.get()->nodesetval->nodeTab[0]), "text");
      BOOST_CHECK_EQUAL(content(o.get()->nodesetval->nodeTab[0]), "x'yz");

      // provoke memory leak ...
      //d.release();
    }


    // }}}
  BOOST_AUTO_TEST_SUITE_END() // xpath_

  BOOST_AUTO_TEST_SUITE(schema_)
    // {{{

    BOOST_AUTO_TEST_CASE(basic)
    {
      const char schema_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<xs:schema
  xmlns:xs='http://www.w3.org/2001/XMLSchema'
  elementFormDefault='qualified'>
  <!-- targetNamespace='http://example.org/foo'
        xmlns='http://example.org/foo'
        xmlns:ns='http://example.org/foo' -->

  <xs:element name='root' type='root'/>

  <xs:complexType name='root'>
    <xs:sequence>
      <xs:element name='foo' type='xs:string'/>
      <xs:element name='bar' type='xs:string' maxOccurs='unbounded'/>
    </xs:sequence>
  </xs:complexType>
</xs:schema>
)";
      schema::Parser_Ctxt_Ptr pc = schema::new_mem_parser_ctxt(schema_s);
      schema::Ptr schema = schema::parse(pc);
      schema::Valid_Ctxt_Ptr v = schema::new_valid_ctxt(schema);

      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_CHECK_NO_THROW(schema::validate_doc(v, d));
    }

    BOOST_AUTO_TEST_CASE(non_valid)
    {
      const char schema_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<xs:schema
  xmlns:xs='http://www.w3.org/2001/XMLSchema'
  elementFormDefault='qualified'>
  <!-- targetNamespace='http://example.org/foo'
        xmlns='http://example.org/foo'
        xmlns:ns='http://example.org/foo' -->

  <xs:element name='root' type='root'/>

  <xs:complexType name='root'>
    <xs:sequence>
      <xs:element name='foo' type='xs:string'/>
      <xs:element name='bar' maxOccurs='unbounded'>
        <xs:simpleType>
          <xs:restriction base='xs:string'>
            <xs:pattern value='[A-Z]{1,20}'/>
          </xs:restriction>
        </xs:simpleType>
      </xs:element>
    </xs:sequence>
  </xs:complexType>
</xs:schema>
)";
      schema::Parser_Ctxt_Ptr pc = schema::new_mem_parser_ctxt(schema_s);
      schema::Ptr schema = schema::parse(pc);
      schema::Valid_Ctxt_Ptr v = schema::new_valid_ctxt(schema);

      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_CHECK_THROW(schema::validate_doc(v, d), xxxml::Validate_Error);
    }

    // }}}
  BOOST_AUTO_TEST_SUITE_END() // schema_

  BOOST_AUTO_TEST_SUITE(relaxng_)
    //{{{

    BOOST_AUTO_TEST_CASE(basic)
    {
      const char *schema_s = rng_schema_s;
      //xmlRelaxNGInitTypes();
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_CHECK_NO_THROW(relaxng::validate_doc(v, d));
    }

    BOOST_AUTO_TEST_CASE(invalid)
    {
      const char schema_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<grammar datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'
         xmlns='http://relaxng.org/ns/structure/1.0'>
         <!-- declaring a default namespace: -->
         <!-- ns='http://example.org/foo' -->
  <start>
    <element name='root'>
      <ref name='root'/>
    </element>
  </start>

  <define name='root'>
    <element name='foo'>
      <data type='string'/>
    </element>
    <oneOrMore>
      <element name='bar'>
        <data type='string'>
          <param name="pattern">[A-Z]{2,20}</param>
        </data>
      </element>
    </oneOrMore>
  </define>

</grammar>
)";
      //xmlRelaxNGInitTypes();
      relaxng::Parser_Ctxt_Ptr pc = relaxng::new_mem_parser_ctxt(schema_s);
      relaxng::Ptr schema = relaxng::parse(pc);
      relaxng::Valid_Ctxt_Ptr v = relaxng::new_valid_ctxt(schema);

      doc::Ptr d = read_memory(
          "<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_CHECK_THROW(relaxng::validate_doc(v, d), xxxml::Validate_Error);
    }

    // cf. relaxng_invalid_with_reporting for an example with non default
    // reporting

    //}}}
  BOOST_AUTO_TEST_SUITE_END() // relaxng_

  BOOST_AUTO_TEST_SUITE(reader)
  //{{{

    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace text_reader;
      Ptr reader = for_memory(
          "<root><foo>Hello</foo><bar id='1' x='23'>World</bar></root>");
      BOOST_CHECK(reader.get());
      int r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "root");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(depth(reader), 0u);
      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "foo");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(depth(reader), 1u);
      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_TEXT_NODE);
      BOOST_CHECK_EQUAL(has_value(reader), true);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(const_value(reader), "Hello");
      BOOST_CHECK_EQUAL(depth(reader), 2u);
      r = read(reader);
      BOOST_CHECK(r == 1);
      // XML_ELEMENT_DECL -> end tag
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_DECL);
      BOOST_CHECK_EQUAL(const_local_name(reader), "foo");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(depth(reader), 1u);
      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "bar");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), true);

      r = move_to_first_attribute(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ATTRIBUTE_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "id");
      BOOST_CHECK_EQUAL(const_value(reader), "1");
      BOOST_CHECK_EQUAL(has_value(reader), true);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(depth(reader), 2u);
      r = move_to_next_attribute(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ATTRIBUTE_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "x");
      BOOST_CHECK_EQUAL(const_value(reader), "23");
      BOOST_CHECK_EQUAL(has_value(reader), true);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      r = move_to_next_attribute(reader);
      BOOST_CHECK(r == 0);

      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_TEXT_NODE);
      BOOST_CHECK_EQUAL(has_value(reader), true);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      BOOST_CHECK_EQUAL(const_value(reader), "World");
      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_DECL);
      BOOST_CHECK_EQUAL(const_local_name(reader), "bar");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), true);

      // attributes can also be read at the close tag
      r = move_to_first_attribute(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ATTRIBUTE_NODE);
      BOOST_CHECK_EQUAL(const_local_name(reader), "id");
      BOOST_CHECK_EQUAL(const_value(reader), "1");
      BOOST_CHECK_EQUAL(has_value(reader), true);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);

      r = read(reader);
      BOOST_CHECK(r == 1);
      BOOST_CHECK_EQUAL(node_type(reader), XML_ELEMENT_DECL);
      BOOST_CHECK_EQUAL(const_local_name(reader), "root");
      BOOST_CHECK_EQUAL(has_value(reader), false);
      BOOST_CHECK_EQUAL(has_attributes(reader), false);
      r = read(reader);
      BOOST_CHECK(r == 0);
    }

    BOOST_AUTO_TEST_CASE(relaxng)
    {
      using namespace text_reader;
      const char schema_s[] =
R"(<?xml version='1.0' encoding='UTF-8'?>
<grammar datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'
         xmlns='http://relaxng.org/ns/structure/1.0'>
         <!-- declaring a default namespace: -->
         <!-- ns='http://example.org/foo' -->
  <start>
    <element name='root'>
      <ref name='root'/>
    </element>
  </start>

  <define name='root'>
    <element name='foo'>
      <data type='string'/>
    </element>
    <oneOrMore>
      <element name='bar'>
        <data type='string'/>
      </element>
    </oneOrMore>
  </define>

</grammar>
)";
      //xmlRelaxNGInitTypes();
      xxxml::relaxng::Parser_Ctxt_Ptr pc = xxxml::relaxng::new_mem_parser_ctxt(schema_s);
      xxxml::relaxng::Ptr schema = xxxml::relaxng::parse(pc);

      Ptr reader = for_memory(
          "<root><foo>Hello</foo><bar>World</bar></root>");
      relaxng_set_schema(reader, schema);
      unsigned i = 0;
      while (int r = read(reader)) {
        (void)r;
        ++i;
      }
      BOOST_CHECK_EQUAL(i, 8u);
    }

    BOOST_AUTO_TEST_CASE(relaxng_invalid)
    {
      using namespace text_reader;
      const char *schema_s = rng_schema_s;
      //xmlRelaxNGInitTypes();
      xxxml::relaxng::Parser_Ctxt_Ptr pc = xxxml::relaxng::new_mem_parser_ctxt(schema_s);
      xxxml::relaxng::Ptr schema = xxxml::relaxng::parse(pc);

      Ptr reader = for_memory(
          "<root><foo>Hello</foo><bar id='1'>World</bar></root>");
      relaxng_set_schema(reader, schema);
      // this property just enables validation with a DTD
      // set_parser_prop(reader, XML_PARSER_VALIDATE, 1);
      unsigned i = 0;
      vector<bool> valids;
      while (int r = read(reader)) {
        (void)r;
        ++i;
        // first invalid status is observed when reader returns
        // the start of the bar tag
        valids.push_back(is_valid(reader));
      }
      BOOST_CHECK_EQUAL(i, 8u);
      BOOST_CHECK_EQUAL(is_valid(reader), false);
      const array <bool, 8> valid_refs = {
        true, true, true, true,
        false, false, false, false };
      BOOST_CHECK_EQUAL_COLLECTIONS(valids.begin(), valids.end(),
          valid_refs.begin(), valid_refs.end());
    }

    struct Proxy {
      string message;
    };
    void local_generic_error_fn(void *ctx, const char * msg, ...)
    {
      Proxy *p = static_cast<Proxy*>(ctx);
      char array[4096] = {0};
      va_list ap;
      va_start(ap, msg);
      vsnprintf(array, sizeof(array), msg, ap);
      va_end(ap);
      p->message = string(array, array+sizeof(array)-1);
    }

    BOOST_AUTO_TEST_CASE(relaxng_invalid_with_reporting)
    {
      using namespace text_reader;
      const char *schema_s = rng_schema_s;
      //xmlRelaxNGInitTypes();
      xxxml::relaxng::Parser_Ctxt_Ptr pc = xxxml::relaxng::new_mem_parser_ctxt(schema_s);
      xxxml::relaxng::Ptr schema = xxxml::relaxng::parse(pc);
      xxxml::relaxng::Valid_Ctxt_Ptr validate =
        xxxml::relaxng::new_valid_ctxt(schema);
      Proxy p;
      xxxml::relaxng::set_valid_errors(validate,
          local_generic_error_fn, // errors
          local_generic_error_fn, // warnings
          &p);

      Ptr reader = for_memory(
          "<root><foo>Hello</foo><bar id='1'>World</bar></root>");
      relaxng_validate_ctxt(reader, validate);
      // this property just enables validation with a DTD
      // set_parser_prop(reader, XML_PARSER_VALIDATE, 1);
      unsigned i = 0;
      vector<bool> valids;
      while (int r = read(reader)) {
        (void)r;
        ++i;
        // first invalid status is observed when reader returns
        // the start of the bar tag
        valids.push_back(is_valid(reader));
      }
      BOOST_CHECK_EQUAL(i, 8u);
      BOOST_CHECK_EQUAL(is_valid(reader), false);
      BOOST_CHECK(p.message.find("Invalid attribute id for element bar")
          != string::npos);
      const array <bool, 8> valid_refs = {
        true, true, true, true,
        false, false, false, false };
      BOOST_CHECK_EQUAL_COLLECTIONS(valids.begin(), valids.end(),
          valid_refs.begin(), valid_refs.end());
    }

    // XXX add XML_PARSER_SUBST_ENTITIES case

  //}}}
  BOOST_AUTO_TEST_SUITE_END() // reader

  BOOST_AUTO_TEST_SUITE(writer)

    using namespace xxxml::text_writer;

    BOOST_AUTO_TEST_CASE(basic)
    {
      Output_Buffer_Ptr o = alloc_output_buffer();
      auto raw_o = o.get();
      Ptr w = new_text_writer(std::move(o));
      start_document(w);
      start_element(w, "root");
      start_element(w, "records");
      start_element(w, "record");
      write_element(w, "a", "Hello");
      write_element(w, "b", "23");
      end_element(w);
      start_element(w, "record");
      write_element(w, "a", "World");
      write_element(w, "b", "42");
      end_element(w);
      end_element(w);
      end_element(w);
      end_document(w);
      flush(w);

      const char *begin = (char*)xmlBufContent(raw_o->buffer);
      const char *end = (char*)xmlBufEnd(raw_o->buffer);
      string s(begin, end);
      //cout << s << '\n';
      BOOST_CHECK(s.find("<a>World</a>") != s.npos);
    }

    BOOST_AUTO_TEST_CASE(indent)
    {
      Output_Buffer_Ptr o = alloc_output_buffer();
      auto raw_o = o.get();
      Ptr w = new_text_writer(std::move(o));
      set_indent(w, true);
      start_document(w);
      start_element(w, "root");
      start_element(w, "records");
      start_element(w, "record");
      write_element(w, "a", "Hello");
      write_element(w, "b", "23");
      end_element(w);
      start_element(w, "record");
      write_element(w, "a", "World");
      write_element(w, "b", "42");
      end_element(w);
      end_element(w);
      end_element(w);
      end_document(w);
      flush(w);

      const char *begin = (char*)xmlBufContent(raw_o->buffer);
      const char *end = (char*)xmlBufEnd(raw_o->buffer);
      string s(begin, end);
      //cout << s << '\n';
      BOOST_CHECK(s.find("   <a>World</a>") != s.npos);
    }

    BOOST_AUTO_TEST_CASE(omit_end_elements)
    {
      Output_Buffer_Ptr o = alloc_output_buffer();
      auto raw_o = o.get();
      Ptr w = new_text_writer(std::move(o));
      set_indent(w, true);
      start_document(w);
      start_element(w, "root");
      start_element(w, "records");
      start_element(w, "record");
      write_element(w, "a", "Hello");
      write_element(w, "b", "23");
      end_element(w);
      start_element(w, "record");
      write_element(w, "a", "World");
      write_element(w, "b", "42");
      //end_element(w);
      //end_element(w);
      //end_element(w);
      end_document(w);
      flush(w);

      const char *begin = (char*)xmlBufContent(raw_o->buffer);
      const char *end = (char*)xmlBufEnd(raw_o->buffer);
      string s(begin, end);
      //cout << s << '\n';
      BOOST_CHECK(s.find("   <a>World</a>") != s.npos);
      BOOST_CHECK(s.find("  </record>") != s.npos);
      BOOST_CHECK(s.find(" </records>") != s.npos);
      BOOST_CHECK(s.find("</root>") != s.npos);
    }

    BOOST_AUTO_TEST_CASE(lets_throw_or_not)
    {
      Output_Buffer_Ptr o = alloc_output_buffer();
      auto raw_o = o.get();
      Ptr w = new_text_writer(std::move(o));
      set_indent(w, true);
      start_document(w);
      // well, the below makes the document invalid, thus,
      // could easily return an error, but it does not
      BOOST_CHECK_NO_THROW(start_document(w));

      const char *begin = (char*)xmlBufContent(raw_o->buffer);
      const char *end = (char*)xmlBufEnd(raw_o->buffer);
      string s(begin, end);
      //cout << s << '\n';
    }

    BOOST_AUTO_TEST_CASE(lets_throw)
    {
      Output_Buffer_Ptr o = alloc_output_buffer();
      Ptr w = new_text_writer(std::move(o));
      set_indent(w, true);
      start_document(w);
      start_element(w, "root");
      BOOST_CHECK_THROW(start_document(w), xxxml::Runtime_Error);
    }
    
  BOOST_AUTO_TEST_SUITE_END() // writer

BOOST_AUTO_TEST_SUITE_END()
