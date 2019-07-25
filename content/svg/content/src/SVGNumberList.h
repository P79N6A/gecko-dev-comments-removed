



































#ifndef MOZILLA_SVGNUMBERLIST_H__
#define MOZILLA_SVGNUMBERLIST_H__

#include "nsTArray.h"
#include "nsSVGElement.h"

namespace mozilla {










class SVGNumberList
{
  friend class SVGAnimatedNumberList;
  friend class DOMSVGNumberList;
  friend class DOMSVGNumber;

public:

  SVGNumberList(){}
  ~SVGNumberList(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  bool IsEmpty() const {
    return mNumbers.IsEmpty();
  }

  PRUint32 Length() const {
    return mNumbers.Length();
  }

  const float& operator[](PRUint32 aIndex) const {
    return mNumbers[aIndex];
  }

  bool operator==(const SVGNumberList& rhs) const {
    return mNumbers == rhs.mNumbers;
  }

  bool SetCapacity(PRUint32 size) {
    return mNumbers.SetCapacity(size);
  }

  void Compact() {
    mNumbers.Compact();
  }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGNumberList& rhs);

  float& operator[](PRUint32 aIndex) {
    return mNumbers[aIndex];
  }

  



  bool SetLength(PRUint32 aNumberOfItems) {
    return mNumbers.SetLength(aNumberOfItems);
  }

private:

  
  
  

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mNumbers.Clear();
  }

  bool InsertItem(PRUint32 aIndex, const float &aNumber) {
    if (aIndex >= mNumbers.Length()) {
      aIndex = mNumbers.Length();
    }
    return !!mNumbers.InsertElementAt(aIndex, aNumber);
  }

  void ReplaceItem(PRUint32 aIndex, const float &aNumber) {
    NS_ABORT_IF_FALSE(aIndex < mNumbers.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mNumbers[aIndex] = aNumber;
  }

  void RemoveItem(PRUint32 aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mNumbers.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mNumbers.RemoveElementAt(aIndex);
  }

  bool AppendItem(float aNumber) {
    return !!mNumbers.AppendElement(aNumber);
  }

protected:

  


  nsTArray<float> mNumbers;
};









class SVGNumberListAndInfo : public SVGNumberList
{
public:

  SVGNumberListAndInfo()
    : mElement(nsnull)
  {}

  SVGNumberListAndInfo(nsSVGElement *aElement)
    : mElement(do_GetWeakReference(static_cast<nsINode*>(aElement)))
  {}

  void SetInfo(nsSVGElement *aElement) {
    mElement = do_GetWeakReference(static_cast<nsINode*>(aElement));
  }

  nsSVGElement* Element() const {
    nsCOMPtr<nsIContent> e = do_QueryReferent(mElement);
    return static_cast<nsSVGElement*>(e.get());
  }

  nsresult CopyFrom(const SVGNumberListAndInfo& rhs) {
    mElement = rhs.mElement;
    return SVGNumberList::CopyFrom(rhs);
  }

  
  
  

  




  nsresult CopyFrom(const SVGNumberList& rhs) {
    return SVGNumberList::CopyFrom(rhs);
  }
  const float& operator[](PRUint32 aIndex) const {
    return SVGNumberList::operator[](aIndex);
  }
  float& operator[](PRUint32 aIndex) {
    return SVGNumberList::operator[](aIndex);
  }
  bool SetLength(PRUint32 aNumberOfItems) {
    return SVGNumberList::SetLength(aNumberOfItems);
  }

private:
  
  
  
  
  nsWeakPtr mElement;
};

} 

#endif 
