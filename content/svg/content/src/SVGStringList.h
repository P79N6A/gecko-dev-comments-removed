




#ifndef MOZILLA_SVGSTRINGLIST_H__
#define MOZILLA_SVGSTRINGLIST_H__

#include "nsDebug.h"
#include "nsTArray.h"
#include "nsString.h" 

namespace mozilla {





class SVGStringList
{
  friend class DOMSVGStringList;

public:

  SVGStringList() : mIsSet(false), mIsCommaSeparated(false) {}
  ~SVGStringList(){}

  void SetIsCommaSeparated(bool aIsCommaSeparated) {
    mIsCommaSeparated = aIsCommaSeparated;
  }
  nsresult SetValue(const nsAString& aValue);

  void Clear() {
    mStrings.Clear();
    mIsSet = false;
  }

  
  void GetValue(nsAString& aValue) const;

  bool IsEmpty() const {
    return mStrings.IsEmpty();
  }

  uint32_t Length() const {
    return mStrings.Length();
  }

  const nsAString& operator[](uint32_t aIndex) const {
    return mStrings[aIndex];
  }

  bool operator==(const SVGStringList& rhs) const {
    return mStrings == rhs.mStrings;
  }

  bool SetCapacity(uint32_t size) {
    return mStrings.SetCapacity(size);
  }

  void Compact() {
    mStrings.Compact();
  }

  
  
  bool IsExplicitlySet() const
    { return mIsSet; }

  
  
  
  
  
  

protected:

  



  nsresult CopyFrom(const SVGStringList& rhs);

  nsAString& operator[](uint32_t aIndex) {
    return mStrings[aIndex];
  }

  



  bool SetLength(uint32_t aStringOfItems) {
    return mStrings.SetLength(aStringOfItems);
  }

private:

  
  
  

  bool InsertItem(uint32_t aIndex, const nsAString &aString) {
    if (aIndex >= mStrings.Length()) {
      aIndex = mStrings.Length();
    }
    if (mStrings.InsertElementAt(aIndex, aString)) {
      mIsSet = true;
      return true;
    }
    return false;
  }

  void ReplaceItem(uint32_t aIndex, const nsAString &aString) {
    NS_ABORT_IF_FALSE(aIndex < mStrings.Length(),
                      "DOM wrapper caller should have raised INDEX_SIZE_ERR");
    mStrings[aIndex] = aString;
  }

  void RemoveItem(uint32_t aIndex) {
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
  bool mIsCommaSeparated;
};

} 

#endif 
