




#include "nsRegion.h"
#include "nsPrintfCString.h"
#include "nsTArray.h"
#include "gfx3DMatrix.h"
#include "gfxUtils.h"

bool nsRegion::Contains(const nsRegion& aRgn) const
{
  
  nsRegionRectIterator iter(aRgn);
  while (const nsRect* r = iter.Next()) {
    if (!Contains (*r)) {
      return false;
    }
  }
  return true;
}

bool nsRegion::Intersects(const nsRect& aRect) const
{
  
  nsRegionRectIterator iter(*this);
  while (const nsRect* r = iter.Next()) {
    if (r->Intersects(aRect)) {
      return true;
    }
  }
  return false;
}

void nsRegion::Inflate(const nsMargin& aMargin)
{
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect.Inflate(aMargin);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t region;
  
  pixman_region32_init_rects(&region, boxes, n);

  pixman_region32_fini(&mImpl);
  mImpl = region;
}

void nsRegion::SimplifyOutward (uint32_t aMaxRects)
{
  MOZ_ASSERT(aMaxRects >= 1, "Invalid max rect count");

  if (GetNumRects() <= aMaxRects)
    return;

  pixman_box32_t *boxes;
  int n;
  boxes = pixman_region32_rectangles(&mImpl, &n);

  
  int dest = 0;
  for (int src = 1; src < n; src++)
  {
    
    
    
    
    
    
    
    
    while ((src < (n)) && boxes[dest].y1 == boxes[src].y1) {
      
      boxes[dest].x2 = boxes[src].x2;
      src++;
    }
    if (src < n) {
      dest++;
      boxes[dest] = boxes[src];
    }
  }

  uint32_t reducedCount = dest+1;
  
  
  
  if (reducedCount > 1 && reducedCount <= aMaxRects) {
    
    
    mImpl.data->numRects = reducedCount;
  } else {
    *this = GetBounds();
  }
}





static uint32_t ComputeMergedAreaIncrease(pixman_box32_t *topRects,
		                     pixman_box32_t *topRectsEnd,
		                     pixman_box32_t *bottomRects,
		                     pixman_box32_t *bottomRectsEnd)
{
  uint32_t totalArea = 0;
  struct pt {
    int32_t x, y;
  };


  pt *i = (pt*)topRects;
  pt *end_i = (pt*)topRectsEnd;
  pt *j = (pt*)bottomRects;
  pt *end_j = (pt*)bottomRectsEnd;
  bool top = false;
  bool bottom = false;

  int cur_x = i->x;
  bool top_next = top;
  bool bottom_next = bottom;
  
  if (j->x < cur_x) {
    cur_x = j->x;
    j++;
    bottom_next = !bottom;
  } else if (j->x == cur_x) {
    i++;
    top_next = !top;
    bottom_next = !bottom;
    j++;
  } else {
    top_next = !top;
    i++;
  }

  int topRectsHeight = topRects->y2 - topRects->y1;
  int bottomRectsHeight = bottomRects->y2 - bottomRects->y1;
  int inbetweenHeight = bottomRects->y1 - topRects->y2;
  int width = cur_x;
  
  do {
    if (top && !bottom) {
      totalArea += (inbetweenHeight+bottomRectsHeight)*width;
    } else if (bottom && !top) {
      totalArea += (inbetweenHeight+topRectsHeight)*width;
    } else if (bottom && top) {
      totalArea += (inbetweenHeight)*width;
    }
    top = top_next;
    bottom = bottom_next;
    
    if (i->x < j->x) {
      top_next = !top;
      width = i->x - cur_x;
      cur_x = i->x;
      i++;
    } else if (j->x < i->x) {
      bottom_next = !bottom;
      width = j->x - cur_x;
      cur_x = j->x;
      j++;
    } else { 
      top_next = !top;
      bottom_next = !bottom;
      width = i->x - cur_x;
      cur_x = i->x;
      i++;
      j++;
    }
  } while (i < end_i && j < end_j);

  
  while (i < end_i) {
    width = i->x - cur_x;
    cur_x = i->x;
    i++;
    if (top)
      totalArea += (inbetweenHeight+bottomRectsHeight)*width;
    top = !top;
  }

  while (j < end_j) {
    width = j->x - cur_x;
    cur_x = j->x;
    j++;
    if (bottom)
      totalArea += (inbetweenHeight+topRectsHeight)*width;
    bottom = !bottom;
  }
  return totalArea;
}

