







































#ifndef _NSHTMLIMAGEACCESSIBLEWRAP_H
#define _NSHTMLIMAGEACCESSIBLEWRAP_H

#include "nsHTMLImageAccessible.h"
#include "CAccessibleImage.h"

class nsHTMLImageAccessibleWrap : public nsHTMLImageAccessible,
                                  public CAccessibleImage
{
public:
  nsHTMLImageAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHTMLImageAccessible(aNode, aShell){}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

