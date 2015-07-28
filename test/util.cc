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

  BOOST_AUTO_TEST_SUITE_END() // util_

BOOST_AUTO_TEST_SUITE_END() // libxxxml
