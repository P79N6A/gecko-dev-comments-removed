





































class nsIRDFService;
class nsIRDFResource;
class nsICSSLoader;
class nsISimpleEnumerator;
class nsSupportsHashtable;
class nsIRDFContainer;
class nsIDOMWindowInternal;
class nsIDocument;

#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"

class nsChromeUIDataSource : public nsIRDFDataSource, public nsIRDFObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsChromeUIDataSource,
                                           nsIRDFDataSource)

  
  NS_DECL_NSIRDFDATASOURCE

  
  NS_DECL_NSIRDFOBSERVER

  
  nsChromeUIDataSource(nsIRDFDataSource* aComposite);
  virtual ~nsChromeUIDataSource();

protected:
  nsCOMPtr<nsIRDFDataSource>  mComposite;
  nsCOMArray<nsIRDFObserver>  mObservers;
  nsIRDFService* mRDFService;
};

nsresult NS_NewChromeUIDataSource(nsIRDFDataSource* aComposite, nsIRDFDataSource** aResult);
