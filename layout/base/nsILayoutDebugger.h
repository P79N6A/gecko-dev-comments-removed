






































#ifndef nsILayoutDebugger_h___
#define nsILayoutDebugger_h___

#include "nsISupports.h"

class nsIDocument;
class nsIPresShell;


#define NS_ILAYOUT_DEBUGGER_IID \
 { 0xa6cf90f8, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}




class nsILayoutDebugger : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILAYOUT_DEBUGGER_IID)

  NS_IMETHOD SetShowFrameBorders(PRBool aEnable) = 0;

  NS_IMETHOD GetShowFrameBorders(PRBool* aResult) = 0;

  NS_IMETHOD SetShowEventTargetFrameBorder(PRBool aEnable) = 0;

  NS_IMETHOD GetShowEventTargetFrameBorder(PRBool* aResult) = 0;

  NS_IMETHOD GetContentSize(nsIDocument* aDocument,
                            PRInt32* aSizeInBytesResult) = 0;

  NS_IMETHOD GetFrameSize(nsIPresShell* aPresentation,
                          PRInt32* aSizeInBytesResult) = 0;

  NS_IMETHOD GetStyleSize(nsIPresShell* aPresentation,
                          PRInt32* aSizeInBytesResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILayoutDebugger, NS_ILAYOUT_DEBUGGER_IID)

#endif 
