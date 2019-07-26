




#ifndef nsXBLResourceLoader_h
#define nsXBLResourceLoader_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"

class nsIContent;
class nsIAtom;
class nsSupportsHashtable;
class nsXBLPrototypeResources;
class nsXBLPrototypeBinding;
struct nsXBLResource;
class nsIObjectOutputStream;




class nsXBLResourceLoader : public nsICSSLoaderObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsXBLResourceLoader)

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet, bool aWasAlternate,
                              nsresult aStatus) MOZ_OVERRIDE;

  void LoadResources(bool* aResult);
  void AddResource(nsIAtom* aResourceType, const nsAString& aSrc);
  void AddResourceListener(nsIContent* aElement);

  nsXBLResourceLoader(nsXBLPrototypeBinding* aBinding,
                      nsXBLPrototypeResources* aResources);
  virtual ~nsXBLResourceLoader();

  void NotifyBoundElements();

  nsresult Write(nsIObjectOutputStream* aStream);


  nsXBLPrototypeBinding* mBinding; 
  nsXBLPrototypeResources* mResources; 
                                       
                                       
                                       
  
  nsXBLResource* mResourceList; 
  nsXBLResource* mLastResource;

  bool mLoadingResources;
  
  
  bool mInLoadResourcesFunc;
  int16_t mPendingSheets; 

  
  nsCOMArray<nsIContent> mBoundElements;
};

#endif
