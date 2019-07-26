








#ifndef mozilla_dom_DOMStringList_h
#define mozilla_dom_DOMStringList_h

#include "nsIDOMDOMStringList.h"
#include "nsTArray.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class DOMStringList : public nsIDOMDOMStringList
{
public:
  DOMStringList();
  virtual ~DOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  bool Add(const nsAString& aName)
  {
    return mNames.AppendElement(aName) != nullptr;
  }

  void Clear()
  {
    mNames.Clear();
  }

  void CopyList(nsTArray<nsString>& aNames)
  {
    aNames = mNames;
  }

private:
  nsTArray<nsString> mNames;
};

} 
} 

#endif 
