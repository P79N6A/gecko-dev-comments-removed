










































#ifndef _nsFontList_H_
#define _nsFontList_H_

#include "nsIFontList.h"
#include "gfxCore.h"

#define NS_FONTLIST_CONTRACTID "@mozilla.org/gfx/fontlist;1"


class NS_GFX nsFontList : public nsIFontList
{
public:
  nsFontList();
  virtual ~nsFontList();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIFONTLIST

protected:

};

#endif
