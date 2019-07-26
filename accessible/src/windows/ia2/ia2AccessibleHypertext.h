






#ifndef _ACCESSIBLE_HYPERTEXT_H
#define _ACCESSIBLE_HYPERTEXT_H

#include "nsISupports.h"

#include "ia2AccessibleText.h"
#include "AccessibleHypertext.h"

namespace mozilla {
namespace a11y {

class ia2AccessibleHypertext : public ia2AccessibleText,
                               public IAccessibleHypertext
{
public:

  
  FORWARD_IACCESSIBLETEXT(ia2AccessibleText)

  
  virtual  HRESULT STDMETHODCALLTYPE get_nHyperlinks(
       long* hyperlinkCount);

  virtual  HRESULT STDMETHODCALLTYPE get_hyperlink(
       long index,
       IAccessibleHyperlink** hyperlink);

  virtual  HRESULT STDMETHODCALLTYPE get_hyperlinkIndex(
       long charIndex,
       long* hyperlinkIndex);
};

} 
} 

#endif
