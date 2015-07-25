#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE xfsx

#include <boost/test/unit_test.hpp>

#include <xxxml/xxxml.hh>

#include <libxml/xmlschemastypes.h>

struct Global_LXML_Fixture {

  Global_LXML_Fixture()
  {
    LIBXML_TEST_VERSION
    xmlInitParser();
  }
  ~Global_LXML_Fixture()
  {
    xmlCleanupParser();
    xmlSchemaCleanupTypes();
  }
};

BOOST_GLOBAL_FIXTURE(Global_LXML_Fixture);
