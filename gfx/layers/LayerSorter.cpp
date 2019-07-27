




#include "LayerSorter.h"
#include <math.h>                       
#include <stdint.h>                     
#include <stdio.h>                      
#include <stdlib.h>                     
#include "DirectedGraph.h"              
#include "Layers.h"                     
#include "gfx3DMatrix.h"                
#include "gfxLineSegment.h"             
#include "gfxPoint.h"                   
#include "gfxQuad.h"                    
#include "gfxRect.h"                    
#include "gfxTypes.h"                   
#include "mozilla/gfx/BasePoint3D.h"    
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "limits.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

enum LayerSortOrder {
  Undefined,
  ABeforeB,
  BBeforeA,
};









static gfxFloat RecoverZDepth(const gfx3DMatrix& aTransform, const gfxPoint& aPoint)
{
    const Point3D l(0, 0, 1);
    Point3D l0 = Point3D(aPoint.x, aPoint.y, 0);
    Point3D p0 = aTransform.Transform3D(Point3D(0, 0, 0));
    Point3D normal = aTransform.GetNormalVector();

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

  gfx3DMatrix ourTransform = To3DMatrix(aOne->GetTransform());
  gfx3DMatrix otherTransform = To3DMatrix(aTwo->GetTransform());

  
  gfxQuad ourTransformedRect = ourTransform.TransformRect(ourRect);
  gfxQuad otherTransformedRect = otherTransform.TransformRect(otherRect);

  gfxRect ourBounds = ourTransformedRect.GetBounds();
  gfxRect otherBounds = otherTransformedRect.GetBounds();

  if (!ourBounds.Intersects(otherBounds)) {
    return Undefined;
  }

  
  
  
  
  nsTArray<gfxPoint> points;
  for (uint32_t i = 0; i < 4; i++) {
    if (ourTransformedRect.Contains(otherTransformedRect.mPoints[i])) {
      points.AppendElement(otherTransformedRect.mPoints[i]);
    }
    if (otherTransformedRect.Contains(ourTransformedRect.mPoints[i])) {
      points.AppendElement(ourTransformedRect.mPoints[i]);
    }
  }
  
  
  
  for (uint32_t i = 0; i < 4; i++) {
    for (uint32_t j = 0; j < 4; j++) {
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
  for (uint32_t i = 0; i < points.Length(); i++) {
    gfxFloat ourDepth = RecoverZDepth(ourTransform, points.ElementAt(i));
    gfxFloat otherDepth = RecoverZDepth(otherTransform, points.ElementAt(i));

    gfxFloat difference = otherDepth - ourDepth;

    if (fabs(difference) > fabs(highest)) {
      highest = difference;
    }
  }
  
  if (fabs(highest) < 0.1 || highest >= 0) {
    return ABeforeB;
  } else {
    return BBeforeA;
  }
}

#ifdef DEBUG
static bool gDumpLayerSortList = getenv("MOZ_DUMP_LAYER_SORT_LIST") != 0;


#ifdef USE_XTERM_COLORING




static const int XTERM_FOREGROUND_COLOR_OFFSET = 30;
static const int XTERM_BACKGROUND_COLOR_OFFSET = 40;
static const int BLACK = 0;

static const int GREEN = 2;






static const int RESET = 0;







static void SetTextColor(uint32_t aColor)
{
  char command[13];

  
  sprintf(command, "%c[%d;%d;%dm", 0x1B, RESET,
          aColor + XTERM_FOREGROUND_COLOR_OFFSET,
          BLACK + XTERM_BACKGROUND_COLOR_OFFSET);
  printf("%s", command);
}

static void print_layer_internal(FILE* aFile, Layer* aLayer, uint32_t aColor)
{
  SetTextColor(aColor);
  fprintf(aFile, "%p", aLayer);
  SetTextColor(GREEN);
}
#else

const char *colors[] = { "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White" };

static void print_layer_internal(FILE* aFile, Layer* aLayer, uint32_t aColor)
{
  fprintf(aFile, "%p(%s)", aLayer, colors[aColor]);
}
#endif

static void print_layer(FILE* aFile, Layer* aLayer)
{
  print_layer_internal(aFile, aLayer, aLayer->GetDebugColorIndex());
}

static void DumpLayerList(nsTArray<Layer*>& aLayers)
{
  for (uint32_t i = 0; i < aLayers.Length(); i++) {
    print_layer(stderr, aLayers.ElementAt(i));
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\n");
}

static void DumpEdgeList(DirectedGraph<Layer*>& aGraph)
{
  const nsTArray<DirectedGraph<Layer*>::Edge>& edges = aGraph.GetEdgeList();
  
  for (uint32_t i = 0; i < edges.Length(); i++) {
    fprintf(stderr, "From: ");
    print_layer(stderr, edges.ElementAt(i).mFrom);
    fprintf(stderr, ", To: ");
    print_layer(stderr, edges.ElementAt(i).mTo);
    fprintf(stderr, "\n");
  }
}
#endif




#define MAX_SORTABLE_LAYERS 100


uint32_t gColorIndex = 1;

void SortLayersBy3DZOrder(nsTArray<Layer*>& aLayers)
{
  uint32_t nodeCount = aLayers.Length();
  if (nodeCount > MAX_SORTABLE_LAYERS) {
    return;
  }
  DirectedGraph<Layer*> graph;

#ifdef DEBUG
  if (gDumpLayerSortList) {
    for (uint32_t i = 0; i < nodeCount; i++) {
      if (aLayers.ElementAt(i)->GetDebugColorIndex() == 0) {
        aLayers.ElementAt(i)->SetDebugColorIndex(gColorIndex++);
        if (gColorIndex > 7) {
          gColorIndex = 1;
        }
      }
    }
    fprintf(stderr, " --- Layers before sorting: --- \n");
    DumpLayerList(aLayers);
  }
#endif

  
  for (uint32_t i = 0; i < nodeCount; i++) {
    for (uint32_t j = i + 1; j < nodeCount; j++) {
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
  for (uint32_t i = 0; i < edges.Length(); i++) {
    noIncoming.RemoveElement(edges.ElementAt(i).mTo);
  }

  
  
  do {
    if (!noIncoming.IsEmpty()) {
      uint32_t last = noIncoming.Length() - 1;

      Layer* layer = noIncoming.ElementAt(last);
      MOZ_ASSERT(layer); 

      noIncoming.RemoveElementAt(last);
      sortedList.AppendElement(layer);

      nsTArray<DirectedGraph<Layer*>::Edge> outgoing;
      graph.GetEdgesFrom(layer, outgoing);
      for (uint32_t i = 0; i < outgoing.Length(); i++) {
        DirectedGraph<Layer*>::Edge edge = outgoing.ElementAt(i);
        graph.RemoveEdge(edge);
        if (!graph.NumEdgesTo(edge.mTo)) {
          
          noIncoming.AppendElement(edge.mTo);
        }
      }
    }

    
    
    if (noIncoming.IsEmpty() && graph.GetEdgeCount()) {
      
      uint32_t minEdges = UINT_MAX;
      Layer* minNode = nullptr;
      for (uint32_t i = 0; i < aLayers.Length(); i++) {
        uint32_t edgeCount = graph.NumEdgesTo(aLayers.ElementAt(i));
        if (edgeCount && edgeCount < minEdges) {
          minEdges = edgeCount;
          minNode = aLayers.ElementAt(i);
          if (minEdges == 1) {
            break;
          }
        }
      }

      if (minNode) {
        
        graph.RemoveEdgesTo(minNode);
        noIncoming.AppendElement(minNode);
      }
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
