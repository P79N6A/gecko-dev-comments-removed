



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

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, uint32_t aLength,
                                    int32_t aWhichClipboard, bool *_retval);

protected:
  NS_IMETHOD SetNativeClipboardData(int32_t aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable *aTransferable, int32_t aWhichClipboard);

  enum ClipboardAction
  {
    Read,
    Write
  };

  uint32_t GetFormatID(const char *aMimeStr);
  bool     GetClipboardData(const char *aFlavour);
  bool     GetClipboardDataByID(uint32_t aFormatID, const char *aFlavor);
  void     SetClipboardData(const char *aFlavour);
  nsresult DoClipboardAction(ClipboardAction aAction);
};

#endif