static pixman_box32_t *
CopyRow(pixman_box32_t *dest_it, pixman_box32_t *src_start, pixman_box32_t *src_end)
{
    
    pixman_box32_t *src_it = src_start;
    while (src_it < src_end) {
        *dest_it++ = *src_it++;
    }
    return dest_it;
}


#define WRITE_RECT(x1, x2, y1, y2) \
    do {                    \
         tmpRect->x1 = x1;  \
         tmpRect->x2 = x2;  \
         tmpRect->y1 = y1;  \
         tmpRect->y2 = y2;  \
         tmpRect++;         \
    } while (0)




#define MERGE_RECT(r)                 \
    do {                              \
      if (r->x1 <= x2) {              \
          if (x2 < r->x2)             \
              x2 = r->x2;             \
      } else {                        \
          WRITE_RECT(x1, x2, y1, y2); \
          x1 = r->x1;                 \
          x2 = r->x2;                 \
      }                               \
      r++;                            \
    } while (0)







static pixman_box32_t *
MergeRects(pixman_box32_t *r1,
           pixman_box32_t *r1_end,
           pixman_box32_t *r2,
           pixman_box32_t *r2_end,
           pixman_box32_t *tmpRect)
{
    




    const int y1 = r1->y1;
    const int y2 = r2->y2;
    int x1;
    int x2;

    
    if (r1->x1 < r2->x1) {
        x1 = r1->x1;
        x2 = r1->x2;
        r1++;
    } else {
        x1 = r2->x1;
        x2 = r2->x2;
        r2++;
    }

    while (r1 != r1_end && r2 != r2_end) {
        
        if (r1->x1 < r2->x1)
            MERGE_RECT (r1);
        else
            MERGE_RECT (r2);
    }

    
    if (r1 != r1_end) {
        do {
            MERGE_RECT (r1);
        } while (r1 != r1_end);
    } else if (r2 != r2_end) {
        do {
            MERGE_RECT(r2);
        } while (r2 != r2_end);
    }

    
    WRITE_RECT(x1, x2, y1, y2);

    return tmpRect;
}

void nsRegion::SimplifyOutwardByArea(uint32_t aThreshold)
{

  pixman_box32_t *boxes;
  int n;
  boxes = pixman_region32_rectangles(&mImpl, &n);

  
  if (!n)
    return;

  pixman_box32_t *end = boxes + n;
  pixman_box32_t *topRectsEnd = boxes+1;
  pixman_box32_t *topRects = boxes;

  
  nsAutoTArray<pixman_box32_t, 10> tmpStorage;
  tmpStorage.SetCapacity(n);
  pixman_box32_t *tmpRect = tmpStorage.Elements();

  pixman_box32_t *destRect = boxes;
  
  while (topRectsEnd < end && topRectsEnd->y1 == topRects->y1) {
    topRectsEnd++;
  }

  
  if (topRectsEnd == end)
    return;

  pixman_box32_t *bottomRects = topRectsEnd;
  pixman_box32_t *bottomRectsEnd = bottomRects+1;
  do {
    
    while (bottomRectsEnd < end && bottomRectsEnd->y1 == bottomRects->y1) {
      bottomRectsEnd++;
    }
    uint32_t totalArea = ComputeMergedAreaIncrease(topRects, topRectsEnd,
                                                   bottomRects, bottomRectsEnd);

    if (totalArea <= aThreshold) {
      
      pixman_box32_t *rect;
      rect = MergeRects(topRects, topRectsEnd, bottomRects, bottomRectsEnd, tmpRect);

      
      
      topRects = destRect;
      
      topRectsEnd = CopyRow(destRect, tmpRect, rect);
    } else {
      
      destRect = CopyRow(destRect, topRects, topRectsEnd);

      topRects = bottomRects;
      topRectsEnd = bottomRectsEnd;
      if (bottomRectsEnd == end) {
        
        topRectsEnd = CopyRow(destRect, topRects, topRectsEnd);
      }
    }
    bottomRects = bottomRectsEnd;
  } while (bottomRectsEnd != end);


  uint32_t reducedCount = topRectsEnd - pixman_region32_rectangles(&this->mImpl, &n);
  
  
  
  if (reducedCount > 1) {
    
    
    this->mImpl.data->numRects = reducedCount;
  } else {
    *this = GetBounds();
  }
}


