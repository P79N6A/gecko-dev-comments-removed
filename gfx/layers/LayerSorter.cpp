




































#include "LayerSorter.h"
#include "DirectedGraph.h"
#include "limits.h"
#include "gfxLineSegment.h"

namespace mozilla {
namespace layers {

enum LayerSortOrder {
  Undefined,
  ABeforeB,
  BBeforeA,
};









static gfxFloat RecoverZDepth(const gfx3DMatrix& aTransform, const gfxPoint& aPoint)
{
    const gfxPoint3D l(0, 0, 1);
    gfxPoint3D l0 = gfxPoint3D(aPoint.x, aPoint.y, 0);
    gfxPoint3D p0 = aTransform.Transform3D(gfxPoint3D(0, 0, 0));
    gfxPoint3D normal = aTransform.GetNormalVector();

    gfxFloat n = normal.DotProduct(p0 - l0); 
    gfxFloat d = normal.DotProduct(l);

    if (!d) {
        return 0;
    }

    return n/d;
}


















static LayerSortOrder CompareDepth(Layer* aOne, Layer* aTwo) {
  gfxRect ourRect = aOne->GetEffectiveVisibleRegion().GetBounds();
  gfxRect otherRect = aTwo->GetEffectiveVisibleRegion().GetBounds();

  gfx3DMatrix ourTransform = aOne->GetTransform();
  gfx3DMatrix otherTransform = aTwo->GetTransform();

  
  gfxQuad ourTransformedRect = ourTransform.TransformRect(ourRect);
  gfxQuad otherTransformedRect = otherTransform.TransformRect(otherRect);

  gfxRect ourBounds = ourTransformedRect.GetBounds();
  gfxRect otherBounds = otherTransformedRect.GetBounds();

  if (!ourBounds.Intersects(otherBounds)) {
    return Undefined;
  }

  
  
  
  
  nsTArray<gfxPoint> points;
  for (PRUint32 i = 0; i < 4; i++) {
    if (ourTransformedRect.Contains(otherTransformedRect.mPoints[i])) {
      points.AppendElement(otherTransformedRect.mPoints[i]);
    }
    if (otherTransformedRect.Contains(ourTransformedRect.mPoints[i])) {
      points.AppendElement(ourTransformedRect.mPoints[i]);
    }
  }
  
  
  
  for (PRUint32 i = 0; i < 4; i++) {
    for (PRUint32 j = 0; j < 4; j++) {
      gfxPoint intersection;
      gfxLineSegment one(ourTransformedRect.mPoints[i],
                         ourTransformedRect.mPoints[(i + 1) % 4]);
      gfxLineSegment two(otherTransformedRect.mPoints[j],
                         otherTransformedRect.mPoints[(j + 1) % 4]);
      if (one.Intersects(two, intersection)) {
        points.AppendElement(intersection);
      }
    }
  }

  
  if (points.IsEmpty()) {
    return Undefined;
  }

  
  gfxFloat highest = 0;
  for (PRUint32 i = 0; i < points.Length(); i++) {
    gfxFloat ourDepth = RecoverZDepth(ourTransform, points.ElementAt(i));
    gfxFloat otherDepth = RecoverZDepth(otherTransform, points.ElementAt(i));

    gfxFloat difference = otherDepth - ourDepth;

    if (fabs(difference) > fabs(highest)) {
      highest = difference;
    }
  }
  
  if (highest >= 0) {
    return ABeforeB;
  } else {
    return BBeforeA;
  }
}

#ifdef DEBUG
static bool gDumpLayerSortList = getenv("MOZ_DUMP_LAYER_SORT_LIST") != 0;

static void DumpLayerList(nsTArray<Layer*>& aLayers)
{
  for (PRUint32 i = 0; i < aLayers.Length(); i++) {
    fprintf(stderr, "%p, ", aLayers.ElementAt(i));
  }
  fprintf(stderr, "\n");
}

static void DumpEdgeList(DirectedGraph<Layer*>& aGraph)
{
  nsTArray<DirectedGraph<Layer*>::Edge> edges = aGraph.GetEdgeList();
  
  for (PRUint32 i = 0; i < edges.Length(); i++) {
    fprintf(stderr, "From: %p, To: %p\n", edges.ElementAt(i).mFrom, edges.ElementAt(i).mTo);
  }
}
#endif




#define MAX_SORTABLE_LAYERS 100

void SortLayersBy3DZOrder(nsTArray<Layer*>& aLayers)
{
  PRUint32 nodeCount = aLayers.Length();
  if (nodeCount > MAX_SORTABLE_LAYERS) {
    return;
  }
  DirectedGraph<Layer*> graph;

#ifdef DEBUG
  if (gDumpLayerSortList) {
    fprintf(stderr, " --- Layers before sorting: --- \n");
    DumpLayerList(aLayers);
  }
#endif

  
  for (PRUint32 i = 0; i < nodeCount; i++) {
    for (PRUint32 j = i + 1; j < nodeCount; j++) {
      Layer* a = aLayers.ElementAt(i);
      Layer* b = aLayers.ElementAt(j);
      LayerSortOrder order = CompareDepth(a, b);
      if (order == ABeforeB) {
        graph.AddEdge(a, b);
      } else if (order == BBeforeA) {
        graph.AddEdge(b, a);
      }
    }
  }

#ifdef DEBUG
  if (gDumpLayerSortList) {
    fprintf(stderr, " --- Edge List: --- \n");
    DumpEdgeList(graph);
  }
#endif

  
  nsTArray<Layer*> noIncoming;
  nsTArray<Layer*> sortedList;

  
  noIncoming.AppendElements(aLayers);
  const nsTArray<DirectedGraph<Layer*>::Edge>& edges = graph.GetEdgeList();
  for (PRUint32 i = 0; i < edges.Length(); i++) {
    noIncoming.RemoveElement(edges.ElementAt(i).mTo);
  }

  
  
  do {
    if (!noIncoming.IsEmpty()) {
      PRUint32 last = noIncoming.Length() - 1;

      Layer* layer = noIncoming.ElementAt(last);

      noIncoming.RemoveElementAt(last);
      sortedList.AppendElement(layer);

      nsTArray<DirectedGraph<Layer*>::Edge> outgoing;
      graph.GetEdgesFrom(layer, outgoing);
      for (PRUint32 i = 0; i < outgoing.Length(); i++) {
        DirectedGraph<Layer*>::Edge edge = outgoing.ElementAt(i);
        graph.RemoveEdge(edge);
        if (!graph.NumEdgesTo(edge.mTo)) {
          
          noIncoming.AppendElement(edge.mTo);
        }
      }
    }

    
    
    if (noIncoming.IsEmpty() && graph.GetEdgeCount()) {
      
      PRUint32 minEdges = UINT_MAX;
      Layer* minNode = nsnull;
      for (PRUint32 i = 0; i < aLayers.Length(); i++) {
        PRUint32 edgeCount = graph.NumEdgesTo(aLayers.ElementAt(i));
        if (edgeCount && edgeCount < minEdges) {
          minEdges = edgeCount;
          minNode = aLayers.ElementAt(i);
          if (minEdges == 1) {
            break;
          }
        }
      }

      
      graph.RemoveEdgesTo(minNode);
      noIncoming.AppendElement(minNode);
    }
  } while (!noIncoming.IsEmpty());
  NS_ASSERTION(!graph.GetEdgeCount(), "Cycles detected!");
#ifdef DEBUG
  if (gDumpLayerSortList) {
    fprintf(stderr, " --- Layers after sorting: --- \n");
    DumpLayerList(sortedList);
  }
#endif

  aLayers.Clear();
  aLayers.AppendElements(sortedList);
}

}
}
