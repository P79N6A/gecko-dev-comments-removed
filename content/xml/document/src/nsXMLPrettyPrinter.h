




































 
#ifndef nsXMLPrettyPrinter_h__
#define nsXMLPrettyPrinter_h__

#include "nsStubDocumentObserver.h"
#include "nsIDocument.h"
#include "nsCOMPtr.h"

class nsXMLPrettyPrinter : public nsStubDocumentObserver
{
public:
    nsXMLPrettyPrinter();
    virtual ~nsXMLPrettyPrinter();

    NS_DECL_ISUPPORTS

    
    virtual void BeginUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType);
    virtual void EndUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType);
    virtual void AttributeChanged(nsIDocument* aDocument, nsIContent* aContent,
                                  PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                  PRInt32 aModType);
    virtual void ContentAppended(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 PRInt32 aNewIndexInContainer);
    virtual void ContentInserted(nsIDocument* aDocument,
                                 nsIContent* aContainer, nsIContent* aChild,
                                 PRInt32 aIndexInContainer);
    virtual void ContentRemoved(nsIDocument* aDocument, nsIContent* aContainer,
                                nsIContent* aChild, PRInt32 aIndexInContainer);
    virtual void NodeWillBeDestroyed(const nsINode* aNode);
    
    







    nsresult PrettyPrint(nsIDocument* aDocument, PRBool* aDidPrettyPrint);

private:
    





    void MaybeUnhook(nsIContent* aContent);

    nsIDocument* mDocument; 
    PRUint32 mUpdateDepth;
    PRPackedBool mUnhookPending;
};

nsresult NS_NewXMLPrettyPrinter(nsXMLPrettyPrinter** aPrinter);

#endif 
