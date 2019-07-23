












































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"

#include "Types.h"

class nsITransferable;


class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_IMETHOD  HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard, PRBool *_retval); 

protected:

  
  NS_IMETHOD SetNativeClipboardData ( PRInt32 aWhichClipboard );
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard );

  
  
  nsresult GetDataOffClipboard ( ResType inMacFlavor, void** outData, PRInt32* outDataSize ) ;

  
  PRBool CheckIfFlavorPresent ( ResType inMacFlavor ) ;

  
  nsresult PutOnClipboard ( ResType inFlavor, const void* inData, PRInt32 inLen ) ;

}; 

#endif 
