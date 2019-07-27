




#ifndef MOZILLA_SVGPATHDATA_H__
#define MOZILLA_SVGPATHDATA_H__

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIContent.h"
#include "nsINode.h"
#include "nsIWeakReferenceUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Types.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/RefPtr.h"
#include "nsSVGElement.h"
#include "nsTArray.h"

#include <string.h>

class gfxContext;
class nsSVGPathDataParser; 

struct nsSVGMark;

namespace mozilla {















































class SVGPathData
{
  friend class SVGAnimatedPathSegList;
  friend class DOMSVGPathSegList;
  friend class DOMSVGPathSeg;
  friend class ::nsSVGPathDataParser;
  
  

  typedef gfx::DrawTarget DrawTarget;
  typedef gfx::Path Path;
  typedef gfx::PathBuilder PathBuilder;
  typedef gfx::FillRule FillRule;
  typedef gfx::Float Float;
  typedef gfx::CapStyle CapStyle;

public:
  typedef const float* const_iterator;

  SVGPathData(){}
  ~SVGPathData(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  bool IsEmpty() const {
    return mData.IsEmpty();
  }

#ifdef DEBUG
  



  uint32_t CountItems() const;
#endif

  



  uint32_t Length() const {
    return mData.Length();
  }

  const float& operator[](uint32_t aIndex) const {
    return mData[aIndex];
  }

  
  bool operator==(const SVGPathData& rhs) const {
    
    
    return mData.Length() == rhs.mData.Length() &&
           memcmp(mData.Elements(), rhs.mData.Elements(),
                  mData.Length() * sizeof(float)) == 0;
  }

  bool SetCapacity(uint32_t aSize) {
    return mData.SetCapacity(aSize);
  }

  void Compact() {
    mData.Compact();
  }


  float GetPathLength() const;

  uint32_t GetPathSegAtLength(float aLength) const;

  void GetMarkerPositioningData(nsTArray<nsSVGMark> *aMarks) const;

  


  bool GetSegmentLengths(nsTArray<double> *aLengths) const;

  


  bool GetDistancesFromOriginToEndsOfVisibleSegments(FallibleTArray<double> *aArray) const;

  




  TemporaryRef<Path> ToPathForLengthOrPositionMeasuring() const;

  void ConstructPath(gfxContext *aCtx) const;
  TemporaryRef<Path> BuildPath(PathBuilder* aBuilder,
                               uint8_t aCapStyle,
                               Float aStrokeWidth) const;

  const_iterator begin() const { return mData.Elements(); }
  const_iterator end() const { return mData.Elements() + mData.Length(); }

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  
  
  
  
  

protected:
  typedef float* iterator;

  



  nsresult CopyFrom(const SVGPathData& rhs);

  float& operator[](uint32_t aIndex) {
    mCachedPath = nullptr;
    return mData[aIndex];
  }

  



  bool SetLength(uint32_t aLength) {
    mCachedPath = nullptr;
    return mData.SetLength(aLength);
  }

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mCachedPath = nullptr;
    mData.Clear();
  }

  
  
  
  
  
  
  

  nsresult AppendSeg(uint32_t aType, ...); 

  iterator begin() { mCachedPath = nullptr; return mData.Elements(); }
  iterator end() { mCachedPath = nullptr; return mData.Elements() + mData.Length(); }

  FallibleTArray<float> mData;
  mutable RefPtr<gfx::Path> mCachedPath;
};










class SVGPathDataAndInfo MOZ_FINAL : public SVGPathData
{
public:
  explicit SVGPathDataAndInfo(nsSVGElement *aElement = nullptr)
    : mElement(do_GetWeakReference(static_cast<nsINode*>(aElement)))
  {}

  void SetElement(nsSVGElement *aElement) {
    mElement = do_GetWeakReference(static_cast<nsINode*>(aElement));
  }

  nsSVGElement* Element() const {
    nsCOMPtr<nsIContent> e = do_QueryReferent(mElement);
    return static_cast<nsSVGElement*>(e.get());
  }

  nsresult CopyFrom(const SVGPathDataAndInfo& rhs) {
    mElement = rhs.mElement;
    return SVGPathData::CopyFrom(rhs);
  }

  




  bool IsIdentity() const {
    if (!mElement) {
      NS_ABORT_IF_FALSE(IsEmpty(), "target element propagation failure");
      return true;
    }
    return false;
  }

  




  using SVGPathData::CopyFrom;

  
  using SVGPathData::iterator;
  using SVGPathData::operator[];
  using SVGPathData::SetLength;
  using SVGPathData::begin;
  using SVGPathData::end;

private:
  
  
  
  
  nsWeakPtr mElement;
};

} 

#endif 