typedef void (*visit_fn)(void *closure, VisitSide side, int x1, int y1, int x2, int y2);

static bool VisitNextEdgeBetweenRect(visit_fn visit, void *closure, VisitSide side,
				     pixman_box32_t *&r1, pixman_box32_t *&r2, const int y, int &x1)
{
  
  if (r1->x2 >= r2->x1) {
    MOZ_ASSERT(r2->x1 >= x1);
    visit(closure, side, x1, y, r2->x1, y);

    
    if (r1->x2 < r2->x2) {
      x1 = r1->x2;
      r1++;
    } else {
      x1 = r2->x2;
      r2++;
    }
    return true;
  } else {
    MOZ_ASSERT(r1->x2 < r2->x2);
    
    visit(closure, side, x1, y, r1->x2+1, y);
    r1++;
    
    
    
    x1 = r2->x1 - 1;
    return false;
  }
}


static void
VisitSides(visit_fn visit, void *closure, pixman_box32_t *r, pixman_box32_t *r_end)
{
  
  
  while (r != r_end) {
    visit(closure, VisitSide::LEFT, r->x1, r->y1, r->x1, r->y2);
    visit(closure, VisitSide::RIGHT, r->x2, r->y1, r->x2, r->y2);
    r++;
  }
}

static void
VisitAbove(visit_fn visit, void *closure, pixman_box32_t *r, pixman_box32_t *r_end)
{
  while (r != r_end) {
    visit(closure, VisitSide::TOP, r->x1-1, r->y1, r->x2+1, r->y1);
    r++;
  }
}

static void
VisitBelow(visit_fn visit, void *closure, pixman_box32_t *r, pixman_box32_t *r_end)
{
  while (r != r_end) {
    visit(closure, VisitSide::BOTTOM, r->x1-1, r->y2, r->x2+1, r->y2);
    r++;
  }
}

static pixman_box32_t *
VisitInbetween(visit_fn visit, void *closure, pixman_box32_t *r1,
               pixman_box32_t *r1_end,
               pixman_box32_t *r2,
               pixman_box32_t *r2_end)
{
  const int y = r1->y2;
  int x1;

  bool overlap = false;
  while (r1 != r1_end && r2 != r2_end) {
    if (!overlap) {
      
      if (r1->x1 < r2->x1) {
	x1 = r1->x1 - 1;
      } else {
	x1 = r2->x1 - 1;
      }
    }

    MOZ_ASSERT((x1 >= (r1->x1 - 1)) || (x1 >= (r2->x1 - 1)));
    if (r1->x1 < r2->x1) {
      overlap = VisitNextEdgeBetweenRect(visit, closure, VisitSide::BOTTOM, r1, r2, y, x1);
    } else {
      overlap = VisitNextEdgeBetweenRect(visit, closure, VisitSide::TOP, r2, r1, y, x1);
    }
  }

  
  if (r1 != r1_end) {
    
    do {
      visit(closure, VisitSide::BOTTOM, x1, y, r1->x2 + 1, y);
      r1++;
      if (r1 == r1_end)
	break;
      x1 = r1->x1 - 1;
    } while (true);
  } else if (r2 != r2_end) {
    
    do {
      visit(closure, VisitSide::TOP, x1, y, r2->x2 + 1, y);
      r2++;
      if (r2 == r2_end)
	break;
      x1 = r2->x1 - 1;
    } while (true);
  }

  return 0;
}

