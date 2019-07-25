






































#ifndef nsClipboardHelper_h__
#define nsClipboardHelper_h__


#include "nsIClipboardHelper.h"


#include "nsString.h"





class nsClipboardHelper : public nsIClipboardHelper
{

public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARDHELPER

  nsClipboardHelper();
  virtual ~nsClipboardHelper();

};

#endif 
