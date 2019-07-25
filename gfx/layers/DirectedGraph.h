




































#ifndef GFX_DIRECTEDGRAPH_H
#define GFX_DIRECTEDGRAPH_H

#include "gfxTypes.h"
#include "nsTArray.h"

namespace mozilla {
namespace layers {

template <typename T>
class DirectedGraph {
public:

  class Edge {
    public:
    Edge(T aFrom, T aTo) : mFrom(aFrom), mTo(aTo) {}

    bool operator==(const Edge& aOther) const
    {
      return mFrom == aOther.mFrom && mTo == aOther.mTo;
    }

    T mFrom;
    T mTo;
  };

  class RemoveEdgesToComparator 
  {
  public:
    PRBool Equals(const Edge& a, T const& b) const { return a.mTo == b; }
  };

  


  void AddEdge(Edge aEdge)
  {
    NS_ASSERTION(!mEdges.Contains(aEdge), "Adding a duplicate edge!");
    mEdges.AppendElement(aEdge);
  }

  void AddEdge(T aFrom, T aTo)
  {
    AddEdge(Edge(aFrom, aTo));
  }

  


  const nsTArray<Edge>& GetEdgeList() const
  {
    return mEdges; 
  }

  


  void RemoveEdge(Edge aEdge)
  {
    mEdges.RemoveElement(aEdge);
  }

  


  void RemoveEdgesTo(T aNode)
  {
    RemoveEdgesToComparator c;
    while (mEdges.RemoveElement(aNode, c)) {}
  }
  
  


  unsigned int NumEdgesTo(T aNode)
  {
    unsigned int count = 0;
    for (unsigned int i = 0; i < mEdges.Length(); i++) {
      if (mEdges.ElementAt(i).mTo == aNode) {
        count++;
      }
    }
    return count;
  }

  


  void GetEdgesFrom(T aNode, nsTArray<Edge>& aResult)
  {
    for (unsigned int i = 0; i < mEdges.Length(); i++) {
      if (mEdges.ElementAt(i).mFrom == aNode) {
        aResult.AppendElement(mEdges.ElementAt(i));
      }
    }
  }

  


  unsigned int GetEdgeCount() { return mEdges.Length(); }

private:

  nsTArray<Edge> mEdges;
};

}
}

#endif 
