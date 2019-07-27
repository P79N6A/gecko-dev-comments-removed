




#ifndef MOZILLA_SVGPOINTLIST_H__
#define MOZILLA_SVGPOINTLIST_H__

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIContent.h"
#include "nsINode.h"
#include "nsIWeakReferenceUtils.h"
#include "nsSVGElement.h"
#include "nsTArray.h"
#include "SVGPoint.h"

#include <string.h>

namespace mozilla {
class nsISVGPoint;










class SVGPointList
{
  friend class mozilla::nsISVGPoint;
  friend class SVGAnimatedPointList;
  friend class DOMSVGPointList;
  friend class DOMSVGPoint;

public:

  SVGPointList(){}
  ~SVGPointList(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  bool IsEmpty() const {
    return mItems.IsEmpty();
  }

  uint32_t Length() const {
    return mItems.Length();
  }

  const SVGPoint& operator[](uint32_t aIndex) const {
    return mItems[aIndex];
  }

  bool operator==(const SVGPointList& rhs) const {
    
    return mItems.Length() == rhs.mItems.Length() &&
           memcmp(mItems.Elements(), rhs.mItems.Elements(),
                  mItems.Length() * sizeof(SVGPoint)) == 0;
  }

  bool SetCapacity(uint32_t aSize) {
    return mItems.SetCapacity(aSize);
  }

  void Compact() {
    mItems.Compact();
  }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGPointList& rhs);

  SVGPoint& operator[](uint32_t aIndex) {
    return mItems[aIndex];
  }

  



  bool SetLength(uint32_t aNumberOfItems) {
    return mItems.SetLength(aNumberOfItems);
  }

private:

  
  
  

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mItems.Clear();
  }

  bool InsertItem(uint32_t aIndex, const SVGPoint &aPoint) {
    if (aIndex >= mItems.Length()) {
      aIndex = mItems.Length();
    }
    return !!mItems.InsertElementAt(aIndex, aPoint);
  }

  void ReplaceItem(uint32_t aIndex, const SVGPoint &aPoint) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems[aIndex] = aPoint;
  }

  void RemoveItem(uint32_t aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems.RemoveElementAt(aIndex);
  }

  bool AppendItem(SVGPoint aPoint) {
    return !!mItems.AppendElement(aPoint);
  }

protected:

  


  FallibleTArray<SVGPoint> mItems;
};














class SVGPointListAndInfo : public SVGPointList
{
public:

  explicit SVGPointListAndInfo(nsSVGElement *aElement = nullptr)
    : mElement(do_GetWeakReference(static_cast<nsINode*>(aElement)))
  {}

  void SetInfo(nsSVGElement *aElement) {
    mElement = do_GetWeakReference(static_cast<nsINode*>(aElement));
  }

  nsSVGElement* Element() const {
    nsCOMPtr<nsIContent> e = do_QueryReferent(mElement);
    return static_cast<nsSVGElement*>(e.get());
  }

  




  bool IsIdentity() const {
    if (!mElement) {
      NS_ABORT_IF_FALSE(IsEmpty(), "target element propagation failure");
      return true;
    } 
    return false;
  }

  nsresult CopyFrom(const SVGPointListAndInfo& rhs) {
    mElement = rhs.mElement;
    return SVGPointList::CopyFrom(rhs);
  }

  




  nsresult CopyFrom(const SVGPointList& rhs) {
    return SVGPointList::CopyFrom(rhs);
  }
  const SVGPoint& operator[](uint32_t aIndex) const {
    return SVGPointList::operator[](aIndex);
  }
  SVGPoint& operator[](uint32_t aIndex) {
    return SVGPointList::operator[](aIndex);
  }
  bool SetLength(uint32_t aNumberOfItems) {
    return SVGPointList::SetLength(aNumberOfItems);
  }

private:
  
  
  
  
  nsWeakPtr mElement;
};

} 

#endif 
