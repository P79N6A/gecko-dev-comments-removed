




































#ifndef nsCopySupport_h__
#define nsCopySupport_h__

#include "nscore.h"
#include "nsINode.h"

class nsISelection;
class nsIDocument;
class nsIImageLoadingContent;
class nsIContent;
class nsITransferable;
class nsACString;
class nsAString;
class nsIPresShell;

class nsCopySupport
{
  
  public:
    static nsresult HTMLCopy(nsISelection *aSel, nsIDocument *aDoc, PRInt16 aClipboardID);
    static nsresult DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                            bool *aDoPutOnClipboard);
    static nsresult IsPlainTextContext(nsISelection *aSel, nsIDocument *aDoc, bool *aIsPlainTextContext);

    
    
    
    static nsresult GetContents(const nsACString& aMimeType, PRUint32 aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata);
    
    static nsresult ImageCopy(nsIImageLoadingContent* aImageElement,
                              PRInt32 aCopyFlags);

    
    
    static nsresult GetTransferableForSelection(nsISelection* aSelection,
                                                nsIDocument* aDocument,
                                                nsITransferable** aTransferable);

    
    static nsresult GetTransferableForNode(nsINode* aNode,
                                           nsIDocument* aDoc,
                                           nsITransferable** aTransferable);
    





    static nsIContent* GetSelectionForCopy(nsIDocument* aDocument,
                                           nsISelection** aSelection);

    



    static bool CanCopy(nsIDocument* aDocument);

    




















    static bool FireClipboardEvent(PRInt32 aType,
                                     nsIPresShell* aPresShell,
                                     nsISelection* aSelection);
};

#endif
