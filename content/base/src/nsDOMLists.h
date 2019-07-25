










































#ifndef nsDOMLists_h___
#define nsDOMLists_h___

#include "nsIDOMDOMStringList.h"
#include "nsTArray.h"
#include "nsString.h"

class nsDOMStringList : public nsIDOMDOMStringList
{
public:
  nsDOMStringList();
  virtual ~nsDOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  bool Add(const nsAString& aName)
  {
    return mNames.AppendElement(aName) != nsnull;
  }

private:
  nsTArray<nsString> mNames;
};

#endif 
