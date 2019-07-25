



































#ifndef MOZILLA_SVGPOINTLIST_H__
#define MOZILLA_SVGPOINTLIST_H__

#include "SVGPoint.h"
#include "nsTArray.h"
#include "nsSVGElement.h"

namespace mozilla {










class SVGPointList
{
  friend class SVGAnimatedPointList;
  friend class DOMSVGPointList;
  friend class DOMSVGPoint;

public:

  SVGPointList(){}
  ~SVGPointList(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  PRBool IsEmpty() const {
    return mItems.IsEmpty();
  }

  PRUint32 Length() const {
    return mItems.Length();
  }

  const SVGPoint& operator[](PRUint32 aIndex) const {
    return mItems[aIndex];
  }

  PRBool operator==(const SVGPointList& rhs) const {
    
    return mItems.Length() == rhs.mItems.Length() &&
           memcmp(mItems.Elements(), rhs.mItems.Elements(),
                  mItems.Length() * sizeof(SVGPoint)) == 0;
  }

  PRBool SetCapacity(PRUint32 aSize) {
    return mItems.SetCapacity(aSize);
  }

  void Compact() {
    mItems.Compact();
  }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGPointList& rhs);

  SVGPoint& operator[](PRUint32 aIndex) {
    return mItems[aIndex];
  }

  



  PRBool SetLength(PRUint32 aNumberOfItems) {
    return mItems.SetLength(aNumberOfItems);
  }

private:

  
  
  

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mItems.Clear();
  }

  PRBool InsertItem(PRUint32 aIndex, const SVGPoint &aPoint) {
    if (aIndex >= mItems.Length()) {
      aIndex = mItems.Length();
    }
    return !!mItems.InsertElementAt(aIndex, aPoint);
  }

  void ReplaceItem(PRUint32 aIndex, const SVGPoint &aPoint) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems[aIndex] = aPoint;
  }

  void RemoveItem(PRUint32 aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems.RemoveElementAt(aIndex);
  }

  PRBool AppendItem(SVGPoint aPoint) {
    return !!mItems.AppendElement(aPoint);
  }

protected:

  


  nsTArray<SVGPoint> mItems;
};














class SVGPointListAndInfo : public SVGPointList
{
public:

  SVGPointListAndInfo(nsSVGElement *aElement = nsnull)
    : mElement(aElement)
  {}

  void SetInfo(nsSVGElement *aElement) {
    mElement = aElement;
  }

  nsSVGElement* Element() const {
    return mElement;
  }

  nsresult CopyFrom(const SVGPointListAndInfo& rhs) {
    mElement = rhs.mElement;
    return SVGPointList::CopyFrom(rhs);
  }

  




  nsresult CopyFrom(const SVGPointList& rhs) {
    return SVGPointList::CopyFrom(rhs);
  }
  const SVGPoint& operator[](PRUint32 aIndex) const {
    return SVGPointList::operator[](aIndex);
  }
  SVGPoint& operator[](PRUint32 aIndex) {
    return SVGPointList::operator[](aIndex);
  }
  PRBool SetLength(PRUint32 aNumberOfItems) {
    return SVGPointList::SetLength(aNumberOfItems);
  }

private:
  
  
  
  nsRefPtr<nsSVGElement> mElement;
};

} 

#endif 
