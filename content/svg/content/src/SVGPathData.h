



































#ifndef MOZILLA_SVGPATHDATA_H__
#define MOZILLA_SVGPATHDATA_H__

#include "SVGPathSegUtils.h"
#include "nsTArray.h"
#include "nsSVGElement.h"

class gfxContext;
class gfxMatrix;
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

  
  
  
  
  
  

protected:

  



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

  nsTArray<float> mData;
};










class SVGPathDataAndOwner : public SVGPathData
{
public:

  SVGPathDataAndOwner(nsSVGElement *aElement = nsnull)
    : mElement(aElement)
  {}

  void SetElement(nsSVGElement *aElement) {
    mElement = aElement;
  }

  nsSVGElement* Element() const {
    return mElement;
  }

  nsresult CopyFrom(const SVGPathDataAndOwner& rhs) {
    mElement = rhs.mElement;
    return SVGPathData::CopyFrom(rhs);
  }

  




  nsresult CopyFrom(const SVGPathData& rhs) {
    return SVGPathData::CopyFrom(rhs);
  }
  const float& operator[](PRUint32 aIndex) const {
    return SVGPathData::operator[](aIndex);
  }
  float& operator[](PRUint32 aIndex) {
    return SVGPathData::operator[](aIndex);
  }
  PRBool SetLength(PRUint32 aNumberOfItems) {
    return SVGPathData::SetLength(aNumberOfItems);
  }

private:
  
  
  
  nsRefPtr<nsSVGElement> mElement;
};

} 

#endif 
