





































#ifndef nsClipboard_h_
#define nsClipboard_h_

#include "nsBaseClipboard.h"
#include "nsXPIDLString.h"

#import <Cocoa/Cocoa.h>

class nsITransferable;

class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, bool *_retval);

  
  static NSDictionary* PasteboardDictFromTransferable(nsITransferable *aTransferable);
  static bool IsStringType(const nsCString& aMIMEType, NSString** aPasteboardType);
  static NSString* WrapHtmlForSystemPasteboard(NSString* aString);
  static nsresult TransferableFromPasteboard(nsITransferable *aTransferable, NSPasteboard *pboard);

protected:

  
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable, PRInt32 aWhichClipboard);
  
private:
  int mChangeCount; 

};

#endif 
