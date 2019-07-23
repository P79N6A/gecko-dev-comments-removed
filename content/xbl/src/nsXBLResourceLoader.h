





































#include "nsCOMPtr.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"

class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScriptContext;
class nsSupportsHashtable;
class nsXBLPrototypeResources;
class nsXBLPrototypeBinding;




struct nsXBLResource {
  nsXBLResource* mNext;
  nsIAtom* mType;
  nsString mSrc;

  nsXBLResource(nsIAtom* aType, const nsAString& aSrc) {
    MOZ_COUNT_CTOR(nsXBLResource);
    mNext = nsnull;
    mType = aType;
    mSrc = aSrc;
  }

  ~nsXBLResource() { 
    MOZ_COUNT_DTOR(nsXBLResource);  
    delete mNext; 
  }
};

class nsXBLResourceLoader : public nsICSSLoaderObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsXBLResourceLoader)

  
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);

  void LoadResources(PRBool* aResult);
  void AddResource(nsIAtom* aResourceType, const nsAString& aSrc);
  void AddResourceListener(nsIContent* aElement);

  nsXBLResourceLoader(nsXBLPrototypeBinding* aBinding,
                      nsXBLPrototypeResources* aResources);
  virtual ~nsXBLResourceLoader();

  void NotifyBoundElements();


  nsXBLPrototypeBinding* mBinding; 
  nsXBLPrototypeResources* mResources; 
                                       
                                       
                                       
  
  nsXBLResource* mResourceList; 
  nsXBLResource* mLastResource;

  PRPackedBool mLoadingResources;
  
  
  PRPackedBool mInLoadResourcesFunc;
  PRInt16 mPendingSheets; 

  
  nsCOMArray<nsIContent> mBoundElements;
};

