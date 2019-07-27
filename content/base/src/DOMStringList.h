




#ifndef mozilla_dom_DOMStringList_h
#define mozilla_dom_DOMStringList_h

#include "nsISupports.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class DOMStringList : public nsISupports,
                      public nsWrapperCache
{
protected:
  virtual ~DOMStringList();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMStringList)

  virtual JSObject* WrapObject(JSContext* aCx);
  nsISupports* GetParentObject()
  {
    return nullptr;
  }

  void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aResult)
  {
    EnsureFresh();
    if (aIndex < mNames.Length()) {
      aFound = true;
      aResult = mNames[aIndex];
    } else {
      aFound = false;
    }
  }

  void Item(uint32_t aIndex, nsAString& aResult)
  {
    EnsureFresh();
    if (aIndex < mNames.Length()) {
      aResult = mNames[aIndex];
    } else {
      aResult.SetIsVoid(true);
    }
  }

  uint32_t Length()
  {
    EnsureFresh();
    return mNames.Length();
  }

  bool Contains(const nsAString& aString)
  {
    EnsureFresh();
    return mNames.Contains(aString);
  }

  bool Add(const nsAString& aName)
  {
    
    
    return mNames.AppendElement(aName) != nullptr;
  }

  void Clear()
  {
    mNames.Clear();
  }

  nsTArray<nsString>& StringArray()
  {
    return mNames;
  }

  void CopyList(nsTArray<nsString>& aNames)
  {
    aNames = mNames;
  }

protected:
  
  
  virtual void EnsureFresh()
  {
  }

  
  
  nsTArray<nsString> mNames;
};

} 
} 

#endif 
