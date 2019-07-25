











































#ifndef nsDOMLists_h___
#define nsDOMLists_h___

#include "nsIDOMDOMStringList.h"
#include "nsIDOMNameList.h"
#include "nsTArray.h"
#include "nsString.h"

class nsDOMStringList : public nsIDOMDOMStringList
{
public:
  nsDOMStringList();
  virtual ~nsDOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  PRBool Add(const nsAString& aName)
  {
    return mNames.AppendElement(aName) != nsnull;
  }

private:
  nsTArray<nsString> mNames;
};

class nsNameList : public nsIDOMNameList
{
public:
  nsNameList();
  virtual ~nsNameList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAMELIST

  PRBool Add(const nsAString& aNamespaceURI, const nsAString& aName);

private:
  nsTArray<nsString> mNamespaceURIs;
  nsTArray<nsString> mNames;
};

#endif 
