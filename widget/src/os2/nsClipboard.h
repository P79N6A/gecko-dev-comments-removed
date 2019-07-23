




































#ifndef _nsClipboard_h
#define _nsClipboard_h

#include "nsWidgetDefs.h"
#include "nsBaseClipboard.h"
#include "nsIObserver.h"

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;
struct IDataObject;





struct FormatRecord;

class nsClipboard : public nsBaseClipboard,
		    public nsIObserver
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard, PRBool *_retval);

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