void nsRegion::VisitEdges (visit_fn visit, void *closure)
{
  pixman_box32_t *boxes;
  int n;
  boxes = pixman_region32_rectangles(&mImpl, &n);

  
  if (!n)
    return;

  pixman_box32_t *end = boxes + n;
  pixman_box32_t *topRectsEnd = boxes + 1;
  pixman_box32_t *topRects = boxes;

  
  while (topRectsEnd < end && topRectsEnd->y1 == topRects->y1) {
    topRectsEnd++;
  }

  
  
  VisitSides(visit, closure, topRects, topRectsEnd);

  VisitAbove(visit, closure, topRects, topRectsEnd);

  pixman_box32_t *bottomRects = topRects;
  pixman_box32_t *bottomRectsEnd = topRectsEnd;
  if (topRectsEnd != end) {
    do {
      
      bottomRects = topRectsEnd;
      bottomRectsEnd = topRectsEnd + 1;
      while (bottomRectsEnd < end && bottomRectsEnd->y1 == bottomRects->y1) {
        bottomRectsEnd++;
      }

      VisitSides(visit, closure, bottomRects, bottomRectsEnd);

      if (topRects->y2 == bottomRects->y1) {
        VisitInbetween(visit, closure, topRects, topRectsEnd,
                                       bottomRects, bottomRectsEnd);
      } else {
        VisitBelow(visit, closure, topRects, topRectsEnd);
        VisitAbove(visit, closure, bottomRects, bottomRectsEnd);
      }

      topRects = bottomRects;
      topRectsEnd = bottomRectsEnd;
    } while (bottomRectsEnd != end);
  }

  
  
  VisitBelow(visit, closure, bottomRects, bottomRectsEnd);
}


void nsRegion::SimplifyInward (uint32_t aMaxRects)
{
  NS_ASSERTION(aMaxRects >= 1, "Invalid max rect count");

  if (GetNumRects() <= aMaxRects)
    return;

  SetEmpty();
}

uint64_t nsRegion::Area () const
{
  uint64_t area = 0;
  nsRegionRectIterator iter(*this);
  const nsRect* r;
  while ((r = iter.Next()) != nullptr) {
    area += uint64_t(r->width)*r->height;
  }
  return area;
}

nsRegion& nsRegion::ScaleRoundOut (float aXScale, float aYScale)
{
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect.ScaleRoundOut(aXScale, aYScale);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t region;
  
  pixman_region32_init_rects(&region, boxes, n);

  pixman_region32_fini(&mImpl);
  mImpl = region;
  return *this;
}

nsRegion& nsRegion::ScaleInverseRoundOut (float aXScale, float aYScale)
{
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect.ScaleInverseRoundOut(aXScale, aYScale);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t region;
  
  pixman_region32_init_rects(&region, boxes, n);

  pixman_region32_fini(&mImpl);
  mImpl = region;
  return *this;
}

static nsIntRect
TransformRect(const nsIntRect& aRect, const gfx3DMatrix& aTransform)
{
    if (aRect.IsEmpty()) {
        return nsIntRect();
    }

    gfxRect rect(aRect.x, aRect.y, aRect.width, aRect.height);
    rect = aTransform.TransformBounds(rect);
    rect.RoundOut();

    nsIntRect intRect;
    if (!gfxUtils::GfxRectToIntRect(rect, &intRect)) {
        return nsIntRect();
    }

    return intRect;
}

nsRegion& nsRegion::Transform (const gfx3DMatrix &aTransform)
{
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    boxes[i] = RectToBox(nsIntRegion::ToRect(TransformRect(nsIntRegion::FromRect(rect), aTransform)));
  }

  pixman_region32_t region;
  
  pixman_region32_init_rects(&region, boxes, n);

  pixman_region32_fini(&mImpl);
  mImpl = region;
  return *this;
}


