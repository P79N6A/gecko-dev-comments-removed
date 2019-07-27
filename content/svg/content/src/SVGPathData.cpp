




#include "SVGPathData.h"

#include "gfx2DGlue.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Types.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/RefPtr.h"
#include "nsError.h"
#include "nsString.h"
#include "nsSVGPathDataParser.h"
#include "nsSVGPathGeometryElement.h" 
#include <stdarg.h>
#include "nsStyleConsts.h"
#include "SVGContentUtils.h"
#include "SVGPathSegUtils.h"
#include "gfxContext.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;

static bool IsMoveto(uint16_t aSegType)
{
  return aSegType == PATHSEG_MOVETO_ABS ||
         aSegType == PATHSEG_MOVETO_REL;
}

nsresult
SVGPathData::CopyFrom(const SVGPathData& rhs)
{
  if (!mData.SetCapacity(rhs.mData.Length())) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mCachedPath = nullptr;
  mData = rhs.mData;
  return NS_OK;
}

void
SVGPathData::GetValueAsString(nsAString& aValue) const
{
  
  aValue.Truncate();
  if (!Length()) {
    return;
  }
  uint32_t i = 0;
  for (;;) {
    nsAutoString segAsString;
    SVGPathSegUtils::GetValueAsString(&mData[i], segAsString);
    
    aValue.Append(segAsString);
    i += 1 + SVGPathSegUtils::ArgCountForType(mData[i]);
    if (i >= mData.Length()) {
      NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");
      return;
    }
    aValue.Append(' ');
  }
}

nsresult
SVGPathData::SetValueFromString(const nsAString& aValue)
{
  
  
  

  mCachedPath = nullptr;
  nsSVGPathDataParser pathParser(aValue, this);
  return pathParser.Parse() ? NS_OK : NS_ERROR_DOM_SYNTAX_ERR;
}

