




































#ifndef _nsClipboard_h
#define _nsClipboard_h

#include "nsBaseClipboard.h"
class nsITransferable;





struct FormatRecord;

class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, bool *_retval);

protected:
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable *aTransferable, PRInt32 aWhichClipboard);

  enum ClipboardAction
  {
    Read,
    Write
  };

  PRUint32 GetFormatID(const char *aMimeStr);
  bool     GetClipboardData(const char *aFlavour);
  bool     GetClipboardDataByID(PRUint32 aFormatID, const char *aFlavor);
  void     SetClipboardData(const char *aFlavour);
  nsresult DoClipboardAction(ClipboardAction aAction);
};

#endif
