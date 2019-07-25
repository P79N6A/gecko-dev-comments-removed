




































#ifndef MOZILLA_SVGSTRINGLIST_H__
#define MOZILLA_SVGSTRINGLIST_H__

#include "nsTArray.h"
#include "nsSVGElement.h"

namespace mozilla {





class SVGStringList
{
  friend class DOMSVGStringList;

public:

  SVGStringList() : mIsSet(false) {}
  ~SVGStringList(){}

  nsresult SetValue(const nsAString& aValue, bool aIsCommaSeparated);

  void Clear() {
    mStrings.Clear();
    mIsSet = false;
  }

  
  void GetValue(nsAString& aValue, bool aIsCommaSeparated) const;

  bool IsEmpty() const {
    return mStrings.IsEmpty();
  }

  PRUint32 Length() const {
    return mStrings.Length();
  }

  const nsAString& operator[](PRUint32 aIndex) const {
    return mStrings[aIndex];
  }

  bool operator==(const SVGStringList& rhs) const {
    return mStrings == rhs.mStrings;
  }

  bool SetCapacity(PRUint32 size) {
    return mStrings.SetCapacity(size);
  }

  void Compact() {
    mStrings.Compact();
  }

  
  
  
  bool IsExplicitlySet() const
    { return mIsSet; }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGStringList& rhs);

  nsAString& operator[](PRUint32 aIndex) {
    return mStrings[aIndex];
  }

  



  bool SetLength(PRUint32 aStringOfItems) {
    return mStrings.SetLength(aStringOfItems);
  }

private:

  
  
  

  bool InsertItem(PRUint32 aIndex, const nsAString &aString) {
    if (aIndex >= mStrings.Length()) {
      aIndex = mStrings.Length();
    }
    if (mStrings.InsertElementAt(aIndex, aString)) {
      mIsSet = true;
      return true;
    }
    return false;
  }

  void ReplaceItem(PRUint32 aIndex, const nsAString &aString) {
    NS_ABORT_IF_FALSE(aIndex < mStrings.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mStrings[aIndex] = aString;
  }

  void RemoveItem(PRUint32 aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mStrings.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mStrings.RemoveElementAt(aIndex);
  }

  bool AppendItem(const nsAString &aString) {
    if (mStrings.AppendElement(aString)) {
      mIsSet = true;
      return true;
    }
    return false;
  }

protected:

  


  nsTArray<nsString> mStrings;
  bool mIsSet;
};

} 

#endif 
