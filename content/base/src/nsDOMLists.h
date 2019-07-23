











































#ifndef nsDOMLists_h___
#define nsDOMLists_h___

#include "nsIDOMDOMStringList.h"
#include "nsIDOMNameList.h"
#include "nsVoidArray.h"

class nsDOMStringList : public nsIDOMDOMStringList
{
public:
  nsDOMStringList();
  virtual ~nsDOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  PRBool Add(const nsAString& aName)
  {
    return mNames.AppendString(aName);
  }

private:
  nsStringArray mNames;
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
  nsStringArray mNamespaceURIs;
  nsStringArray mNames;
};

#endif 
