#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE xfsx

#include <boost/test/unit_test.hpp>

#include <xxxml/xxxml.hh>


using xxxml_Library = xxxml::Library;

BOOST_GLOBAL_FIXTURE(xxxml_Library);
