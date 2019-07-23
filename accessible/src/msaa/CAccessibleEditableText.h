







































#ifndef _ACCESSIBLE_EDITABLETEXT_H
#define _ACCESSIBLE_EDITABLETEXT_H

#include "nsISupports.h"
#include "nsIAccessibleEditableText.h"

#include "AccessibleEditableText.h"

class CAccessibleEditableText: public nsISupports,
                               public IAccessibleEditableText
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual HRESULT STDMETHODCALLTYPE copyText(
       long startOffset,
       long endOffset);

  virtual HRESULT STDMETHODCALLTYPE deleteText(
       long startOffset,
       long endOffset);

  virtual HRESULT STDMETHODCALLTYPE insertText(
       long offset,
       BSTR *text);

  virtual HRESULT STDMETHODCALLTYPE cutText(
       long startOffset,
       long endOffset);

  virtual HRESULT STDMETHODCALLTYPE pasteText(
       long offset);

  virtual HRESULT STDMETHODCALLTYPE replaceText(
       long startOffset,
       long endOffset,
       BSTR *text);

  virtual HRESULT STDMETHODCALLTYPE setAttributes(
       long startOffset,
       long endOffset,
       BSTR *attributes);
};

#endif

