




#ifndef nsBaseClipboard_h__
#define nsBaseClipboard_h__

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsClipboardPrivacyHandler.h"
#include "nsAutoPtr.h"

class nsITransferable;
class nsDataObj;
class nsIClipboardOwner;
class nsIWidget;





class nsBaseClipboard : public nsIClipboard
{

public:
  nsBaseClipboard();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICLIPBOARD
  
protected:
  virtual ~nsBaseClipboard();

  NS_IMETHOD SetNativeClipboardData ( int32_t aWhichClipboard ) = 0;
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, int32_t aWhichClipboard ) = 0;

  bool                mEmptyingForSetData;
  bool                mIgnoreEmptyNotification;
  nsIClipboardOwner * mClipboardOwner;
  nsITransferable   * mTransferable;
  nsRefPtr<nsClipboardPrivacyHandler> mPrivacyHandler;

};

#endif 

