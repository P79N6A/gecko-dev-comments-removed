





#ifndef nsClipboardHelper_h__
#define nsClipboardHelper_h__


#include "nsIClipboardHelper.h"


#include "nsString.h"





class nsClipboardHelper : public nsIClipboardHelper
{
  virtual ~nsClipboardHelper();

public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARDHELPER

  nsClipboardHelper();
};

#endif 
