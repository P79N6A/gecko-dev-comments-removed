




































#include "nsHtml5Atom.h"

nsHtml5Atom::nsHtml5Atom(const nsAString& aString)
  : mData(aString)
{
}

nsHtml5Atom::~nsHtml5Atom()
{
}

NS_IMETHODIMP_(nsrefcnt)
nsHtml5Atom::AddRef()
{
  NS_NOTREACHED("Attempt to AddRef an nsHtml5Atom.");
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsHtml5Atom::Release()
{
  NS_NOTREACHED("Attempt to Release an nsHtml5Atom.");
  return 1;
}

NS_IMETHODIMP
nsHtml5Atom::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_NOTREACHED("Attempt to call QueryInterface an nsHtml5Atom.");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHtml5Atom::ToString(nsAString& aReturn)
{
  aReturn.Assign(mData);
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Atom::ToUTF8String(nsACString& aReturn)
{
  NS_NOTREACHED("Should not attempt to convert to an UTF-8 string.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Atom::GetUTF8String(const char **aReturn)
{
  NS_NOTREACHED("Should not attempt to get a UTF-8 string from nsHtml5Atom");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(PRBool)
nsHtml5Atom::IsStaticAtom()
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsHtml5Atom::Equals(const nsAString& aString, PRBool *aReturn)
{
  *aReturn = mData.Equals(aString);
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Atom::EqualsUTF8(const nsACString& aString, PRBool *aReturn)
{
  NS_NOTREACHED("Should not attempt to compare with an UTF-8 string.");
  return NS_ERROR_NOT_IMPLEMENTED;
}
