




#ifndef nsClipbard_h__
#define nsClipbard_h__

#include "nsIClipboard.h"

class nsClipboard MOZ_FINAL : public nsIClipboard
{
  nsAutoString mClipboard;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARD

  nsClipboard();
};

#endif
