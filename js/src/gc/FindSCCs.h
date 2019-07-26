





#ifndef gc_findsccs_h___
#define gc_findsccs_h___

#include "jsutil.h"

namespace js {
namespace gc {

class ComponentFinder;

struct GraphNodeBase {
    GraphNodeBase  *gcNextGraphNode;
    unsigned       gcDiscoveryTime;
    unsigned       gcLowLink;

    GraphNodeBase()
      : gcNextGraphNode(NULL),
        gcDiscoveryTime(0),
        gcLowLink(0) {}

    virtual ~GraphNodeBase() {}
    virtual void findOutgoingEdges(ComponentFinder& finder) = 0;
};

template <class T> static T *
NextGraphNode(const T *current)
{
    const GraphNodeBase *node = current;
    return static_cast<T *>(node->gcNextGraphNode);
}

template <class T> void
AddGraphNode(T *&listHead, T *newFirstNode)
{
    GraphNodeBase *node = newFirstNode;
    JS_ASSERT(!node->gcNextGraphNode);
    node->gcNextGraphNode = listHead;
    listHead = newFirstNode;
}

template <class T> static T *
RemoveGraphNode(T *&listHead)
{
    GraphNodeBase *node = listHead;
    if (!node)
        return NULL;

    T *result = listHead;
    listHead = static_cast<T *>(node->gcNextGraphNode);
    node->gcNextGraphNode = NULL;
    return result;
}


















class ComponentFinder
{
  public:
    ComponentFinder(uintptr_t stackLimit);
    ~ComponentFinder();
    void addNode(GraphNodeBase *v);
    GraphNodeBase *getResultsList();

    template <class T> static T *
    getNextGroup(T *&resultsList) {
        T *group = resultsList;
        if (resultsList)
            resultsList = static_cast<T *>(removeFirstGroup(resultsList));
        return group;
    }

    template <class T> static T *
    getAllRemaining(T *&resultsList) {
        T *all = resultsList;
        removeAllRemaining(resultsList);
        resultsList = NULL;
        return all;
    }

  private:
    static GraphNodeBase *removeFirstGroup(GraphNodeBase *resultsList);
    static void removeAllRemaining(GraphNodeBase *resultsList);

  public:
    
    void addEdgeTo(GraphNodeBase *w);

  private:
    
    static const unsigned Undefined = 0;

    
    static const unsigned Finished = (unsigned)-1;

    void processNode(GraphNodeBase *v);
    void checkStackFull();

  private:
    unsigned       clock;
    GraphNodeBase  *stack;
    GraphNodeBase  *firstComponent;
    GraphNodeBase  *cur;
    uintptr_t      stackLimit;
    bool           stackFull;
};

} 
} 

#endif 