nsRegion nsRegion::ScaleToOtherAppUnitsRoundOut (int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect = rect.ScaleToOtherAppUnitsRoundOut(aFromAPP, aToAPP);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t pixmanRegion;
  
  pixman_region32_init_rects(&pixmanRegion, boxes, n);

  pixman_region32_fini(&region.mImpl);
  region.mImpl = pixmanRegion;
  return region;
}

nsRegion nsRegion::ScaleToOtherAppUnitsRoundIn (int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect = rect.ScaleToOtherAppUnitsRoundIn(aFromAPP, aToAPP);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t pixmanRegion;
  
  pixman_region32_init_rects(&pixmanRegion, boxes, n);

  pixman_region32_fini(&region.mImpl);
  region.mImpl = pixmanRegion;
  return region;
}

nsIntRegion nsRegion::ToPixels (nscoord aAppUnitsPerPixel, bool aOutsidePixels) const
{
  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    nsIntRect deviceRect;
    if (aOutsidePixels)
      deviceRect = rect.ToOutsidePixels(aAppUnitsPerPixel);
    else
      deviceRect = rect.ToNearestPixels(aAppUnitsPerPixel);

    boxes[i] = RectToBox(deviceRect);
  }

  nsIntRegion intRegion;
  pixman_region32_fini(&intRegion.mImpl.mImpl);
  
  pixman_region32_init_rects(&intRegion.mImpl.mImpl, boxes, n);

  return intRegion;
}

nsIntRegion nsRegion::ToOutsidePixels (nscoord aAppUnitsPerPixel) const
{
  return ToPixels(aAppUnitsPerPixel, true);
}

nsIntRegion nsRegion::ToNearestPixels (nscoord aAppUnitsPerPixel) const
{
  return ToPixels(aAppUnitsPerPixel, false);
}

nsIntRegion nsRegion::ScaleToNearestPixels (float aScaleX, float aScaleY,
                                            nscoord aAppUnitsPerPixel) const
{
  nsIntRegion result;
  nsRegionRectIterator rgnIter(*this);
  const nsRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsIntRect deviceRect =
      currentRect->ScaleToNearestPixels(aScaleX, aScaleY, aAppUnitsPerPixel);
    result.Or(result, deviceRect);
  }
  return result;
}

nsIntRegion nsRegion::ScaleToOutsidePixels (float aScaleX, float aScaleY,
                                            nscoord aAppUnitsPerPixel) const
{
  nsIntRegion result;
  nsRegionRectIterator rgnIter(*this);
  const nsRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsIntRect deviceRect =
      currentRect->ScaleToOutsidePixels(aScaleX, aScaleY, aAppUnitsPerPixel);
    result.Or(result, deviceRect);
  }
  return result;
}

nsIntRegion nsRegion::ScaleToInsidePixels (float aScaleX, float aScaleY,
                                           nscoord aAppUnitsPerPixel) const
{
  












  
  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);

  nsIntRegion intRegion;
  if (n) {
    nsRect first = BoxToRect(boxes[0]);
    nsIntRect firstDeviceRect =
      first.ScaleToInsidePixels(aScaleX, aScaleY, aAppUnitsPerPixel);

    for (int i=1; i<n; i++) {
      nsRect rect = nsRect(boxes[i].x1, boxes[i].y1,
	  boxes[i].x2 - boxes[i].x1,
	  boxes[i].y2 - boxes[i].y1);
      nsIntRect deviceRect =
	rect.ScaleToInsidePixels(aScaleX, aScaleY, aAppUnitsPerPixel);

      if (rect.y <= first.YMost()) {
	if (rect.XMost() == first.x && rect.YMost() <= first.YMost()) {
	  
	  
	  deviceRect.SetRightEdge(firstDeviceRect.x);
	} else if (rect.x == first.XMost() && rect.YMost() <= first.YMost()) {
	  
	  
	  deviceRect.SetLeftEdge(firstDeviceRect.XMost());
	} else if (rect.y == first.YMost()) {
	  
	  
	  if (rect.x <= first.x && rect.XMost() >= first.XMost()) {
	    
	    firstDeviceRect.SetBottomEdge(deviceRect.y);
	  } else if (rect.x >= first.x && rect.XMost() <= first.XMost()) {
	    
	    deviceRect.SetTopEdge(firstDeviceRect.YMost());
	  }
	}
      }

      boxes[i] = RectToBox(deviceRect);
    }

    boxes[0] = RectToBox(firstDeviceRect);

    pixman_region32_fini(&intRegion.mImpl.mImpl);
    
    pixman_region32_init_rects(&intRegion.mImpl.mImpl, boxes, n);
  }
  return intRegion;

}


















































































