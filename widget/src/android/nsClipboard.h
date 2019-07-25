



































#ifndef NS_CLIPBOARD_H
#define NS_CLIPBOARD_H

#include "nsIClipboard.h"

class nsClipboard : public nsIClipboard
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARD

  nsClipboard();
};

#endif
