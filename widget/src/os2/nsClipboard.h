




































#ifndef _nsClipboard_h
#define _nsClipboard_h

#include "nsBaseClipboard.h"
#include "nsIObserver.h"

#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>

class nsITransferable;





struct FormatRecord;

class nsClipboard : public nsBaseClipboard,
		    public nsIObserver
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, PRBool *_retval);

protected:
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable *aTransferable, PRInt32 aWhichClipboard);

  enum ClipboardAction
  {
    Read,
    Write
  };

  ULONG    GetFormatID(const char *aMimeStr);
  PRBool   GetClipboardData(const char *aFlavour);
  PRBool   GetClipboardDataByID(ULONG ulFormatID, const char *aFlavor);
  void     SetClipboardData(const char *aFlavour);
  nsresult DoClipboardAction(ClipboardAction aAction);
};

#endif
