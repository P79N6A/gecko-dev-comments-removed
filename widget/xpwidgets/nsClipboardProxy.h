




#ifndef NS_CLIPBOARD_PROXY_H
#define NS_CLIPBOARD_PROXY_H

#include "nsIClipboard.h"

class nsClipboardProxy MOZ_FINAL : public nsIClipboard
{
  ~nsClipboardProxy() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARD

  nsClipboardProxy();
};

#endif
