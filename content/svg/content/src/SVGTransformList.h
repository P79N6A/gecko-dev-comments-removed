





































#ifndef MOZILLA_SVGTRANSFORMLIST_H__
#define MOZILLA_SVGTRANSFORMLIST_H__

#include "SVGTransform.h"
#include "nsTArray.h"
#include "nsSVGElement.h"

namespace mozilla {










class SVGTransformList
{
  friend class SVGAnimatedTransformList;
  friend class DOMSVGTransformList;
  friend class DOMSVGTransform;

public:
  SVGTransformList() {}
  ~SVGTransformList() {}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  PRBool IsEmpty() const {
    return mItems.IsEmpty();
  }

  PRUint32 Length() const {
    return mItems.Length();
  }

  const SVGTransform& operator[](PRUint32 aIndex) const {
    return mItems[aIndex];
  }

  PRBool operator==(const SVGTransformList& rhs) const {
    return mItems == rhs.mItems;
  }

  PRBool SetCapacity(PRUint32 size) {
    return mItems.SetCapacity(size);
  }

  void Compact() {
    mItems.Compact();
  }

  gfxMatrix GetConsolidationMatrix() const;

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGTransformList& rhs);
  nsresult CopyFrom(const nsTArray<SVGTransform>& aTransformArray);

  SVGTransform& operator[](PRUint32 aIndex) {
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

  PRBool InsertItem(PRUint32 aIndex, const SVGTransform& aTransform) {
    if (aIndex >= mItems.Length()) {
      aIndex = mItems.Length();
    }
    return !!mItems.InsertElementAt(aIndex, aTransform);
  }

  void ReplaceItem(PRUint32 aIndex, const SVGTransform& aTransform) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems[aIndex] = aTransform;
  }

  void RemoveItem(PRUint32 aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems.RemoveElementAt(aIndex);
  }

  PRBool AppendItem(const SVGTransform& aTransform) {
    return !!mItems.AppendElement(aTransform);
  }

protected:
  



  nsTArray<SVGTransform> mItems;
};

} 

#endif 
