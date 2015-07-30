#include <boost/test/unit_test.hpp>

#include <xxxml/util.hh>

using namespace std;

BOOST_AUTO_TEST_SUITE(libxxxml)

  BOOST_AUTO_TEST_SUITE(util_)

    using namespace xxxml;
    using namespace xxxml::util;

    BOOST_AUTO_TEST_CASE(remove_)
    {
      doc::Ptr d = read_memory("<root><foo><foo>Hello</foo></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      remove(d, "//foo");
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
    }

    BOOST_AUTO_TEST_CASE(replace_)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      replace(d, "//fubar", "^H(.*)$", "h\\1 world");
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(d);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval("//fubar/text()", c);
        BOOST_REQUIRE(o.get()->type == XPATH_NODESET);
        BOOST_REQUIRE(o.get()->nodesetval->nodeNr);
        BOOST_CHECK_EQUAL(xxxml::content(o.get()->nodesetval->nodeTab[0]), "hello world");
      }
    }

    BOOST_AUTO_TEST_CASE(get_child_)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode *root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      BOOST_CHECK(optional_get_child(root, "bar"));
      BOOST_CHECK_EQUAL(name(optional_get_child(root, "bar")), "bar");
      BOOST_CHECK(!optional_get_child(root, "baz"));
    }

    BOOST_AUTO_TEST_CASE(add_)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      add(d, "//root", "foo/element", "blah");
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(d);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval("/root/foo/element/text()", c);
        BOOST_REQUIRE(o.get()->type == XPATH_NODESET);
        BOOST_REQUIRE(o.get()->nodesetval);
        BOOST_CHECK_EQUAL(xxxml::content(o.get()->nodesetval->nodeTab[0]), "blah");
      }
    }

    BOOST_AUTO_TEST_CASE(add_sibling)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      add(d, "//root", "+foo/element", "blah");
      BOOST_CHECK_EQUAL(child_element_count(root), 3u);
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(d);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval("/root/foo[2]/element/text()", c);
        BOOST_REQUIRE(o.get()->type == XPATH_NODESET);
        BOOST_REQUIRE(o.get()->nodesetval);
        BOOST_CHECK_EQUAL(xxxml::content(o.get()->nodesetval->nodeTab[0]), "blah");
      }
    }

    BOOST_AUTO_TEST_CASE(add_append)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      add(d, "//root", "foo/fubar", " World", false);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(d);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval("/root/foo/fubar/text()", c);
        BOOST_REQUIRE(o.get()->type == XPATH_NODESET);
        BOOST_REQUIRE(o.get()->nodesetval);
        BOOST_CHECK_EQUAL(xxxml::content(o.get()->nodesetval->nodeTab[0]), "Hello World");
      }
    }

    BOOST_AUTO_TEST_CASE(empty_node_set)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      Node_Set node_set(d, "/root/foo[2]/element/text()");
      BOOST_CHECK(node_set.begin() == node_set.end());
      unsigned i = 0;
      for (auto node : node_set) {
        (void)node;
        ++i;
      }
      BOOST_CHECK_EQUAL(i, 0u);
    }

    BOOST_AUTO_TEST_CASE(add_attributes)
    {
      doc::Ptr d = read_memory("<root><foo><bar xyz='a'>Hello</bar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      set_attribute(d, "//bar", "xyz", "23");
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar xyz=\"23\">Hello</bar></foo>"
          "<bar xyz=\"23\">World</bar></root>\n");
    }

    BOOST_AUTO_TEST_CASE(add_snippet_after)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      add_snippet(d, "//foo", inp, inp + sizeof(inp) - 1, 2);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>Hello</bar></foo>"
          "<bar>World</bar></root>\n");
    }

    BOOST_AUTO_TEST_CASE(add_snippet_before)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      add_snippet(d, "//foo", inp, inp + sizeof(inp) - 1, -2);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><bar>World</bar><foo><bar>Hello</bar></foo>"
          "</root>\n");
    }

    BOOST_AUTO_TEST_CASE(add_snippet_first_child)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      add_snippet(d, "//foo", inp, inp + sizeof(inp) - 1, 1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>World</bar><bar>Hello</bar></foo>"
          "</root>\n");
    }
    BOOST_AUTO_TEST_CASE(add_snippet_first_child_no_sibling)
    {
      doc::Ptr d = read_memory("<root><foo></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      add_snippet(d, "//foo", inp, inp + sizeof(inp) - 1, 1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>World</bar></foo>"
          "</root>\n");
    }
    BOOST_AUTO_TEST_CASE(add_snippet_last_child)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      add_snippet(d, "//foo", inp, inp + sizeof(inp) - 1, -1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>Hello</bar><bar>World</bar></foo>"
          "</root>\n");
    }

  BOOST_AUTO_TEST_SUITE_END() // util_

BOOST_AUTO_TEST_SUITE_END() // libxxxml
