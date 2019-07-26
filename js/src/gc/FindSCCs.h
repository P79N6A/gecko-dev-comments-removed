





#ifndef gc_FindSCCs_h
#define gc_FindSCCs_h

#include "jsfriendapi.h"
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
      : gcNextGraphNode(nullptr),
        gcNextGraphComponent(nullptr),
        gcDiscoveryTime(0),
        gcLowLink(0) {}

    ~GraphNodeBase() {}

    Node *nextNodeInGroup() const {
        if (gcNextGraphNode && gcNextGraphNode->gcNextGraphComponent == gcNextGraphComponent)
            return gcNextGraphNode;
        return nullptr;
    }

    Node *nextGroup() const {
        return gcNextGraphComponent;
    }
};





















template<class Node>
class ComponentFinder
{
  public:
    explicit ComponentFinder(uintptr_t sl)
      : clock(1),
        stack(nullptr),
        firstComponent(nullptr),
        cur(nullptr),
        stackLimit(sl),
        stackFull(false)
    {}

    ~ComponentFinder() {
        JS_ASSERT(!stack);
        JS_ASSERT(!firstComponent);
    }

    
    void useOneComponent() { stackFull = true; }

    void addNode(Node *v) {
        if (v->gcDiscoveryTime == Undefined) {
            JS_ASSERT(v->gcLowLink == Undefined);
            processNode(v);
        }
    }

    Node *getResultsList() {
        if (stackFull) {
            



            Node *firstGoodComponent = firstComponent;
            for (Node *v = stack; v; v = stack) {
                stack = v->gcNextGraphNode;
                v->gcNextGraphComponent = firstGoodComponent;
                v->gcNextGraphNode = firstComponent;
                firstComponent = v;
            }
            stackFull = false;
        }

        JS_ASSERT(!stack);

        Node *result = firstComponent;
        firstComponent = nullptr;

        for (Node *v = result; v; v = v->gcNextGraphNode) {
            v->gcDiscoveryTime = Undefined;
            v->gcLowLink = Undefined;
        }

        return result;
    }

    static void mergeGroups(Node *first) {
        for (Node *v = first; v; v = v->gcNextGraphNode)
            v->gcNextGraphComponent = nullptr;
    }

  public:
    
    void addEdgeTo(Node *w) {
        if (w->gcDiscoveryTime == Undefined) {
            processNode(w);
            cur->gcLowLink = Min(cur->gcLowLink, w->gcLowLink);
        } else if (w->gcDiscoveryTime != Finished) {
            cur->gcLowLink = Min(cur->gcLowLink, w->gcDiscoveryTime);
        }
    }

  private:
    
    static const unsigned Undefined = 0;

    
    static const unsigned Finished = (unsigned)-1;

    void processNode(Node *v) {
        v->gcDiscoveryTime = clock;
        v->gcLowLink = clock;
        ++clock;

        v->gcNextGraphNode = stack;
        stack = v;

        int stackDummy;
        if (stackFull || !JS_CHECK_STACK_SIZE(stackLimit, &stackDummy)) {
            stackFull = true;
            return;
        }

        Node *old = cur;
        cur = v;
        cur->findOutgoingEdges(*this);
        cur = old;

        if (stackFull)
            return;

        if (v->gcLowLink == v->gcDiscoveryTime) {
            Node *nextComponent = firstComponent;
            Node *w;
            do {
                JS_ASSERT(stack);
                w = stack;
                stack = w->gcNextGraphNode;

                



                w->gcDiscoveryTime = Finished;

                
                w->gcNextGraphComponent = nextComponent;

                



                w->gcNextGraphNode = firstComponent;
                firstComponent = w;
            } while (w != v);
        }
    }

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
