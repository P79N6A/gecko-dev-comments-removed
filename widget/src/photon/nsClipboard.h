







































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include <Pt.h>

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"
#include <nsCOMPtr.h>

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;





class nsClipboard : public nsIClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICLIPBOARD

  NS_IMETHOD SetInputGroup(PRInt32 aInputGroup)
  {
    mInputGroup = aInputGroup;
    return NS_OK;
  }


protected:
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable, 
                                    PRInt32 aWhichClipboard );
nsresult GetFormat(const char* aMimeStr, char *format );

  PRBool  mIgnoreEmptyNotification;
	inline nsITransferable *GetTransferable(PRInt32 aWhichClipboard);

private:
  unsigned long GetFlavourTimestamp( char *type );
  nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
  nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
  nsCOMPtr<nsITransferable>   mSelectionTransferable;
  nsCOMPtr<nsITransferable>   mGlobalTransferable;

  
  
  PRBool mBlocking;
  
  PRInt32 mInputGroup;
};

#endif 
