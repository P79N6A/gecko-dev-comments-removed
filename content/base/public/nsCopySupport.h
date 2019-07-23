




































#ifndef nsCopySupport_h__
#define nsCopySupport_h__

#include "nscore.h"

class nsISelection;
class nsIDocument;
class nsIImageLoadingContent;
class nsIContent;
class nsITransferable;
class nsACString;
class nsAString;

class nsCopySupport
{
  
  public:
    static nsresult HTMLCopy(nsISelection *aSel, nsIDocument *aDoc, PRInt16 aClipboardID);
    static nsresult DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                            PRBool *aDoPutOnClipboard);
    static nsresult IsPlainTextContext(nsISelection *aSel, nsIDocument *aDoc, PRBool *aIsPlainTextContext);

    
    
    
    static nsresult GetContents(const nsACString& aMimeType, PRUint32 aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata);
    
    static nsresult ImageCopy(nsIImageLoadingContent* aImageElement,
                              PRInt32 aCopyFlags);
};

#endif
