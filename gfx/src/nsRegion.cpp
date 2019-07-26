




#include "nsRegion.h"
#include "nsPrintfCString.h"
#include "nsTArray.h"


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

nsRegion nsRegion::ConvertAppUnitsRoundOut (int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect = rect.ConvertAppUnitsRoundOut(aFromAPP, aToAPP);
    boxes[i] = RectToBox(rect);
  }

  pixman_region32_t pixmanRegion;
  
  pixman_region32_init_rects(&pixmanRegion, boxes, n);

  pixman_region32_fini(&region.mImpl);
  region.mImpl = pixmanRegion;
  return region;
}

nsRegion nsRegion::ConvertAppUnitsRoundIn (int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRegion region = *this;
  int n;
  pixman_box32_t *boxes = pixman_region32_rectangles(&region.mImpl, &n);
  for (int i=0; i<n; i++) {
    nsRect rect = BoxToRect(boxes[i]);
    rect = rect.ConvertAppUnitsRoundIn(aFromAPP, aToAPP);
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

nsCString
nsRegion::ToString() const {
    nsCString result;
    result.AppendLiteral("[");

    int n;
    pixman_box32_t *boxes = pixman_region32_rectangles(const_cast<pixman_region32_t*>(&mImpl), &n);
    for (int i=0; i<n; i++) {
        if (i != 0) {
            result.AppendLiteral("; ");
        }
        result.Append(nsPrintfCString("%d,%d,%d,%d", boxes[i].x1, boxes[i].y1, boxes[i].x2, boxes[i].y2));

    }
    result.AppendLiteral("]");

    return result;
}


nsRegion nsIntRegion::ToAppUnits (nscoord aAppUnitsPerPixel) const
{
  nsRegion result;
  nsIntRegionRectIterator rgnIter(*this);
  const nsIntRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsRect appRect = currentRect->ToAppUnits(aAppUnitsPerPixel);
    result.Or(result, appRect);
  }
  return result;
}
