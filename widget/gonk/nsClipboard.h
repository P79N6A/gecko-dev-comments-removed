




#ifndef nsClipbard_h__
#define nsClipbard_h__

#include "nsIClipboard.h"

class nsClipboard final : public nsIClipboard
{
  nsAutoString mClipboard;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARD

  nsClipboard();

protected:
  ~nsClipboard() {}
};

#endif
