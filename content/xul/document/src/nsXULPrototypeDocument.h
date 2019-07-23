








































#ifndef nsXULPrototypeDocument_h__
#define nsXULPrototypeDocument_h__

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsISerializable.h"
#include "nsCycleCollectionParticipant.h"

class nsIAtom;
class nsIPrincipal;
class nsIURI;
class nsNodeInfoManager;
class nsXULDocument;
class nsXULPrototypeElement;
class nsXULPrototypePI;
class nsXULPDGlobalObject;








class nsXULPrototypeDocument : public nsIScriptGlobalObjectOwner,
                               public nsISerializable
{
public:
    static nsresult
    Create(nsIURI* aURI, nsXULPrototypeDocument** aResult);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    
    NS_DECL_NSISERIALIZABLE

    nsresult InitPrincipal(nsIURI* aURI, nsIPrincipal* aPrincipal);
    nsIURI* GetURI();

    


    nsXULPrototypeElement* GetRootElement();
    void SetRootElement(nsXULPrototypeElement* aElement);

    








    nsresult AddProcessingInstruction(nsXULPrototypePI* aPI);
    



    const nsTArray<nsXULPrototypePI*>& GetProcessingInstructions() const;

    







    void AddStyleSheetReference(nsIURI* aStyleSheet);
    const nsCOMArray<nsIURI>& GetStyleSheetReferences() const;

    



    NS_IMETHOD GetHeaderData(nsIAtom* aField, nsAString& aData) const;
    NS_IMETHOD SetHeaderData(nsIAtom* aField, const nsAString& aData);

    nsIPrincipal *DocumentPrincipal();
    void SetDocumentPrincipal(nsIPrincipal *aPrincipal);

    





    nsresult AwaitLoadDone(nsXULDocument* aDocument, PRBool* aResult);

    





    nsresult NotifyLoadDone();

    nsNodeInfoManager *GetNodeInfoManager();

    
    virtual nsIScriptGlobalObject* GetScriptGlobalObject();

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULPrototypeDocument,
                                             nsIScriptGlobalObjectOwner)

protected:
    nsCOMPtr<nsIURI> mURI;
    nsXULPrototypeElement* mRoot;
    nsTArray<nsXULPrototypePI*> mProcessingInstructions;
    nsCOMArray<nsIURI> mStyleSheetReferences;

    nsRefPtr<nsXULPDGlobalObject> mGlobalObject;

    PRPackedBool mLoaded;
    nsTArray< nsRefPtr<nsXULDocument> > mPrototypeWaiters;

    nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

    nsXULPrototypeDocument();
    virtual ~nsXULPrototypeDocument();
    nsresult Init();

    friend NS_IMETHODIMP
    NS_NewXULPrototypeDocument(nsXULPrototypeDocument** aResult);

    nsXULPDGlobalObject *NewXULPDGlobalObject();

    static nsIPrincipal* gSystemPrincipal;
    static nsXULPDGlobalObject* gSystemGlobal;
    static PRUint32 gRefCnt;

    friend class nsXULPDGlobalObject;
};

#endif 