namespace {
  
  
  class AxisPartition {
  public:
    
    
    void InsertCoord(nscoord c) {
      uint32_t i = mStops.IndexOfFirstElementGt(c);
      if (i == 0 || mStops[i-1] != c) {
        mStops.InsertElementAt(i, c);
      }
    }

    
    
    int32_t IndexOf(nscoord p) const {
      return mStops.BinaryIndexOf(p);
    }

    
    
    nscoord StopAt(int32_t index) const {
      return mStops[index];
    }

    
    
    
    nscoord StopSize(int32_t index) const {
      return mStops[index+1] - mStops[index];
    }

    
    int32_t GetNumStops() const { return mStops.Length(); }

  private:
    nsTArray<nscoord> mStops;
  };

  const int64_t kVeryLargeNegativeNumber = 0xffff000000000000ll;

  struct SizePair {
    int64_t mSizeContainingRect;
    int64_t mSize;

    SizePair() : mSizeContainingRect(0), mSize(0) {}

    static SizePair VeryLargeNegative() {
      SizePair result;
      result.mSize = result.mSizeContainingRect = kVeryLargeNegativeNumber;
      return result;
    }
    SizePair& operator=(const SizePair& aOther) {
      mSizeContainingRect = aOther.mSizeContainingRect;
      mSize = aOther.mSize;
      return *this;
    }
    bool operator<(const SizePair& aOther) const {
      if (mSizeContainingRect < aOther.mSizeContainingRect)
        return true;
      if (mSizeContainingRect > aOther.mSizeContainingRect)
        return false;
      return mSize < aOther.mSize;
    }
    bool operator>(const SizePair& aOther) const {
      return aOther.operator<(*this);
    }
    SizePair operator+(const SizePair& aOther) const {
      SizePair result = *this;
      result.mSizeContainingRect += aOther.mSizeContainingRect;
      result.mSize += aOther.mSize;
      return result;
    }
    SizePair operator-(const SizePair& aOther) const {
      SizePair result = *this;
      result.mSizeContainingRect -= aOther.mSizeContainingRect;
      result.mSize -= aOther.mSize;
      return result;
    }
  };

  
  
  SizePair MaxSum1D(const nsTArray<SizePair> &A, int32_t n,
                    int32_t *minIdx, int32_t *maxIdx) {
    
    SizePair min, max;
    int32_t currentMinIdx = 0;

    *minIdx = 0;
    *maxIdx = 0;

    
    
    for(int32_t i = 1; i < n; i++) {
      SizePair cand = A[i] - min;
      if (cand > max) {
        max = cand;
        *minIdx = currentMinIdx;
        *maxIdx = i;
      }
      if (min > A[i]) {
        min = A[i];
        currentMinIdx = i;
      }
    }

    return max;
  }
}

