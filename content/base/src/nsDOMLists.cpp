











































#include "nsDOMLists.h"
#include "nsDOMError.h"
#include "nsIDOMClassInfo.h"
#include "nsContentUtils.h"

nsDOMStringList::nsDOMStringList()
{
}

nsDOMStringList::~nsDOMStringList()
{
}

NS_IMPL_ADDREF(nsDOMStringList)
NS_IMPL_RELEASE(nsDOMStringList)
NS_INTERFACE_MAP_BEGIN(nsDOMStringList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMStringList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMStringList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMStringList::Item(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= (PRUint32)mNames.Count()) {
    SetDOMStringToNull(aResult);
  } else {
    mNames.StringAt(aIndex, aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStringList::GetLength(PRUint32 *aLength)
{
  *aLength = (PRUint32)mNames.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStringList::Contains(const nsAString& aString, PRBool *aResult)
{
  *aResult = mNames.IndexOf(aString) > -1;

  return NS_OK;
}


nsNameList::nsNameList()
{
}

nsNameList::~nsNameList()
{
}

NS_IMPL_ADDREF(nsNameList)
  NS_IMPL_RELEASE(nsNameList)
  NS_INTERFACE_MAP_BEGIN(nsNameList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNameList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NameList)
  NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsNameList::GetName(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= (PRUint32)mNames.Count()) {
    SetDOMStringToNull(aResult);
  } else {
    mNames.StringAt(aIndex, aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::GetNamespaceURI(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= (PRUint32)mNames.Count()) {
    SetDOMStringToNull(aResult);
  } else {
    mNamespaceURIs.StringAt(aIndex, aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::GetLength(PRUint32 *aLength)
{
  *aLength = (PRUint32)mNames.Count();

  return NS_OK;
}

PRBool
nsNameList::Add(const nsAString& aNamespaceURI, const nsAString& aName)
{
  PRInt32 count = mNamespaceURIs.Count();
  if (mNamespaceURIs.InsertStringAt(aNamespaceURI, count)) {
    if (mNames.InsertStringAt(aName, count)) {
      return PR_TRUE;
    }
    mNamespaceURIs.RemoveStringAt(count);
  }

  return PR_FALSE;
}

NS_IMETHODIMP
nsNameList::Contains(const nsAString& aName, PRBool *aResult)
{
  *aResult = mNames.IndexOf(aName) > -1;

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::ContainsNS(const nsAString& aNamespaceURI, const nsAString& aName,
                       PRBool *aResult)
{
  PRInt32 index = mNames.IndexOf(aName);
  if (index > -1) {
    nsAutoString ns;
    mNamespaceURIs.StringAt(index, ns);

    *aResult = ns.Equals(aNamespaceURI);
  }
  else {
    *aResult = PR_FALSE;
  }

  return NS_OK;
}
