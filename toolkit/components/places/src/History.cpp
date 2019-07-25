






































#include "History.h"
#include "nsNavHistory.h"

#include "mozilla/storage.h"
#include "mozilla/dom/Link.h"
#include "nsDocShellCID.h"
#include "nsIEventStateManager.h"

using namespace mozilla::dom;

namespace mozilla {
namespace places {




#define URI_VISITED "visited"
#define URI_NOT_VISITED "not visited"
#define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"




namespace {

class VisitedQuery : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS

  static nsresult Start(nsIURI* aURI)
  {
    NS_ASSERTION(aURI, "Don't pass a null URI!");

    nsNavHistory* navHist = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(navHist, NS_ERROR_FAILURE);
    mozIStorageStatement* stmt = navHist->DBGetIsVisited();
    NS_ENSURE_STATE(stmt);

    
    mozStorageStatementScoper scoper(stmt);
    nsCString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindUTF8StringParameter(0, spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI);
    NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<mozIStoragePendingStatement> handle;
    return stmt->ExecuteAsync(callback, getter_AddRefs(handle));
  }

  NS_IMETHOD HandleResult(mozIStorageResultSet* aResults)
  {
    
    
    mIsVisited = true;
    return NS_OK;
  }

  NS_IMETHOD HandleError(mozIStorageError* aError)
  {
    
    
    return NS_OK;
  }

  NS_IMETHOD HandleCompletion(PRUint16 aReason)
  {
    if (mIsVisited) {
      History::GetService()->NotifyVisited(mURI);
    }

    
    
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (observerService) {
      nsAutoString status;
      if (mIsVisited) {
        status.AssignLiteral(URI_VISITED);
      }
      else {
        status.AssignLiteral(URI_NOT_VISITED);
      }
      (void)observerService->NotifyObservers(mURI,
                                             URI_VISITED_RESOLUTION_TOPIC,
                                             status.get());
    }

    return NS_OK;
  }
private:
  VisitedQuery(nsIURI* aURI)
  : mURI(aURI)
  , mIsVisited(false)
  {
  }

  nsCOMPtr<nsIURI> mURI;
  bool mIsVisited;
};
NS_IMPL_ISUPPORTS1(
  VisitedQuery,
  mozIStorageStatementCallback
)

} 




History* History::gService = NULL;

History::History()
{
  NS_ASSERTION(!gService, "Ruh-roh!  This service has already been created!");
  gService = this;
}

History::~History()
{
  gService = NULL;
#ifdef DEBUG
  if (mObservers.IsInitialized()) {
    NS_ASSERTION(mObservers.Count() == 0,
                 "Not all Links were removed before we disappear!");
  }
#endif
}

void
History::NotifyVisited(nsIURI* aURI)
{
  NS_ASSERTION(aURI, "Ruh-roh!  A NULL URI was passed to us!");

  
  
  if (!mObservers.IsInitialized()) {
    return;
  }

  
  
  KeyClass* key = mObservers.GetEntry(aURI);
  if (!key) {
    return;
  }

  
  const ObserverArray& observers = key->array;
  ObserverArray::index_type len = observers.Length();
  for (ObserverArray::index_type i = 0; i < len; i++) {
    Link* link = observers[i];
    link->SetLinkState(eLinkState_Visited);
    NS_ASSERTION(len == observers.Length(),
                 "Calling SetLinkState added or removed an observer!");
  }

  
  mObservers.RemoveEntry(aURI);
}


History*
History::GetService()
{
  if (gService) {
    return gService;
  }

  nsCOMPtr<IHistory> service(do_GetService(NS_IHISTORY_CONTRACTID));
  NS_ABORT_IF_FALSE(service, "Cannot obtain IHistory service!");
  NS_ASSERTION(gService, "Our constructor was not run?!");

  return gService;
}


History*
History::GetSingleton()
{
  if (!gService) {
    gService = new History();
    NS_ENSURE_TRUE(gService, nsnull);
  }

  NS_ADDREF(gService);
  return gService;
}




NS_IMETHODIMP
History::RegisterVisitedCallback(nsIURI* aURI,
                                 Link* aLink)
{
  NS_ASSERTION(aURI, "Must pass a non-null URI!");
  NS_ASSERTION(aLink, "Must pass a non-null Link object!");

  
  if (!mObservers.IsInitialized()) {
    NS_ENSURE_TRUE(mObservers.Init(), NS_ERROR_OUT_OF_MEMORY);
  }

  
#ifdef DEBUG
  bool keyAlreadyExists = !!mObservers.GetEntry(aURI);
#endif
  KeyClass* key = mObservers.PutEntry(aURI);
  NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);
  ObserverArray& observers = key->array;

  if (observers.IsEmpty()) {
    NS_ASSERTION(!keyAlreadyExists,
                 "An empty key was kept around in our hashtable!");

    
    
    
    nsresult rv = VisitedQuery::Start(aURI);
    if (NS_FAILED(rv)) {
      
      mObservers.RemoveEntry(aURI);
      return rv;
    }
  }

  
  
  NS_ASSERTION(!observers.Contains(aLink),
               "Already tracking this Link object!");

  
  if (!observers.AppendElement(aLink)) {
    
    (void)UnregisterVisitedCallback(aURI, aLink);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
History::UnregisterVisitedCallback(nsIURI* aURI,
                                   Link* aLink)
{
  NS_ASSERTION(aURI, "Must pass a non-null URI!");
  NS_ASSERTION(aLink, "Must pass a non-null Link object!");

  
  KeyClass* key = mObservers.GetEntry(aURI);
  if (!key) {
    NS_ERROR("Trying to unregister for a URI that wasn't registered!");
    return NS_ERROR_UNEXPECTED;
  }
  ObserverArray& observers = key->array;
  if (!observers.RemoveElement(aLink)) {
    NS_ERROR("Trying to unregister a node that wasn't registered!");
    return NS_ERROR_UNEXPECTED;
  }

  
  if (observers.IsEmpty()) {
    mObservers.RemoveEntry(aURI);
  }

  return NS_OK;
}




NS_IMPL_ISUPPORTS1(
  History,
  IHistory
)

} 
} 
