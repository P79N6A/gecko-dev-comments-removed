




































 
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

    
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

    







    nsresult PrettyPrint(nsIDocument* aDocument, PRBool* aDidPrettyPrint);

    


    void Unhook();
private:
    





    void MaybeUnhook(nsIContent* aContent);

    nsIDocument* mDocument; 
    PRUint32 mUpdateDepth;
    PRPackedBool mUnhookPending;
};

nsresult NS_NewXMLPrettyPrinter(nsXMLPrettyPrinter** aPrinter);

#endif 
