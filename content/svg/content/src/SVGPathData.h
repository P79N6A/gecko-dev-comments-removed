



































#ifndef MOZILLA_SVGPATHDATA_H__
#define MOZILLA_SVGPATHDATA_H__

#include "SVGPathSegUtils.h"
#include "nsTArray.h"
#include "nsSVGElement.h"
#include "nsIWeakReferenceUtils.h"

class gfxContext;
struct gfxMatrix;
class gfxFlattenedPath;
class nsSVGPathDataParserToInternal;
struct nsSVGMark;

namespace mozilla {















































class SVGPathData
{
  friend class SVGAnimatedPathSegList;
  friend class DOMSVGPathSegList;
  friend class DOMSVGPathSeg;
  friend class ::nsSVGPathDataParserToInternal;
  
  

public:
  typedef const float* const_iterator;

  SVGPathData(){}
  ~SVGPathData(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  PRBool IsEmpty() const {
    return mData.IsEmpty();
  }

#ifdef DEBUG
  



  PRUint32 CountItems() const;
#endif

  



  PRUint32 Length() const {
    return mData.Length();
  }

  const float& operator[](PRUint32 aIndex) const {
    return mData[aIndex];
  }

  
  PRBool operator==(const SVGPathData& rhs) const {
    
    
    return mData.Length() == rhs.mData.Length() &&
           memcmp(mData.Elements(), rhs.mData.Elements(),
                  mData.Length() * sizeof(float)) == 0;
  }

  PRBool SetCapacity(PRUint32 aSize) {
    return mData.SetCapacity(aSize);
  }

  void Compact() {
    mData.Compact();
  }


  float GetPathLength() const;

  PRUint32 GetPathSegAtLength(float aLength) const;

  void GetMarkerPositioningData(nsTArray<nsSVGMark> *aMarks) const;

  


  PRBool GetSegmentLengths(nsTArray<double> *aLengths) const;

  


  PRBool GetDistancesFromOriginToEndsOfVisibleSegments(nsTArray<double> *aArray) const;

  already_AddRefed<gfxFlattenedPath>
  ToFlattenedPath(const gfxMatrix& aMatrix) const;

  void ConstructPath(gfxContext *aCtx) const;

  const_iterator begin() const { return mData.Elements(); }
  const_iterator end() const { return mData.Elements() + mData.Length(); }

  
  
  
  
  
  

protected:
  typedef float* iterator;

  



  nsresult CopyFrom(const SVGPathData& rhs);

  float& operator[](PRUint32 aIndex) {
    return mData[aIndex];
  }

  



  PRBool SetLength(PRUint32 aLength) {
    return mData.SetLength(aLength);
  }

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mData.Clear();
  }

  
  
  
  
  
  
  

  nsresult AppendSeg(PRUint32 aType, ...); 

  iterator begin() { return mData.Elements(); }
  iterator end() { return mData.Elements() + mData.Length(); }

  nsTArray<float> mData;
};










class SVGPathDataAndOwner : public SVGPathData
{
public:
  SVGPathDataAndOwner(nsSVGElement *aElement = nsnull)
    : mElement(do_GetWeakReference(static_cast<nsINode*>(aElement)))
  {}

  void SetElement(nsSVGElement *aElement) {
    mElement = do_GetWeakReference(static_cast<nsINode*>(aElement));
  }

  nsSVGElement* Element() const {
    nsCOMPtr<nsIContent> e = do_QueryReferent(mElement);
    return static_cast<nsSVGElement*>(e.get());
  }

  nsresult CopyFrom(const SVGPathDataAndOwner& rhs) {
    mElement = rhs.mElement;
    return SVGPathData::CopyFrom(rhs);
  }

  PRBool IsIdentity() const {
    if (!mElement) {
      NS_ABORT_IF_FALSE(IsEmpty(), "target element propagation failure");
      return PR_TRUE;
    }
    return PR_FALSE;
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
