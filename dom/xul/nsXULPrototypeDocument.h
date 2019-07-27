




#ifndef nsXULPrototypeDocument_h__
#define nsXULPrototypeDocument_h__

#include "js/TracingAPI.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsISerializable.h"
#include "nsCycleCollectionParticipant.h"

class nsIAtom;
class nsIPrincipal;
class nsIURI;
class nsNodeInfoManager;
class nsXULPrototypeElement;
class nsXULPrototypePI;

namespace mozilla {
namespace dom {
class XULDocument;
} 
} 








class nsXULPrototypeDocument MOZ_FINAL : public nsISerializable
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
    



    const nsTArray<nsRefPtr<nsXULPrototypePI> >& GetProcessingInstructions() const;

    







    void AddStyleSheetReference(nsIURI* aStyleSheet);
    const nsCOMArray<nsIURI>& GetStyleSheetReferences() const;

    



    NS_IMETHOD GetHeaderData(nsIAtom* aField, nsAString& aData) const;
    NS_IMETHOD SetHeaderData(nsIAtom* aField, const nsAString& aData);

    nsIPrincipal *DocumentPrincipal();
    void SetDocumentPrincipal(nsIPrincipal *aPrincipal);

    





    nsresult AwaitLoadDone(mozilla::dom::XULDocument* aDocument, bool* aResult);

    





    nsresult NotifyLoadDone();

    nsNodeInfoManager *GetNodeInfoManager();

    void MarkInCCGeneration(uint32_t aCCGeneration);

    NS_DECL_CYCLE_COLLECTION_CLASS(nsXULPrototypeDocument)

    void TraceProtos(JSTracer* aTrc, uint32_t aGCNumber);

protected:
    nsCOMPtr<nsIURI> mURI;
    nsRefPtr<nsXULPrototypeElement> mRoot;
    nsTArray<nsRefPtr<nsXULPrototypePI> > mProcessingInstructions;
    nsCOMArray<nsIURI> mStyleSheetReferences;

    bool mLoaded;
    nsTArray< nsRefPtr<mozilla::dom::XULDocument> > mPrototypeWaiters;

    nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

    uint32_t mCCGeneration;
    uint32_t mGCNumber;

    nsXULPrototypeDocument();
    virtual ~nsXULPrototypeDocument();
    nsresult Init();

    friend NS_IMETHODIMP
    NS_NewXULPrototypeDocument(nsXULPrototypeDocument** aResult);

    static uint32_t gRefCnt;
};

#endif 
