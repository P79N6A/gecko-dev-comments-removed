





#ifndef gc_findsccs_h___
#define gc_findsccs_h___

#include "jsutil.h"

namespace js {
namespace gc {

template<class Node>
struct GraphNodeBase
{
    Node           *gcNextGraphNode;
    Node           *gcNextGraphComponent;
    unsigned       gcDiscoveryTime;
    unsigned       gcLowLink;

    GraphNodeBase()
      : gcNextGraphNode(NULL),
        gcNextGraphComponent(NULL),
        gcDiscoveryTime(0),
        gcLowLink(0) {}

    ~GraphNodeBase() {}

    Node *nextNodeInGroup() const {
        if (gcNextGraphNode && gcNextGraphNode->gcNextGraphComponent == gcNextGraphComponent)
            return gcNextGraphNode;
        return NULL;
    }

    Node *nextGroup() const {
        return gcNextGraphComponent;
    }
};





















template<class Node>
class ComponentFinder
{
  public:
    ComponentFinder(uintptr_t stackLimit);
    ~ComponentFinder();

    
    void useOneComponent() { stackFull = true; }

    void addNode(Node *v);
    Node *getResultsList();

    static void mergeGroups(Node *first);

  public:
    
    void addEdgeTo(Node *w);

  private:
    
    static const unsigned Undefined = 0;

    
    static const unsigned Finished = (unsigned)-1;

    void processNode(Node *v);

  private:
    unsigned       clock;
    Node           *stack;
    Node           *firstComponent;
    Node           *cur;
    uintptr_t      stackLimit;
    bool           stackFull;
};

} 
} 

#endif 
