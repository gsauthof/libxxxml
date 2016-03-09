#include <boost/test/unit_test.hpp>

#include <xxxml/util.hh>

#include <boost/algorithm/string/erase.hpp>

#include <sstream>
#include <iostream>

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

    BOOST_AUTO_TEST_CASE(add_overwrite)
    {
      doc::Ptr d = read_memory("<root><foo><fubar>Hello</fubar></foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      add(d, "//root", "foo/fubar", "World");
      BOOST_CHECK_EQUAL(child_element_count(root), 2u);
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(d);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval("/root/foo/fubar/text()", c);
        BOOST_REQUIRE(o.get()->type == XPATH_NODESET);
        BOOST_REQUIRE(o.get()->nodesetval);
        BOOST_CHECK_EQUAL(xxxml::content(o.get()->nodesetval->nodeTab[0]), "World");
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

    BOOST_AUTO_TEST_CASE(insert_after)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      insert(d, "//foo", inp, inp + sizeof(inp) - 1, 2);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>Hello</bar></foo>"
          "<bar>World</bar></root>\n");
    }

    BOOST_AUTO_TEST_CASE(insert_before)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      insert(d, "//foo", inp, inp + sizeof(inp) - 1, -2);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><bar>World</bar><foo><bar>Hello</bar></foo>"
          "</root>\n");
    }

    BOOST_AUTO_TEST_CASE(insert_first_child)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      insert(d, "//foo", inp, inp + sizeof(inp) - 1, 1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>World</bar><bar>Hello</bar></foo>"
          "</root>\n");
    }
    BOOST_AUTO_TEST_CASE(insert_first_child_no_sibling)
    {
      doc::Ptr d = read_memory("<root><foo></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      insert(d, "//foo", inp, inp + sizeof(inp) - 1, 1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>World</bar></foo>"
          "</root>\n");
    }
    BOOST_AUTO_TEST_CASE(insert_last_child)
    {
      doc::Ptr d = read_memory("<root><foo><bar>Hello</bar></foo></root>");
      BOOST_REQUIRE(d.get());
      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(child_element_count(root), 1u);
      const char inp[] = "<bar>World</bar>";
      insert(d, "//foo", inp, inp + sizeof(inp) - 1, -1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>Hello</bar><bar>World</bar></foo>"
          "</root>\n");
    }
    BOOST_AUTO_TEST_CASE(insert_into_empty_doc)
    {
      doc::Ptr d = new_doc();
      BOOST_REQUIRE(d.get());
      const char inp[] = "<root><foo><bar>Hello</bar><bar>World</bar></foo></root>";
      insert(d, nullptr, inp, inp + sizeof(inp) - 1, -1);
      auto r = doc::dump_format_memory(d, false);
      BOOST_CHECK_EQUAL(string(r.first.get(), r.second), "<?xml version=\"1.0\"?>\n"
          "<root><foo><bar>Hello</bar><bar>World</bar></foo>"
          "</root>\n");
    }

    BOOST_AUTO_TEST_CASE(get_string)
    {
      doc::Ptr d = read_memory(
          "<root><foo><bar>Hello</bar></foo><xyz>World</xyz></root>");
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "//bar"), "Hello");
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "string(//bar)"),
          "Hello");
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "//bar/text()"), "Hello");
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "string(//bar/text())"),
          "Hello");
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "//xyz"), "World");
    }

    BOOST_AUTO_TEST_CASE(get_string_empty)
    {
      doc::Ptr d = read_memory(
          "<root><foo><bar>Hello</bar></foo><xyz>World</xyz></root>");
      BOOST_CHECK_THROW(xxxml::util::xpath::get_string(d, "//baz"),
          std::underflow_error);
      BOOST_CHECK_THROW(xxxml::util::xpath::get_string(d, "//baz"),
          std::runtime_error);
      BOOST_CHECK_EQUAL(xxxml::util::xpath::get_string(d, "string(//baz)"), "");
    }

    BOOST_AUTO_TEST_CASE(dump)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar>World</bar></root>");
      BOOST_REQUIRE(d.get());

      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");

      auto r = xxxml::util::dump(d, root);

      string s(r.first.first, r.first.second);
      BOOST_CHECK(s.find("<bar>World</bar>") != s.npos);
      BOOST_CHECK(s.find("<root>") != s.npos);
    }

    BOOST_AUTO_TEST_CASE(dump_one)
    {
      doc::Ptr d = read_memory("<root><foo>Hello</foo><bar><a>Wo</a><b>rld</b></bar></root>");
      BOOST_REQUIRE(d.get());

      const xmlNode* root = doc::get_root_element(d);
      BOOST_CHECK_EQUAL(name(root), "root");

      auto r = xxxml::util::dump(d, xxxml::last_element_child(root));

      string s(r.first.first, r.first.second);
      boost::erase_all(s, " ");
      boost::erase_all(s, "\n");
      BOOST_CHECK_EQUAL(s,  "<bar><a>Wo</a><b>rld</b></bar>");
    }

    BOOST_AUTO_TEST_SUITE(df_traverser_)

      BOOST_AUTO_TEST_CASE(basic)
      {
        doc::Ptr d = read_memory("<root><foo>Hello</foo><bar><a>Wo</a><b>rld</b></bar></root>");
        BOOST_REQUIRE(d.get());
        xxxml::util::DF_Traverser t(d);
        ostringstream o;
        while (!t.eot()) {
          o << xxxml::name(*t) << ' ';
          t.advance();
        }
        BOOST_CHECK_EQUAL(o.str(), "root foo bar a b ");
      }

      BOOST_AUTO_TEST_CASE(skip)
      {
        doc::Ptr d = read_memory("<root><foo>Hello</foo><bar><a>Wo</a><b>rld</b></bar><baz>!</baz></root>");
        BOOST_REQUIRE(d.get());
        xxxml::util::DF_Traverser t(d);
        ostringstream o;
        while (!t.eot()) {
          o << xxxml::name(*t) << ' ';
          if (strcmp(xxxml::name(*t), "bar"))
            t.advance();
          else
            t.skip_children();
        }
        BOOST_CHECK_EQUAL(o.str(), "root foo bar baz ");
      }

      BOOST_AUTO_TEST_CASE(height)
      {
        doc::Ptr d = read_memory("<root><foo>Hello</foo><bar><a>Wo</a><b>rld</b></bar><baz>23</baz></root>");
        BOOST_REQUIRE(d.get());
        xxxml::util::DF_Traverser t(d);
        ostringstream o;
        while (!t.eot()) {
          o << xxxml::name(*t) << ' ' << t.height() << ' ';
          t.advance();
        }
        BOOST_CHECK_EQUAL(o.str(), "root 0 foo 1 bar 1 a 2 b 2 baz 1 ");
      }

      BOOST_AUTO_TEST_CASE(related)
      {
        doc::Ptr d = read_memory("<root><foo>Hello</foo><bar><a>Wo</a><b>rld</b></bar><baz>23</baz></root>");
        BOOST_REQUIRE(d.get());
        xxxml::util::DF_Traverser t(d);
        BOOST_REQUIRE((*t)->parent != nullptr);
        BOOST_CHECK((*t)->parent->parent == nullptr);
        BOOST_CHECK(next_element_sibling((*t)->parent) == nullptr);
      }

    BOOST_AUTO_TEST_SUITE_END() // df_traverser_

  BOOST_AUTO_TEST_SUITE_END() // util_

BOOST_AUTO_TEST_SUITE_END() // libxxxml
