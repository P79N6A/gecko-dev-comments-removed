





#ifndef js_UbiNodeTraverse_h
#define js_UbiNodeTraverse_h

#include "js/UbiNode.h"
#include "js/Utility.h"
#include "js/Vector.h"

namespace JS {
namespace ubi {





























































template<typename Handler>
struct BreadthFirst {

    
    
    
    
    
    
    BreadthFirst(JSContext* cx, Handler& handler, const JS::AutoCheckCannotGC& noGC)
      : wantNames(true), cx(cx), visited(cx), handler(handler), pending(cx),
        traversalBegun(false), stopRequested(false), abandonRequested(false)
    { }

    
    bool init() { return visited.init(); }

    
    
    bool addStart(Node node) { return pending.append(node); }

    
    
    bool addStartVisited(Node node) {
        typename NodeMap::AddPtr ptr = visited.lookupForAdd(node);
        if (!ptr && !visited.add(ptr, node, typename Handler::NodeData()))
            return false;
        return addStart(node);
    }

    
    
    bool wantNames;

    
    
    
    
    
    
    
    bool traverse()
    {
        MOZ_ASSERT(!traversalBegun);
        traversalBegun = true;

        
        while (!pending.empty()) {
            Node origin = pending.front();
            pending.popFront();

            
            js::ScopedJSDeletePtr<EdgeRange> range(origin.edges(cx, wantNames));
            if (!range)
                return false;

            
            for (; !range->empty(); range->popFront()) {
                MOZ_ASSERT(!stopRequested);

                const Edge& edge = range->front();
                typename NodeMap::AddPtr a = visited.lookupForAdd(edge.referent);
                bool first = !a;

                if (first) {
                    
                    
                    if (!visited.add(a, edge.referent, typename Handler::NodeData()))
                        return false;
                }

                MOZ_ASSERT(a);

                
                if (!handler(*this, origin, edge, &a->value(), first))
                    return false;

                if (stopRequested)
                    return true;

                
                
                if (abandonRequested) {
                    
                    abandonRequested = false;
                } else if (first) {
                    if (!pending.append(edge.referent))
                        return false;
                }
            }
        }

        return true;
    }

    
    
    
    
    
    void stop() { stopRequested = true; }

    
    
    
    void abandonReferent() { abandonRequested = true; }

    
    JSContext* cx;

    
    
    
    typedef js::HashMap<Node, typename Handler::NodeData> NodeMap;
    NodeMap visited;

  private:
    
    Handler& handler;

    
    
    
    template <typename T>
    class Queue {
        js::Vector<T, 0> head, tail;
        size_t frontIndex;
      public:
        explicit Queue(JSContext* cx) : head(cx), tail(cx), frontIndex(0) { }
        bool empty() { return frontIndex >= head.length(); }
        T& front() {
            MOZ_ASSERT(!empty());
            return head[frontIndex];
        }
        void popFront() {
            MOZ_ASSERT(!empty());
            frontIndex++;
            if (frontIndex >= head.length()) {
                head.clearAndFree();
                head.swap(tail);
                frontIndex = 0;
            }
        }
        bool append(const T& elt) {
            return frontIndex == 0 ? head.append(elt) : tail.append(elt);
        }
    };

    
    
    
    Queue<Node> pending;

    
    bool traversalBegun;

    
    bool stopRequested;

    
    bool abandonRequested;
};

} 
} 

#endif 