nsresult
SVGPathData::AppendSeg(uint32_t aType, ...)
{
  uint32_t oldLength = mData.Length();
  uint32_t newLength = oldLength + 1 + SVGPathSegUtils::ArgCountForType(aType);
  if (!mData.SetLength(newLength)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mCachedPath = nullptr;

  mData[oldLength] = SVGPathSegUtils::EncodeType(aType);
  va_list args;
  va_start(args, aType);
  for (uint32_t i = oldLength + 1; i < newLength; ++i) {
    
    mData[i] = float(va_arg(args, double));
  }
  va_end(args);
  return NS_OK;
}

float
SVGPathData::GetPathLength() const
{
  SVGPathTraversalState state;

  uint32_t i = 0;
  while (i < mData.Length()) {
    SVGPathSegUtils::TraversePathSegment(&mData[i], state);
    i += 1 + SVGPathSegUtils::ArgCountForType(mData[i]);
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");

  return state.length;
}

#ifdef DEBUG
uint32_t
SVGPathData::CountItems() const
{
  uint32_t i = 0, count = 0;

  while (i < mData.Length()) {
    i += 1 + SVGPathSegUtils::ArgCountForType(mData[i]);
    count++;
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");

  return count;
}
#endif

bool
SVGPathData::GetSegmentLengths(nsTArray<double> *aLengths) const
{
  aLengths->Clear();
  SVGPathTraversalState state;

  uint32_t i = 0;
  while (i < mData.Length()) {
    state.length = 0.0;
    SVGPathSegUtils::TraversePathSegment(&mData[i], state);
    if (!aLengths->AppendElement(state.length)) {
      aLengths->Clear();
      return false;
    }
    i += 1 + SVGPathSegUtils::ArgCountForType(mData[i]);
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");

  return true;
}

bool
SVGPathData::GetDistancesFromOriginToEndsOfVisibleSegments(FallibleTArray<double> *aOutput) const
{
  SVGPathTraversalState state;

  aOutput->Clear();

  uint32_t i = 0;
  while (i < mData.Length()) {
    uint32_t segType = SVGPathSegUtils::DecodeType(mData[i]);
    SVGPathSegUtils::TraversePathSegment(&mData[i], state);

    
    
    
    
    
    
    
    
    

    if (i == 0 || (segType != PATHSEG_MOVETO_ABS &&
                   segType != PATHSEG_MOVETO_REL)) {
      if (!aOutput->AppendElement(state.length)) {
        return false;
      }
    }
    i += 1 + SVGPathSegUtils::ArgCountForType(segType);
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt?");

  return true;
}

uint32_t
SVGPathData::GetPathSegAtLength(float aDistance) const
{
  
  
  
  

  uint32_t i = 0, segIndex = 0;
  SVGPathTraversalState state;

  while (i < mData.Length()) {
    SVGPathSegUtils::TraversePathSegment(&mData[i], state);
    if (state.length >= aDistance) {
      return segIndex;
    }
    i += 1 + SVGPathSegUtils::ArgCountForType(mData[i]);
    segIndex++;
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");

  return std::max(0U, segIndex - 1); 
}






























static void
ApproximateZeroLengthSubpathSquareCaps(PathBuilder* aPB,
                                       const Point& aPoint,
                                       Float aStrokeWidth)
{
  
  
  
  
  MOZ_ASSERT(aStrokeWidth > 0.0f,
             "Make the caller check for this, or check it here");

  
  
  

  Float tinyLength = aStrokeWidth / 32;

  aPB->MoveTo(aPoint);
  aPB->LineTo(aPoint + Point(tinyLength, 0));
  aPB->MoveTo(aPoint);
}

static void
ApproximateZeroLengthSubpathSquareCaps(const gfxPoint &aPoint, gfxContext *aCtx)
{
  
  
  
  

  const gfxSize tinyAdvance = aCtx->DeviceToUser(gfxSize(2.0/256.0, 0.0));

  aCtx->MoveTo(aPoint);
  aCtx->LineTo(aPoint + gfxPoint(tinyAdvance.width, tinyAdvance.height));
  aCtx->MoveTo(aPoint);
}

#define MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS_TO_DT               \
  do {                                                                        \
    if (capsAreSquare && !subpathHasLength && aStrokeWidth > 0 &&             \
        subpathContainsNonArc && SVGPathSegUtils::IsValidType(prevSegType) && \
        (!IsMoveto(prevSegType) || segType == PATHSEG_CLOSEPATH)) {           \
      ApproximateZeroLengthSubpathSquareCaps(builder, segStart, aStrokeWidth);\
    }                                                                         \
  } while(0)

#define MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS                     \
  do {                                                                        \
    if (capsAreSquare && !subpathHasLength && subpathContainsNonArc &&        \
        SVGPathSegUtils::IsValidType(prevSegType) &&                          \
        (!IsMoveto(prevSegType) ||                                            \
         segType == PATHSEG_CLOSEPATH)) {                                     \
      ApproximateZeroLengthSubpathSquareCaps(segStart, aCtx);                 \
    }                                                                         \
  } while(0)

TemporaryRef<Path>
SVGPathData::BuildPath(PathBuilder* builder,
                       uint8_t aStrokeLineCap,
                       Float aStrokeWidth) const
{
  if (mData.IsEmpty() || !IsMoveto(SVGPathSegUtils::DecodeType(mData[0]))) {
    return nullptr; 
  }

  bool capsAreSquare = aStrokeLineCap == NS_STYLE_STROKE_LINECAP_SQUARE;
  bool subpathHasLength = false;  
  bool subpathContainsNonArc = false;

  uint32_t segType     = PATHSEG_UNKNOWN;
  uint32_t prevSegType = PATHSEG_UNKNOWN;
  Point pathStart(0.0, 0.0); 
  Point segStart(0.0, 0.0);
  Point segEnd;
  Point cp1, cp2;            
  Point tcp1, tcp2;          

  
  
  

  uint32_t i = 0;
  while (i < mData.Length()) {
    segType = SVGPathSegUtils::DecodeType(mData[i++]);
    uint32_t argCount = SVGPathSegUtils::ArgCountForType(segType);

    switch (segType)
    {
    case PATHSEG_CLOSEPATH:
      
      subpathContainsNonArc = true;
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS_TO_DT;
      segEnd = pathStart;
      builder->Close();
      break;

    case PATHSEG_MOVETO_ABS:
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS_TO_DT;
      pathStart = segEnd = Point(mData[i], mData[i+1]);
      builder->MoveTo(segEnd);
      subpathHasLength = false;
      subpathContainsNonArc = false;
      break;

    case PATHSEG_MOVETO_REL:
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS_TO_DT;
      pathStart = segEnd = segStart + Point(mData[i], mData[i+1]);
      builder->MoveTo(segEnd);
      subpathHasLength = false;
      subpathContainsNonArc = false;
      break;

    case PATHSEG_LINETO_ABS:
      segEnd = Point(mData[i], mData[i+1]);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_REL:
      segEnd = segStart + Point(mData[i], mData[i+1]);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_ABS:
      cp1 = Point(mData[i], mData[i+1]);
      cp2 = Point(mData[i+2], mData[i+3]);
      segEnd = Point(mData[i+4], mData[i+5]);
      builder->BezierTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_REL:
      cp1 = segStart + Point(mData[i], mData[i+1]);
      cp2 = segStart + Point(mData[i+2], mData[i+3]);
      segEnd = segStart + Point(mData[i+4], mData[i+5]);
      builder->BezierTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_ABS:
      cp1 = Point(mData[i], mData[i+1]);
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = Point(mData[i+2], mData[i+3]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      builder->BezierTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_REL:
      cp1 = segStart + Point(mData[i], mData[i+1]);
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = segStart + Point(mData[i+2], mData[i+3]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      builder->BezierTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_ARC_ABS:
    case PATHSEG_ARC_REL:
    {
      Point radii(mData[i], mData[i+1]);
      segEnd = Point(mData[i+5], mData[i+6]);
      if (segType == PATHSEG_ARC_REL) {
        segEnd += segStart;
      }
      if (segEnd != segStart) {
        if (radii.x == 0.0f || radii.y == 0.0f) {
          builder->LineTo(segEnd);
        } else {
          nsSVGArcConverter converter(segStart, segEnd, radii, mData[i+2],
                                      mData[i+3] != 0, mData[i+4] != 0);
          while (converter.GetNextSegment(&cp1, &cp2, &segEnd)) {
            builder->BezierTo(cp1, cp2, segEnd);
          }
        }
      }
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      break;
    }

    case PATHSEG_LINETO_HORIZONTAL_ABS:
      segEnd = Point(mData[i], segStart.y);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_HORIZONTAL_REL:
      segEnd = segStart + Point(mData[i], 0.0f);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_VERTICAL_ABS:
      segEnd = Point(segStart.x, mData[i]);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_VERTICAL_REL:
      segEnd = segStart + Point(0.0f, mData[i]);
      builder->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_SMOOTH_ABS:
      cp1 = SVGPathSegUtils::IsCubicType(prevSegType) ? segStart * 2 - cp2 : segStart;
      cp2 = Point(mData[i],   mData[i+1]);
      segEnd = Point(mData[i+2], mData[i+3]);
      builder->BezierTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_SMOOTH_REL:
      cp1 = SVGPathSegUtils::IsCubicType(prevSegType) ? segStart * 2 - cp2 : segStart;
      cp2 = segStart + Point(mData[i], mData[i+1]);
      segEnd = segStart + Point(mData[i+2], mData[i+3]);
      builder->BezierTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS:
      cp1 = SVGPathSegUtils::IsQuadraticType(prevSegType) ? segStart * 2 - cp1 : segStart;
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = Point(mData[i], mData[i+1]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      builder->BezierTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL:
      cp1 = SVGPathSegUtils::IsQuadraticType(prevSegType) ? segStart * 2 - cp1 : segStart;
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = segStart + Point(mData[i], mData[i+1]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      builder->BezierTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    default:
      NS_NOTREACHED("Bad path segment type");
      return nullptr; 
    }
    i += argCount;
    prevSegType = segType;
    segStart = segEnd;
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");
  NS_ABORT_IF_FALSE(prevSegType == segType,
                    "prevSegType should be left at the final segType");

  MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS_TO_DT;

  return builder->Finish();
}

void
SVGPathData::ConstructPath(gfxContext *aCtx) const
{
  if (mData.IsEmpty() || !IsMoveto(SVGPathSegUtils::DecodeType(mData[0]))) {
    return; 
  }

  bool capsAreSquare = aCtx->CurrentLineCap() == gfxContext::LINE_CAP_SQUARE;
  bool subpathHasLength = false;  
  bool subpathContainsNonArc = false;

  uint32_t segType     = PATHSEG_UNKNOWN;
  uint32_t prevSegType = PATHSEG_UNKNOWN;
  gfxPoint pathStart(0.0, 0.0); 
  gfxPoint segStart(0.0, 0.0);
  gfxPoint segEnd;
  gfxPoint cp1, cp2;            
  gfxPoint tcp1, tcp2;          

  
  
  

  uint32_t i = 0;
  while (i < mData.Length()) {
    segType = SVGPathSegUtils::DecodeType(mData[i++]);
    uint32_t argCount = SVGPathSegUtils::ArgCountForType(segType);

    switch (segType)
    {
    case PATHSEG_CLOSEPATH:
      
      subpathContainsNonArc = true;
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS;
      segEnd = pathStart;
      aCtx->ClosePath();
      break;

    case PATHSEG_MOVETO_ABS:
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS;
      pathStart = segEnd = gfxPoint(mData[i], mData[i+1]);
      aCtx->MoveTo(segEnd);
      subpathHasLength = false;
      subpathContainsNonArc = false;
      break;

    case PATHSEG_MOVETO_REL:
      MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS;
      pathStart = segEnd = segStart + gfxPoint(mData[i], mData[i+1]);
      aCtx->MoveTo(segEnd);
      subpathHasLength = false;
      subpathContainsNonArc = false;
      break;

    case PATHSEG_LINETO_ABS:
      segEnd = gfxPoint(mData[i], mData[i+1]);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_REL:
      segEnd = segStart + gfxPoint(mData[i], mData[i+1]);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_ABS:
      cp1 = gfxPoint(mData[i], mData[i+1]);
      cp2 = gfxPoint(mData[i+2], mData[i+3]);
      segEnd = gfxPoint(mData[i+4], mData[i+5]);
      aCtx->CurveTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_REL:
      cp1 = segStart + gfxPoint(mData[i], mData[i+1]);
      cp2 = segStart + gfxPoint(mData[i+2], mData[i+3]);
      segEnd = segStart + gfxPoint(mData[i+4], mData[i+5]);
      aCtx->CurveTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_ABS:
      cp1 = gfxPoint(mData[i], mData[i+1]);
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = gfxPoint(mData[i+2], mData[i+3]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      aCtx->CurveTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_REL:
      cp1 = segStart + gfxPoint(mData[i], mData[i+1]);
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = segStart + gfxPoint(mData[i+2], mData[i+3]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      aCtx->CurveTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_ARC_ABS:
    case PATHSEG_ARC_REL:
    {
      gfxPoint radii(mData[i], mData[i+1]);
      segEnd = gfxPoint(mData[i+5], mData[i+6]);
      if (segType == PATHSEG_ARC_REL) {
        segEnd += segStart;
      }
      if (segEnd != segStart) {
        if (radii.x == 0.0f || radii.y == 0.0f) {
          aCtx->LineTo(segEnd);
        } else {
          nsSVGArcConverter converter(ToPoint(segStart), ToPoint(segEnd),
                                      ToPoint(radii), mData[i+2],
                                      mData[i+3] != 0, mData[i+4] != 0);
          Point cp1, cp2, segEnd_;
          while (converter.GetNextSegment(&cp1, &cp2, &segEnd_)) {
            aCtx->CurveTo(ThebesPoint(cp1), ThebesPoint(cp2), ThebesPoint(segEnd_));
          }
          segEnd = ThebesPoint(segEnd_);
        }
      }
      if (!subpathHasLength) {
        
        
        
        segStart = ThebesPoint(ToPoint(segStart));
        subpathHasLength = (segEnd != segStart);
      }
      break;
    }

    case PATHSEG_LINETO_HORIZONTAL_ABS:
      segEnd = gfxPoint(mData[i], segStart.y);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_HORIZONTAL_REL:
      segEnd = segStart + gfxPoint(mData[i], 0.0f);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_VERTICAL_ABS:
      segEnd = gfxPoint(segStart.x, mData[i]);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_LINETO_VERTICAL_REL:
      segEnd = segStart + gfxPoint(0.0f, mData[i]);
      aCtx->LineTo(segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_SMOOTH_ABS:
      cp1 = SVGPathSegUtils::IsCubicType(prevSegType) ? segStart * 2 - cp2 : segStart;
      cp2 = gfxPoint(mData[i],   mData[i+1]);
      segEnd = gfxPoint(mData[i+2], mData[i+3]);
      aCtx->CurveTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_CUBIC_SMOOTH_REL:
      cp1 = SVGPathSegUtils::IsCubicType(prevSegType) ? segStart * 2 - cp2 : segStart;
      cp2 = segStart + gfxPoint(mData[i], mData[i+1]);
      segEnd = segStart + gfxPoint(mData[i+2], mData[i+3]);
      aCtx->CurveTo(cp1, cp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1 || segEnd != cp2);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS:
      cp1 = SVGPathSegUtils::IsQuadraticType(prevSegType) ? segStart * 2 - cp1 : segStart;
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = gfxPoint(mData[i], mData[i+1]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      aCtx->CurveTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL:
      cp1 = SVGPathSegUtils::IsQuadraticType(prevSegType) ? segStart * 2 - cp1 : segStart;
      
      tcp1 = segStart + (cp1 - segStart) * 2 / 3;
      segEnd = segStart + gfxPoint(mData[i], mData[i+1]); 
      tcp2 = cp1 + (segEnd - cp1) / 3;
      aCtx->CurveTo(tcp1, tcp2, segEnd);
      if (!subpathHasLength) {
        subpathHasLength = (segEnd != segStart || segEnd != cp1);
      }
      subpathContainsNonArc = true;
      break;

    default:
      NS_NOTREACHED("Bad path segment type");
      return; 
    }
    i += argCount;
    prevSegType = segType;
    segStart = segEnd;
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");
  NS_ABORT_IF_FALSE(prevSegType == segType,
                    "prevSegType should be left at the final segType");

  MAYBE_APPROXIMATE_ZERO_LENGTH_SUBPATH_SQUARE_CAPS;
}

TemporaryRef<Path>
SVGPathData::ToPathForLengthOrPositionMeasuring() const
{
  
  
  
  
  
  
  

  if (!mCachedPath) {
    RefPtr<DrawTarget> drawTarget =
      gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
    RefPtr<PathBuilder> builder =
      drawTarget->CreatePathBuilder(FillRule::FILL_WINDING);
    mCachedPath = BuildPath(builder, NS_STYLE_STROKE_LINECAP_BUTT, 0);
  }

  return mCachedPath;
}

static double
AngleOfVector(const Point& aVector)
{
  
  
  
  
  

  return (aVector != Point(0.0, 0.0)) ? atan2(aVector.y, aVector.x) : 0.0;
}

static float
AngleOfVectorF(const Point& aVector)
{
  return static_cast<float>(AngleOfVector(aVector));
}

void
SVGPathData::GetMarkerPositioningData(nsTArray<nsSVGMark> *aMarks) const
{
  
  
  

  
  Point pathStart(0.0, 0.0);
  float pathStartAngle = 0.0f;

  
  uint16_t prevSegType = PATHSEG_UNKNOWN;
  Point prevSegEnd(0.0, 0.0);
  float prevSegEndAngle = 0.0f;
  Point prevCP; 

  uint32_t i = 0;
  while (i < mData.Length()) {

    
    uint16_t segType =
      SVGPathSegUtils::DecodeType(mData[i++]); 
    Point& segStart = prevSegEnd;
    Point segEnd;
    float segStartAngle, segEndAngle;

    switch (segType) 
    {
    case PATHSEG_CLOSEPATH:
      segEnd = pathStart;
      segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
      break;

    case PATHSEG_MOVETO_ABS:
    case PATHSEG_MOVETO_REL:
      if (segType == PATHSEG_MOVETO_ABS) {
        segEnd = Point(mData[i], mData[i+1]);
      } else {
        segEnd = segStart + Point(mData[i], mData[i+1]);
      }
      pathStart = segEnd;
      
      
      segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
      i += 2;
      break;

    case PATHSEG_LINETO_ABS:
    case PATHSEG_LINETO_REL:
      if (segType == PATHSEG_LINETO_ABS) {
        segEnd = Point(mData[i], mData[i+1]);
      } else {
        segEnd = segStart + Point(mData[i], mData[i+1]);
      }
      segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
      i += 2;
      break;

    case PATHSEG_CURVETO_CUBIC_ABS:
    case PATHSEG_CURVETO_CUBIC_REL:
    {
      Point cp1, cp2; 
      if (segType == PATHSEG_CURVETO_CUBIC_ABS) {
        cp1 = Point(mData[i], mData[i+1]);
        cp2 = Point(mData[i+2], mData[i+3]);
        segEnd = Point(mData[i+4], mData[i+5]);
      } else {
        cp1 = segStart + Point(mData[i], mData[i+1]);
        cp2 = segStart + Point(mData[i+2], mData[i+3]);
        segEnd = segStart + Point(mData[i+4], mData[i+5]);
      }
      prevCP = cp2;
      if (cp1 == segStart) {
        cp1 = cp2;
      }
      if (cp2 == segEnd) {
        cp2 = cp1;
      }
      segStartAngle = AngleOfVectorF(cp1 - segStart);
      segEndAngle = AngleOfVectorF(segEnd - cp2);
      i += 6;
      break;
    }

    case PATHSEG_CURVETO_QUADRATIC_ABS:
    case PATHSEG_CURVETO_QUADRATIC_REL:
    {
      Point cp1, cp2; 
      if (segType == PATHSEG_CURVETO_QUADRATIC_ABS) {
        cp1 = Point(mData[i], mData[i+1]);
        segEnd = Point(mData[i+2], mData[i+3]);
      } else {
        cp1 = segStart + Point(mData[i], mData[i+1]);
        segEnd = segStart + Point(mData[i+2], mData[i+3]);
      }
      prevCP = cp1;
      segStartAngle = AngleOfVectorF(cp1 - segStart);
      segEndAngle = AngleOfVectorF(segEnd - cp1);
      i += 4;
      break;
    }

    case PATHSEG_ARC_ABS:
    case PATHSEG_ARC_REL:
    {
      double rx = mData[i];
      double ry = mData[i+1];
      double angle = mData[i+2];
      bool largeArcFlag = mData[i+3] != 0.0f;
      bool sweepFlag = mData[i+4] != 0.0f;
      if (segType == PATHSEG_ARC_ABS) {
        segEnd = Point(mData[i+5], mData[i+6]);
      } else {
        segEnd = segStart + Point(mData[i+5], mData[i+6]);
      }

      
      

      if (segStart == segEnd) {
        
        
        
        
        
        
        
        i += 7;
        continue;
      }

      
      
      

      if (rx == 0.0 || ry == 0.0) {
        
        segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
        i += 7;
        break;
      }
      rx = fabs(rx); 
      ry = fabs(ry);

      
      angle = angle * M_PI/180.0;
      double x1p =  cos(angle) * (segStart.x - segEnd.x) / 2.0
                  + sin(angle) * (segStart.y - segEnd.y) / 2.0;
      double y1p = -sin(angle) * (segStart.x - segEnd.x) / 2.0
                  + cos(angle) * (segStart.y - segEnd.y) / 2.0;

      
      double root;
      double numerator = rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p;

      if (numerator >= 0.0) {
        root = sqrt(numerator/(rx*rx*y1p*y1p + ry*ry*x1p*x1p));
        if (largeArcFlag == sweepFlag)
          root = -root;
      } else {
        
        
        
        
        
        

        double lamedh = 1.0 - numerator/(rx*rx*ry*ry); 
        double s = sqrt(lamedh);
        rx *= s;  
        ry *= s;
        root = 0.0;
      }

      double cxp =  root * rx * y1p / ry;  
      double cyp = -root * ry * x1p / rx;

      double theta, delta;
      theta = AngleOfVector(Point((x1p-cxp)/rx, (y1p-cyp)/ry));    
      delta = AngleOfVector(Point((-x1p-cxp)/rx, (-y1p-cyp)/ry)) - 
              theta;
      if (!sweepFlag && delta > 0)
        delta -= 2.0 * M_PI;
      else if (sweepFlag && delta < 0)
        delta += 2.0 * M_PI;

      double tx1, ty1, tx2, ty2;
      tx1 = -cos(angle)*rx*sin(theta) - sin(angle)*ry*cos(theta);
      ty1 = -sin(angle)*rx*sin(theta) + cos(angle)*ry*cos(theta);
      tx2 = -cos(angle)*rx*sin(theta+delta) - sin(angle)*ry*cos(theta+delta);
      ty2 = -sin(angle)*rx*sin(theta+delta) + cos(angle)*ry*cos(theta+delta);

      if (delta < 0.0f) {
        tx1 = -tx1;
        ty1 = -ty1;
        tx2 = -tx2;
        ty2 = -ty2;
      }

      segStartAngle = static_cast<float>(atan2(ty1, tx1));
      segEndAngle = static_cast<float>(atan2(ty2, tx2));
      i += 7;
      break;
    }

    case PATHSEG_LINETO_HORIZONTAL_ABS:
    case PATHSEG_LINETO_HORIZONTAL_REL:
      if (segType == PATHSEG_LINETO_HORIZONTAL_ABS) {
        segEnd = Point(mData[i++], segStart.y);
      } else {
        segEnd = segStart + Point(mData[i++], 0.0f);
      }
      segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
      break;

    case PATHSEG_LINETO_VERTICAL_ABS:
    case PATHSEG_LINETO_VERTICAL_REL:
      if (segType == PATHSEG_LINETO_VERTICAL_ABS) {
        segEnd = Point(segStart.x, mData[i++]);
      } else {
        segEnd = segStart + Point(0.0f, mData[i++]);
      }
      segStartAngle = segEndAngle = AngleOfVectorF(segEnd - segStart);
      break;

    case PATHSEG_CURVETO_CUBIC_SMOOTH_ABS:
    case PATHSEG_CURVETO_CUBIC_SMOOTH_REL:
    {
      Point cp1 = SVGPathSegUtils::IsCubicType(prevSegType) ?
                       segStart * 2 - prevCP : segStart;
      Point cp2;
      if (segType == PATHSEG_CURVETO_CUBIC_SMOOTH_ABS) {
        cp2 = Point(mData[i], mData[i+1]);
        segEnd = Point(mData[i+2], mData[i+3]);
      } else {
        cp2 = segStart + Point(mData[i], mData[i+1]);
        segEnd = segStart + Point(mData[i+2], mData[i+3]);
      }
      prevCP = cp2;
      if (cp1 == segStart) {
        cp1 = cp2;
      }
      if (cp2 == segEnd) {
        cp2 = cp1;
      }
      segStartAngle = AngleOfVectorF(cp1 - segStart);
      segEndAngle = AngleOfVectorF(segEnd - cp2);
      i += 4;
      break;
    }

    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS:
    case PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL:
    {
      Point cp1 = SVGPathSegUtils::IsQuadraticType(prevSegType) ?
                       segStart * 2 - prevCP : segStart;
      Point cp2;
      if (segType == PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS) {
        segEnd = Point(mData[i], mData[i+1]);
      } else {
        segEnd = segStart + Point(mData[i], mData[i+1]);
      }
      prevCP = cp1;
      segStartAngle = AngleOfVectorF(cp1 - segStart);
      segEndAngle = AngleOfVectorF(segEnd - cp1);
      i += 2;
      break;
    }

    default:
      
      
      NS_ABORT_IF_FALSE(false, "Unknown segment type - path corruption?");
      return;
    }

    
    if (aMarks->Length()) {
      nsSVGMark &mark = aMarks->LastElement();
      if (!IsMoveto(segType) && IsMoveto(prevSegType)) {
        
        pathStartAngle = mark.angle = segStartAngle;
      } else if (IsMoveto(segType) && !IsMoveto(prevSegType)) {
        
        if (prevSegType != PATHSEG_CLOSEPATH)
          mark.angle = prevSegEndAngle;
      } else {
        if (!(segType == PATHSEG_CLOSEPATH &&
              prevSegType == PATHSEG_CLOSEPATH))
          mark.angle = SVGContentUtils::AngleBisect(prevSegEndAngle, segStartAngle);
      }
    }

    
    if (!aMarks->AppendElement(nsSVGMark(static_cast<float>(segEnd.x),
                                         static_cast<float>(segEnd.y),
                                         0.0f,
                                         nsSVGMark::eMid))) {
      aMarks->Clear(); 
      return;
    }

    if (segType == PATHSEG_CLOSEPATH &&
        prevSegType != PATHSEG_CLOSEPATH) {
      aMarks->LastElement().angle =
        
        SVGContentUtils::AngleBisect(segEndAngle, pathStartAngle);
    }

    prevSegType = segType;
    prevSegEnd = segEnd;
    prevSegEndAngle = segEndAngle;
  }

  NS_ABORT_IF_FALSE(i == mData.Length(), "Very, very bad - mData corrupt");

  if (aMarks->Length()) {
    if (prevSegType != PATHSEG_CLOSEPATH) {
      aMarks->LastElement().angle = prevSegEndAngle;
    }
    aMarks->LastElement().type = nsSVGMark::eEnd;
    aMarks->ElementAt(0).type = nsSVGMark::eStart;
  }
}

size_t
SVGPathData::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return mData.SizeOfExcludingThis(aMallocSizeOf);
}

size_t
SVGPathData::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

