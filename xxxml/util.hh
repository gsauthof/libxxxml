#ifndef XXXML_UTIL_HH
#define XXXML_UTIL_HH

#include <string>
#include <xxxml/xxxml.hh>

namespace xxxml {

  namespace util {

    void remove(doc::Ptr &doc, const std::string &xpath);
    void replace(doc::Ptr &doc, const std::string &xpath,
        const std::string &regex, const std::string &subst);

  }


}



#endif
