






#ifndef _NSHTMLIMAGEACCESSIBLEWRAP_H
#define _NSHTMLIMAGEACCESSIBLEWRAP_H

#include "nsHTMLImageAccessible.h"
#include "ia2AccessibleImage.h"

class nsHTMLImageAccessibleWrap : public nsHTMLImageAccessible,
                                  public ia2AccessibleImage
{
public:
  nsHTMLImageAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    nsHTMLImageAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

