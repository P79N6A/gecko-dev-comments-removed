







































#include "CAccessibleEditableText.h"

#include "nsIAccessibleEditableText.h"
#include "AccessibleEditableText_i.c"
#include "nsAccessNodeWrap.h"

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
    nsCOMPtr<nsIAccessibleEditableText> editTextAcc(do_QueryInterface(this));
    if (!editTextAcc)
      return E_NOINTERFACE;
    *ppv = static_cast<IAccessibleEditableText*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef(); 
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleEditableText::copyText(long aStartOffset, long aEndOffset)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->CopyText(aStartOffset, aEndOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::deleteText(long aStartOffset, long aEndOffset)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->DeleteText(aStartOffset, aEndOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::insertText(long aOffset, BSTR *aText)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  PRUint32 length = ::SysStringLen(*aText);
  nsAutoString text(*aText, length);

  nsresult rv = textAcc->InsertText(text, aOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::cutText(long aStartOffset, long aEndOffset)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->CutText(aStartOffset, aEndOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::pasteText(long aOffset)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->PasteText(aOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::replaceText(long aStartOffset, long aEndOffset,
                                     BSTR *aText)
{
__try {
  GET_NSIACCESSIBLEEDITABLETEXT

  nsresult rv = textAcc->DeleteText(aStartOffset, aEndOffset);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = ::SysStringLen(*aText);
  nsAutoString text(*aText, length);

  rv = textAcc->InsertText(text, aStartOffset);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleEditableText::setAttributes(long aStartOffset, long aEndOffset,
                                       BSTR *aAttributes)
{
  return E_NOTIMPL;
}