nsRect nsRegion::GetLargestRectangle (const nsRect& aContainingRect) const {
  nsRect bestRect;

  if (GetNumRects() <= 1) {
    bestRect = GetBounds();
    return bestRect;
  }

  AxisPartition xaxis, yaxis;

  
  nsRegionRectIterator iter(*this);
  const nsRect *currentRect;
  while ((currentRect = iter.Next())) {
    xaxis.InsertCoord(currentRect->x);
    xaxis.InsertCoord(currentRect->XMost());
    yaxis.InsertCoord(currentRect->y);
    yaxis.InsertCoord(currentRect->YMost());
  }
  if (!aContainingRect.IsEmpty()) {
    xaxis.InsertCoord(aContainingRect.x);
    xaxis.InsertCoord(aContainingRect.XMost());
    yaxis.InsertCoord(aContainingRect.y);
    yaxis.InsertCoord(aContainingRect.YMost());
  }

  
  
  
  int32_t matrixHeight = yaxis.GetNumStops() - 1;
  int32_t matrixWidth = xaxis.GetNumStops() - 1;
  int32_t matrixSize = matrixHeight * matrixWidth;
  nsTArray<SizePair> areas(matrixSize);
  areas.SetLength(matrixSize);

  iter.Reset();
  while ((currentRect = iter.Next())) {
    int32_t xstart = xaxis.IndexOf(currentRect->x);
    int32_t xend = xaxis.IndexOf(currentRect->XMost());
    int32_t y = yaxis.IndexOf(currentRect->y);
    int32_t yend = yaxis.IndexOf(currentRect->YMost());

    for (; y < yend; y++) {
      nscoord height = yaxis.StopSize(y);
      for (int32_t x = xstart; x < xend; x++) {
        nscoord width = xaxis.StopSize(x);
        int64_t size = width*int64_t(height);
        if (currentRect->Intersects(aContainingRect)) {
          areas[y*matrixWidth+x].mSizeContainingRect = size;
        }
        areas[y*matrixWidth+x].mSize = size;
      }
    }
  }

  
  {
    
    int32_t m = matrixHeight + 1;
    int32_t n = matrixWidth + 1;
    nsTArray<SizePair> pareas(m*n);
    pareas.SetLength(m*n);
    for (int32_t y = 1; y < m; y++) {
      for (int32_t x = 1; x < n; x++) {
        SizePair area = areas[(y-1)*matrixWidth+x-1];
        if (!area.mSize) {
          area = SizePair::VeryLargeNegative();
        }
        area = area + pareas[    y*n+x-1]
                    + pareas[(y-1)*n+x  ]
                    - pareas[(y-1)*n+x-1];
        pareas[y*n+x] = area;
      }
    }

    
    areas.SetLength(0);

    SizePair bestArea;
    struct {
      int32_t left, top, right, bottom;
    } bestRectIndices = { 0, 0, 0, 0 };
    for (int32_t m1 = 0; m1 < m; m1++) {
      for (int32_t m2 = m1+1; m2 < m; m2++) {
        nsTArray<SizePair> B;
        B.SetLength(n);
        for (int32_t i = 0; i < n; i++) {
          B[i] = pareas[m2*n+i] - pareas[m1*n+i];
        }
        int32_t minIdx, maxIdx;
        SizePair area = MaxSum1D(B, n, &minIdx, &maxIdx);
        if (area > bestArea) {
          bestRectIndices.left = minIdx;
          bestRectIndices.top = m1;
          bestRectIndices.right = maxIdx;
          bestRectIndices.bottom = m2;
          bestArea = area;
        }
      }
    }

    bestRect.MoveTo(xaxis.StopAt(bestRectIndices.left),
                    yaxis.StopAt(bestRectIndices.top));
    bestRect.SizeTo(xaxis.StopAt(bestRectIndices.right) - bestRect.x,
                    yaxis.StopAt(bestRectIndices.bottom) - bestRect.y);
  }

  return bestRect;
}

std::ostream& operator<<(std::ostream& stream, const nsRegion& m) {
  stream << "[";

  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(const_cast<pixman_region32_t*>(&m.mImpl), &n);
  for (int i=0; i<n; i++) {
    if (i != 0) {
      stream << "; ";
    }
    stream << boxes[i].x1 << "," << boxes[i].y1 << "," << boxes[i].x2 << "," << boxes[i].y2;
  }

  stream << "]";
  return stream;
}

nsCString
nsRegion::ToString() const {
  return nsCString(mozilla::ToString(this).c_str());
}
