#include "util.hh"

#include <deque>


using namespace std;

namespace xxxml {

  namespace util {

    void remove(doc::Ptr &doc, const std::string &xpath)
    {
      // making sure that removed_nodes is destroyed after the
      // xpath::Object_Ptr, since those node are accessed during
      // the destruction of the xpath object
      deque<Node_Ptr> removed_nodes;
      {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(doc);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval(xpath, c);
        if (o.get()->type != XPATH_NODESET)
          throw std::runtime_error("xpath must produce a nodeset for removal");

        for (unsigned i = 0; i < unsigned(o.get()->nodesetval->nodeNr); ++i) {
          xmlNode *node = o.get()->nodesetval->nodeTab[i];
          removed_nodes.push_back(unlink_node(node));
        }
      }
    }

  }

}
