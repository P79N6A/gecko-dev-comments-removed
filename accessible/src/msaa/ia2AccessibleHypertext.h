






#ifndef _ACCESSIBLE_HYPERTEXT_H
#define _ACCESSIBLE_HYPERTEXT_H

#include "nsISupports.h"

#include "CAccessibleText.h"
#include "AccessibleHypertext.h"

class ia2AccessibleHypertext : public CAccessibleText,
                               public IAccessibleHypertext
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  FORWARD_IACCESSIBLETEXT(CAccessibleText)

  
  virtual  HRESULT STDMETHODCALLTYPE get_nHyperlinks(
       long* hyperlinkCount);

  virtual  HRESULT STDMETHODCALLTYPE get_hyperlink(
       long index,
       IAccessibleHyperlink** hyperlink);

  virtual  HRESULT STDMETHODCALLTYPE get_hyperlinkIndex(
       long charIndex,
       long* hyperlinkIndex);

  
  NS_IMETHOD QueryInterface(const nsIID& uuid, void** result) = 0;
};

#endif

