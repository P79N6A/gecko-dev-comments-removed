







































#include "CAccessibleEditableText.h"

#include "nsIAccessibleEditableText.h"
#include "AccessibleEditableText_i.c"

#include "nsCOMPtr.h"
#include "nsString.h"

#define GET_NSIACCESSIBLEEDITABLETEXT \
nsCOMPtr<nsIAccessibleEditableText> textAcc(do_QueryInterface(this));\
NS_ASSERTION(textAcc,\
             "Subclass of CAccessibleEditableText doesn't implement nsIAccessibleEditableText");\
if (!textAcc)\
  return E_FAIL;\



STDMETHODIMP
CAccessibleEditableText::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleEditableText == iid) {
    *ppv = NS_STATIC_CAST(IAccessibleEditableText*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef(); 
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleEditableText::copyText(long aStartOffset, long aEndOffset)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->CopyText(aStartOffset, aEndOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::deleteText(long aStartOffset, long aEndOffset)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->DeleteText(aStartOffset, aEndOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::insertText(long aOffset, BSTR *aText)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  PRUint32 length = ::SysStringLen(*aText);
  nsAutoString text(*aText, length);

  nsresult rv = textAcc->InsertText(text, aOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::cutText(long aStartOffset, long aEndOffset)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->CutText(aStartOffset, aEndOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::pasteText(long aOffset)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->PasteText(aOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::replaceText(long aStartOffset, long aEndOffset,
                                     BSTR *aText)
{
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->DeleteText(aStartOffset, aEndOffset);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = ::SysStringLen(*aText);
  nsAutoString text(*aText, length);

  rv = textAcc->InsertText(text, aStartOffset);
  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleEditableText::setAttributes(long aStartOffset, long aEndOffset,
                                       BSTR *aAttributes)
{
  return E_NOTIMPL;
}
