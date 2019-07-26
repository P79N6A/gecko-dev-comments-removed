





#ifndef MOZILLA_SVGTRANSFORMLIST_H__
#define MOZILLA_SVGTRANSFORMLIST_H__

#include "gfxMatrix.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "nsSVGTransform.h"

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

  bool IsEmpty() const {
    return mItems.IsEmpty();
  }

  uint32_t Length() const {
    return mItems.Length();
  }

  const nsSVGTransform& operator[](uint32_t aIndex) const {
    return mItems[aIndex];
  }

  bool operator==(const SVGTransformList& rhs) const {
    return mItems == rhs.mItems;
  }

  bool SetCapacity(uint32_t size) {
    return mItems.SetCapacity(size);
  }

  void Compact() {
    mItems.Compact();
  }

  gfxMatrix GetConsolidationMatrix() const;

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGTransformList& rhs);
  nsresult CopyFrom(const nsTArray<nsSVGTransform>& aTransformArray);

  nsSVGTransform& operator[](uint32_t aIndex) {
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

  bool InsertItem(uint32_t aIndex, const nsSVGTransform& aTransform) {
    if (aIndex >= mItems.Length()) {
      aIndex = mItems.Length();
    }
    return !!mItems.InsertElementAt(aIndex, aTransform);
  }

  void ReplaceItem(uint32_t aIndex, const nsSVGTransform& aTransform) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems[aIndex] = aTransform;
  }

  void RemoveItem(uint32_t aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mItems.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mItems.RemoveElementAt(aIndex);
  }

  bool AppendItem(const nsSVGTransform& aTransform) {
    return !!mItems.AppendElement(aTransform);
  }

protected:
  



  FallibleTArray<nsSVGTransform> mItems;
};

} 

#endif 
