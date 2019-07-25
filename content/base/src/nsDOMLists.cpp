











































#include "nsDOMLists.h"
#include "nsDOMError.h"
#include "nsIDOMClassInfo.h"
#include "nsContentUtils.h"
#include "nsINode.h"

nsDOMStringList::nsDOMStringList()
{
}

nsDOMStringList::~nsDOMStringList()
{
}

DOMCI_DATA(DOMStringList, nsDOMStringList)

NS_IMPL_ADDREF(nsDOMStringList)
NS_IMPL_RELEASE(nsDOMStringList)
NS_INTERFACE_TABLE_HEAD(nsDOMStringList)
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsDOMStringList)
    NS_INTERFACE_TABLE_ENTRY(nsDOMStringList, nsIDOMDOMStringList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMStringList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMStringList::Item(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= mNames.Length()) {
    SetDOMStringToNull(aResult);
  } else {
    aResult = mNames[aIndex];
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStringList::GetLength(PRUint32 *aLength)
{
  *aLength = mNames.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStringList::Contains(const nsAString& aString, bool *aResult)
{
  *aResult = mNames.Contains(aString);

  return NS_OK;
}


nsNameList::nsNameList()
{
}

nsNameList::~nsNameList()
{
}
DOMCI_DATA(NameList, nsNameList)

NS_IMPL_ADDREF(nsNameList)
NS_IMPL_RELEASE(nsNameList)
NS_INTERFACE_TABLE_HEAD(nsNameList)
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsNameList)
    NS_INTERFACE_TABLE_ENTRY(nsNameList, nsIDOMNameList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NameList)
  NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsNameList::GetName(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= mNames.Length()) {
    SetDOMStringToNull(aResult);
  } else {
    aResult = mNames[aIndex];
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::GetNamespaceURI(PRUint32 aIndex, nsAString& aResult)
{
  if (aIndex >= mNames.Length()) {
    SetDOMStringToNull(aResult);
  } else {
    aResult = mNamespaceURIs[aIndex];
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::GetLength(PRUint32 *aLength)
{
  *aLength = mNames.Length();

  return NS_OK;
}

bool
nsNameList::Add(const nsAString& aNamespaceURI, const nsAString& aName)
{
  PRUint32 count = mNamespaceURIs.Length();
  if (mNamespaceURIs.InsertElementAt(count, aNamespaceURI)) {
    if (mNames.InsertElementAt(count, aName)) {
      return PR_TRUE;
    }
    mNamespaceURIs.RemoveElementAt(count);
  }

  return PR_FALSE;
}

NS_IMETHODIMP
nsNameList::Contains(const nsAString& aName, bool *aResult)
{
  *aResult = mNames.Contains(aName);

  return NS_OK;
}

NS_IMETHODIMP
nsNameList::ContainsNS(const nsAString& aNamespaceURI, const nsAString& aName,
                       bool *aResult)
{
  PRUint32 index = mNames.IndexOf(aName);
  if (index != mNames.NoIndex) {
    *aResult = mNamespaceURIs[index].Equals(aNamespaceURI);
  }
  else {
    *aResult = PR_FALSE;
  }

  return NS_OK;
}
