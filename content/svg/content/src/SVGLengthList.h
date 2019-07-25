



































#ifndef MOZILLA_SVGLENGTHLIST_H__
#define MOZILLA_SVGLENGTHLIST_H__

#include "SVGLength.h"
#include "nsTArray.h"
#include "nsSVGElement.h"

namespace mozilla {










class SVGLengthList
{
  friend class SVGAnimatedLengthList;
  friend class DOMSVGLengthList;
  friend class DOMSVGLength;

public:

  SVGLengthList(){}
  ~SVGLengthList(){}

  
  

  
  void GetValueAsString(nsAString& aValue) const;

  PRBool IsEmpty() const {
    return mLengths.IsEmpty();
  }

  PRUint32 Length() const {
    return mLengths.Length();
  }

  const SVGLength& operator[](PRUint32 aIndex) const {
    return mLengths[aIndex];
  }

  PRBool operator==(const SVGLengthList& rhs) const;

  PRBool SetCapacity(PRUint32 size) {
    return mLengths.SetCapacity(size);
  }

  void Compact() {
    mLengths.Compact();
  }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGLengthList& rhs);

  SVGLength& operator[](PRUint32 aIndex) {
    return mLengths[aIndex];
  }

  



  PRBool SetLength(PRUint32 aNumberOfItems) {
    return mLengths.SetLength(aNumberOfItems);
  }

private:

  
  
  

  nsresult SetValueFromString(const nsAString& aValue);

  void Clear() {
    mLengths.Clear();
  }

  PRBool InsertItem(PRUint32 aIndex, const SVGLength &aLength) {
    if (aIndex >= mLengths.Length()) aIndex = mLengths.Length();
    return !!mLengths.InsertElementAt(aIndex, aLength);
  }

  void ReplaceItem(PRUint32 aIndex, const SVGLength &aLength) {
    NS_ABORT_IF_FALSE(aIndex < mLengths.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mLengths[aIndex] = aLength;
  }

  void RemoveItem(PRUint32 aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mLengths.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mLengths.RemoveElementAt(aIndex);
  }

  PRBool AppendItem(SVGLength aLength) {
    return !!mLengths.AppendElement(aLength);
  }

protected:

  
































  nsTArray<SVGLength> mLengths;
};







class SVGLengthListAndInfo : public SVGLengthList
{
public:

  SVGLengthListAndInfo()
    : mElement(nsnull)
    , mAxis(0)
    , mCanZeroPadList(PR_FALSE)
  {}

  SVGLengthListAndInfo(nsSVGElement *aElement, PRUint8 aAxis, PRBool aCanZeroPadList)
    : mElement(aElement)
    , mAxis(aAxis)
    , mCanZeroPadList(aCanZeroPadList)
  {}

  void SetInfo(nsSVGElement *aElement, PRUint8 aAxis, PRBool aCanZeroPadList) {
    mElement = aElement;
    mAxis = aAxis;
    mCanZeroPadList = aCanZeroPadList;
  }

  nsSVGElement* Element() const {
    return mElement; 
  }

  PRUint8 Axis() const {
    NS_ABORT_IF_FALSE(mElement, "Axis() isn't valid");
    return mAxis;
  }

  
























  PRBool CanZeroPadList() const {
    
    return mCanZeroPadList;
  }

  
  void SetCanZeroPadList(PRBool aCanZeroPadList) {
    mCanZeroPadList = aCanZeroPadList;
  }

  nsresult CopyFrom(const SVGLengthListAndInfo& rhs) {
    mElement = rhs.mElement;
    mAxis = rhs.mAxis;
    mCanZeroPadList = rhs.mCanZeroPadList;
    return SVGLengthList::CopyFrom(rhs);
  }

  
  
  

  




  nsresult CopyFrom(const SVGLengthList& rhs) {
    return SVGLengthList::CopyFrom(rhs);
  }
  const SVGLength& operator[](PRUint32 aIndex) const {
    return SVGLengthList::operator[](aIndex);
  }
  SVGLength& operator[](PRUint32 aIndex) {
    return SVGLengthList::operator[](aIndex);
  }
  PRBool SetLength(PRUint32 aNumberOfItems) {
    return SVGLengthList::SetLength(aNumberOfItems);
  }

private:
  
  
  
  nsRefPtr<nsSVGElement> mElement;
  PRUint8 mAxis;
  PRPackedBool mCanZeroPadList;
};
















class NS_STACK_CLASS SVGUserUnitList
{
public:

  SVGUserUnitList()
    : mList(nsnull)
  {}

  void Init(const SVGLengthList *aList, nsSVGElement *aElement, PRUint8 aAxis) {
    mList = aList;
    mElement = aElement;
    mAxis = aAxis;
  }

  void Clear() {
    mList = nsnull;
  }

  PRUint32 Length() {
    return mList ? mList->Length() : 0;
  }

  
  float operator[](PRUint32 aIndex) {
    return (*mList)[aIndex].GetValueInUserUnits(mElement, mAxis);
  }

private:
  const SVGLengthList *mList;
  nsSVGElement *mElement;
  PRUint8 mAxis;
};

} 

#endif 
