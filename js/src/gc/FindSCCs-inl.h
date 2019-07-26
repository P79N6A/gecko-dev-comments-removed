





#include "jsfriendapi.h"

#include "gc/FindSCCs.h"

namespace js {
namespace gc {

template<class Node>
ComponentFinder<Node>::ComponentFinder(uintptr_t sl)
  : clock(1),
    stack(NULL),
    firstComponent(NULL),
    cur(NULL),
    stackLimit(sl),
    stackFull(false)
{
}

template<class Node>
ComponentFinder<Node>::~ComponentFinder()
{
    JS_ASSERT(!stack);
    JS_ASSERT(!firstComponent);
}

template<class Node>
void
ComponentFinder<Node>::addNode(Node *v)
{
    if (v->gcDiscoveryTime == Undefined) {
        JS_ASSERT(v->gcLowLink == Undefined);
        processNode(v);
    }
}

template<class Node>
void
ComponentFinder<Node>::processNode(Node *v)
{
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

template<class Node>
void
ComponentFinder<Node>::addEdgeTo(Node *w)
{
    if (w->gcDiscoveryTime == Undefined) {
        processNode(w);
        cur->gcLowLink = Min(cur->gcLowLink, w->gcLowLink);
    } else if (w->gcDiscoveryTime != Finished) {
        cur->gcLowLink = Min(cur->gcLowLink, w->gcDiscoveryTime);
    }
}

template<class Node>
Node *
ComponentFinder<Node>::getResultsList()
{
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
    firstComponent = NULL;

    for (Node *v = result; v; v = v->gcNextGraphNode) {
        v->gcDiscoveryTime = Undefined;
        v->gcLowLink = Undefined;
    }

    return result;
}

template<class Node>
 void
ComponentFinder<Node>::mergeGroups(Node *first)
{
    for (Node *v = first; v; v = v->gcNextGraphNode)
        v->gcNextGraphComponent = NULL;
}

} 
} 
