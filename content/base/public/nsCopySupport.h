




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
class nsILoadContext;

class nsCopySupport
{
  
  public:
    static nsresult HTMLCopy(nsISelection *aSel, nsIDocument *aDoc, int16_t aClipboardID);
    static nsresult DoHooks(nsIDocument *aDoc, nsITransferable *aTrans,
                            bool *aDoPutOnClipboard);
    static nsresult IsPlainTextContext(nsISelection *aSel, nsIDocument *aDoc, bool *aIsPlainTextContext);

    
    
    
    static nsresult GetContents(const nsACString& aMimeType, uint32_t aFlags, nsISelection *aSel, nsIDocument *aDoc, nsAString& outdata);
    
    static nsresult ImageCopy(nsIImageLoadingContent* aImageElement,
                              nsILoadContext* aLoadContext,
                              int32_t aCopyFlags);

    
    
    static nsresult GetTransferableForSelection(nsISelection* aSelection,
                                                nsIDocument* aDocument,
                                                nsITransferable** aTransferable);

    
    static nsresult GetTransferableForNode(nsINode* aNode,
                                           nsIDocument* aDoc,
                                           nsITransferable** aTransferable);
    





    static nsIContent* GetSelectionForCopy(nsIDocument* aDocument,
                                           nsISelection** aSelection);

    



    static bool CanCopy(nsIDocument* aDocument);

    




















    static bool FireClipboardEvent(int32_t aType,
                                     nsIPresShell* aPresShell,
                                     nsISelection* aSelection);
};

#endif
