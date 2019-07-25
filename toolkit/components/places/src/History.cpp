






































#ifdef MOZ_IPC
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"
#endif

#ifdef MOZ_IPC
#include "mozilla/dom/ContentChild.h"
#include "nsXULAppAPI.h"
#endif

#include "History.h"
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "Helpers.h"

#include "mozilla/storage.h"
#include "mozilla/dom/Link.h"
#include "nsDocShellCID.h"
#include "nsIEventStateManager.h"
#include "mozilla/Services.h"

using namespace mozilla::dom;

namespace mozilla {
namespace places {




#define URI_VISITED "visited"
#define URI_NOT_VISITED "not visited"
#define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"

#define URI_VISIT_SAVED "uri-visit-saved"




class Step : public AsyncStatementCallback
{
public:
  





  NS_IMETHOD ExecuteAsync(mozIStorageStatement* aStmt);

  








  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet);

  






  NS_IMETHOD HandleResult(mozIStorageResultSet* aResultSet);

  






  NS_IMETHOD HandleCompletion(PRUint16 aReason);

private:
  
  nsCOMPtr<mozIStorageResultSet> mResultSet;
};

NS_IMETHODIMP
Step::ExecuteAsync(mozIStorageStatement* aStmt)
{
  nsCOMPtr<mozIStoragePendingStatement> handle;
  nsresult rv = aStmt->ExecuteAsync(this, getter_AddRefs(handle));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
Step::Callback(mozIStorageResultSet* aResultSet)
{
  return NS_OK;
}

NS_IMETHODIMP
Step::HandleResult(mozIStorageResultSet* aResultSet)
{
  mResultSet = aResultSet;
  return NS_OK;
}

NS_IMETHODIMP
Step::HandleCompletion(PRUint16 aReason)
{
  if (aReason == mozIStorageStatementCallback::REASON_FINISHED) {
    nsCOMPtr<mozIStorageResultSet> resultSet = mResultSet;
    mResultSet = NULL;
    Callback(resultSet);
  }
  return NS_OK;
}




namespace {

class VisitedQuery : public AsyncStatementCallback
{
public:
  static nsresult Start(nsIURI* aURI)
  {
    NS_PRECONDITION(aURI, "Null URI");

#ifdef MOZ_IPC
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendStartVisitedQuery(aURI);
    return NS_OK;
  }
#endif

    mozIStorageAsyncStatement* stmt =
      History::GetService()->GetIsVisitedStatement();
    NS_ENSURE_STATE(stmt);

    
    nsresult rv = URIBinder::Bind(stmt, 0, aURI);
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
    if (aReason != mozIStorageStatementCallback::REASON_FINISHED) {
      return NS_OK;
    }

    if (mIsVisited) {
      History::GetService()->NotifyVisited(mURI);
    }

    
    
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
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








class FailSafeFinishTask
{
public:
  FailSafeFinishTask()
  : mAppended(false)
  {
  }

  ~FailSafeFinishTask()
  {
    if (mAppended) {
      History::GetService()->CurrentTaskFinished();
    }
  }

  



  void AppendTask(Step* step)
  {
    History::GetService()->AppendTask(step);
    mAppended = true;
  }

private:
  bool mAppended;
};




struct VisitURIData : public FailSafeFinishTask
{
  PRInt64 placeId;
  PRInt32 hidden;
  PRInt32 typed;
  nsCOMPtr<nsIURI> uri;

  
  nsCString lastSpec;
  PRInt64 lastVisitId;
  PRInt32 transitionType;
  PRInt64 sessionId;
  PRTime dateTime;
};




class UpdateFrecencyAndNotifyStep : public Step
{
public:
  UpdateFrecencyAndNotifyStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    
    NS_ENSURE_STATE(aResultSet);

    nsCOMPtr<mozIStorageRow> row;
    nsresult rv = aResultSet->GetNextRow(getter_AddRefs(row));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 visitId;
    rv = row->GetInt64(0, &visitId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    
    
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_WARN_IF_FALSE(history, "Could not get history service");
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_WARN_IF_FALSE(bookmarks, "Could not get bookmarks service");
    if (history && bookmarks) {
      
      nsresult rv = history->UpdateFrecency(mData->placeId);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not update frecency");

      
      
      if (!mData->hidden &&
          mData->transitionType != nsINavHistoryService::TRANSITION_EMBED &&
          mData->transitionType != nsINavHistoryService::TRANSITION_FRAMED_LINK) {
        history->NotifyOnVisit(mData->uri, visitId, mData->dateTime,
                               mData->sessionId, mData->lastVisitId,
                               mData->transitionType);
      }
    }

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService) {
      nsresult rv = obsService->NotifyObservers(mData->uri, URI_VISIT_SAVED, nsnull);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify observers");
    }

    History::GetService()->NotifyVisited(mData->uri);

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};




class GetVisitIDStep : public Step
{
public:
  GetVisitIDStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_RECENT_VISIT_OF_URL);
    NS_ENSURE_STATE(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new UpdateFrecencyAndNotifyStep(mData);
    NS_ENSURE_STATE(step);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};




class AddVisitStep : public Step
{
public:
  AddVisitStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsresult rv;

    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

    
    

    if (aResultSet) {
      
      nsCOMPtr<mozIStorageRow> row;
      rv = aResultSet->GetNextRow(getter_AddRefs(row));
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt64 possibleSessionId;
      PRTime lastVisitOfSession;

      rv = row->GetInt64(0, &mData->lastVisitId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = row->GetInt64(1, &possibleSessionId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = row->GetInt64(2, &lastVisitOfSession);
      NS_ENSURE_SUCCESS(rv, rv);

      if (mData->dateTime - lastVisitOfSession <= RECENT_EVENT_THRESHOLD) {
        mData->sessionId = possibleSessionId;
      }
      else {
        
        mData->sessionId = history->GetNewSessionID();
        mData->lastVisitId = 0;
      }
    }
    else {
      
      mData->sessionId = history->GetNewSessionID();
      mData->lastVisitId = 0;
    }

    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_INSERT_VISIT);
    NS_ENSURE_STATE(stmt);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("from_visit"),
                               mData->lastVisitId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                               mData->placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"),
                               mData->dateTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("visit_type"),
                               mData->transitionType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("session"),
                               mData->sessionId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new GetVisitIDStep(mData);
    NS_ENSURE_STATE(step);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};





class CheckLastVisitStep : public Step
{
public:
  CheckLastVisitStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsresult rv;

    if (aResultSet) {
      
      nsCOMPtr<mozIStorageRow> row;
      rv = aResultSet->GetNextRow(getter_AddRefs(row));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = row->GetInt64(0, &mData->placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!mData->lastSpec.IsEmpty()) {
      
      
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      nsCOMPtr<mozIStorageStatement> stmt =
        history->GetStatementById(DB_RECENT_VISIT_OF_URL);
      NS_ENSURE_STATE(stmt);

      rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->lastSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<Step> step = new AddVisitStep(mData);
      NS_ENSURE_STATE(step);
      rv = step->ExecuteAsync(stmt);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      
      nsCOMPtr<Step> step = new AddVisitStep(mData);
      NS_ENSURE_STATE(step);
      rv = step->Callback(NULL);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};





class FindNewIdStep : public Step
{
public:
  FindNewIdStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_GET_PAGE_VISIT_STATS);
    NS_ENSURE_STATE(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new CheckLastVisitStep(mData);
    NS_ENSURE_STATE(step);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};





class CheckExistingStep : public Step
{
public:
  CheckExistingStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsresult rv;
    nsCOMPtr<mozIStorageStatement> stmt;

    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

    if (aResultSet) {
      nsCOMPtr<mozIStorageRow> row;
      rv = aResultSet->GetNextRow(getter_AddRefs(row));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = row->GetInt64(0, &mData->placeId);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!mData->typed) {
        
        
        rv = row->GetInt32(2, &mData->typed);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      if (mData->hidden) {
        
        
        
        rv = row->GetInt32(3, &mData->hidden);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      stmt = history->GetStatementById(DB_UPDATE_PAGE_VISIT_STATS);
      NS_ENSURE_STATE(stmt);

      rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), mData->typed);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), mData->hidden);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mData->placeId);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<Step> step = new CheckLastVisitStep(mData);
      NS_ENSURE_STATE(step);
      rv = step->ExecuteAsync(stmt);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      stmt = history->GetStatementById(DB_ADD_NEW_PAGE);
      NS_ENSURE_STATE(stmt);

      nsAutoString revHost;
      rv = GetReversedHostname(mData->uri, revHost);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("rev_host"), revHost);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), mData->typed);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), mData->hidden);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("frecency"), -1);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<Step> step = new FindNewIdStep(mData);
      NS_ENSURE_STATE(step);
      rv = step->ExecuteAsync(stmt);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};




class StartVisitURIStep : public Step
{
public:
  StartVisitURIStep(nsAutoPtr<VisitURIData> aData)
  : mData(aData)
  {
    mData->AppendTask(this);
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

    
    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_GET_PAGE_VISIT_STATS);
    NS_ENSURE_STATE(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new CheckExistingStep(mData);
    NS_ENSURE_STATE(step);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<VisitURIData> mData;
};




struct SetTitleData : public FailSafeFinishTask
{
  nsCOMPtr<nsIURI> uri;
  nsString title;
};




class TitleNotifyStep: public Step
{
public:
  TitleNotifyStep(nsAutoPtr<SetTitleData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    history->NotifyTitleChange(mData->uri, mData->title);

    return NS_OK;
  }

protected:
  nsAutoPtr<SetTitleData> mData;
};




class SetTitleStep : public Step
{
public:
  SetTitleStep(nsAutoPtr<SetTitleData> aData)
  : mData(aData)
  {
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    if (!aResultSet) {
      
      return NS_OK;
    }

    nsCOMPtr<mozIStorageRow> row;
    nsresult rv = aResultSet->GetNextRow(getter_AddRefs(row));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString title;
    rv = row->GetString(2, title);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    if (mData->title.Equals(title) || (mData->title.IsVoid() && title.IsVoid()))
      return NS_OK;

    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_SET_PLACE_TITLE);
    NS_ENSURE_STATE(stmt);

    if (mData->title.IsVoid()) {
      rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_title"));
    }
    else {
      rv = stmt->BindStringByName(
        NS_LITERAL_CSTRING("page_title"),
        StringHead(mData->title, TITLE_LENGTH_MAX)
      );
    }
    NS_ENSURE_SUCCESS(rv, rv);

    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new TitleNotifyStep(mData);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<SetTitleData> mData;
};




class StartSetURITitleStep : public Step
{
public:
  StartSetURITitleStep(nsAutoPtr<SetTitleData> aData)
  : mData(aData)
  {
    mData->AppendTask(this);
  }

  NS_IMETHOD Callback(mozIStorageResultSet* aResultSet)
  {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

    
    nsCOMPtr<mozIStorageStatement> stmt =
      history->GetStatementById(DB_GET_URL_PAGE_INFO);
    NS_ENSURE_STATE(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData->uri);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<Step> step = new SetTitleStep(mData);
    rv = step->ExecuteAsync(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

protected:
  nsAutoPtr<SetTitleData> mData;
};

} 




History* History::gService = NULL;

History::History()
: mShuttingDown(false)
{
  NS_ASSERTION(!gService, "Ruh-roh!  This service has already been created!");
  gService = this;

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_WARN_IF_FALSE(os, "Observer service was not found!");
  if (os) {
    (void)os->AddObserver(this, TOPIC_PLACES_SHUTDOWN, PR_FALSE);
  }
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

  
  
  Shutdown();
}

void
History::AppendTask(Step* aTask)
{
  NS_PRECONDITION(aTask, "Got NULL task.");

  if (mShuttingDown) {
    return;
  }

  NS_ADDREF(aTask);
  mPendingVisits.Push(aTask);

  if (mPendingVisits.GetSize() == 1) {
    
    StartNextTask();
  }
}

void
History::CurrentTaskFinished()
{
  if (mShuttingDown) {
    return;
  }

  NS_ASSERTION(mPendingVisits.PeekFront(), "Tried to finish task not on the queue");

  nsCOMPtr<Step> deadTaskWalking =
    dont_AddRef(static_cast<Step*>(mPendingVisits.PopFront()));
  StartNextTask();
}

void
History::NotifyVisited(nsIURI* aURI)
{
  NS_ASSERTION(aURI, "Ruh-roh!  A NULL URI was passed to us!");

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mozilla::dom::ContentParent* cpp = 
      mozilla::dom::ContentParent::GetSingleton(PR_FALSE);
    if (cpp)
      (void)cpp->SendNotifyVisited(aURI);
  }
#endif

  
  
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

mozIStorageAsyncStatement*
History::GetIsVisitedStatement()
{
  if (mIsVisitedStatement) {
    return mIsVisitedStatement;
  }

  
  if (!mReadOnlyDBConn) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, nsnull);

    nsCOMPtr<mozIStorageConnection> dbConn;
    (void)history->GetDBConnection(getter_AddRefs(dbConn));
    NS_ENSURE_TRUE(dbConn, nsnull);

    (void)dbConn->Clone(PR_TRUE, getter_AddRefs(mReadOnlyDBConn));
    NS_ENSURE_TRUE(mReadOnlyDBConn, nsnull);
  }

  
  nsresult rv = mReadOnlyDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT h.id "
    "FROM moz_places h "
    "WHERE url = ?1 "
      "AND EXISTS(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
  ),  getter_AddRefs(mIsVisitedStatement));
  NS_ENSURE_SUCCESS(rv, nsnull);
  return mIsVisitedStatement;
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

void
History::StartNextTask()
{
  if (mShuttingDown) {
    return;
  }

  nsCOMPtr<Step> nextTask =
    static_cast<Step*>(mPendingVisits.PeekFront());
  if (!nextTask) {
    
    return;
  }
  nsresult rv = nextTask->Callback(NULL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Beginning a task failed.");
}

void
History::Shutdown()
{
  mShuttingDown = true;

  while (mPendingVisits.PeekFront()) {
    nsCOMPtr<Step> deadTaskWalking =
      dont_AddRef(static_cast<Step*>(mPendingVisits.PopFront()));
  }

  
  if (mReadOnlyDBConn) {
    if (mIsVisitedStatement) {
      (void)mIsVisitedStatement->Finalize();
    }
    (void)mReadOnlyDBConn->AsyncClose(nsnull);
  }
}




NS_IMETHODIMP
History::VisitURI(nsIURI* aURI,
                  nsIURI* aLastVisitedURI,
                  PRUint32 aFlags)
{
  NS_PRECONDITION(aURI, "URI should not be NULL.");
  if (mShuttingDown) {
    return NS_OK;
  }

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendVisitURI(aURI, aLastVisitedURI, aFlags);
    return NS_OK;
  } 
#endif 

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  PRBool canAdd;
  nsresult rv = history->CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  
  nsAutoPtr<VisitURIData> data(new VisitURIData());
  NS_ENSURE_STATE(data);

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aLastVisitedURI) {
    rv = aLastVisitedURI->GetSpec(data->lastSpec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (spec.Equals(data->lastSpec)) {
    
    return NS_OK;
  }

  
  
  PRUint32 recentFlags = history->GetRecentFlags(aURI);
  bool redirected = false;
  if (aFlags & IHistory::REDIRECT_TEMPORARY) {
    data->transitionType = nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY;
    redirected = true;
  }
  else if (aFlags & IHistory::REDIRECT_PERMANENT) {
    data->transitionType = nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT;
    redirected = true;
  }
  else if (recentFlags & nsNavHistory::RECENT_TYPED) {
    data->transitionType = nsINavHistoryService::TRANSITION_TYPED;
  }
  else if (recentFlags & nsNavHistory::RECENT_BOOKMARKED) {
    data->transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;
  }
  else if (aFlags & IHistory::TOP_LEVEL) {
    
    data->transitionType = nsINavHistoryService::TRANSITION_LINK;
  }
  else if (recentFlags & nsNavHistory::RECENT_ACTIVATED) {
    
    data->transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;
  }
  else {
    
    data->transitionType = nsINavHistoryService::TRANSITION_EMBED;
  }

  data->typed = (data->transitionType == nsINavHistoryService::TRANSITION_TYPED) ? 1 : 0;
  data->hidden = 
    (data->transitionType == nsINavHistoryService::TRANSITION_FRAMED_LINK ||
     data->transitionType == nsINavHistoryService::TRANSITION_EMBED ||
     redirected) ? 1 : 0;
  data->dateTime = PR_Now();
  data->uri = aURI;

  nsCOMPtr<Step> task(new StartVisitURIStep(data));

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
History::RegisterVisitedCallback(nsIURI* aURI,
                                 Link* aLink)
{
  NS_ASSERTION(aURI, "Must pass a non-null URI!");
#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_PRECONDITION(aLink, "Must pass a non-null Link!");
  }
#else
  NS_PRECONDITION(aLink, "Must pass a non-null Link!");
#endif

  
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

    
    
    
    
    if (NS_FAILED(rv) || !aLink) {
      
      mObservers.RemoveEntry(aURI);
      return rv;
    }
  }
#ifdef MOZ_IPC
  
  
  
  else if (!aLink) {
    NS_ASSERTION(XRE_GetProcessType() == GeckoProcessType_Default,
                 "We should only ever get a null Link in the default process!");
    return NS_OK;
  }
#endif

  
  
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

NS_IMETHODIMP
History::SetURITitle(nsIURI* aURI, const nsAString& aTitle)
{
  NS_PRECONDITION(aURI, "Must pass a non-null URI!");
  if (mShuttingDown) {
    return NS_OK;
  }

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendSetURITitle(aURI, nsDependentString(aTitle));
    return NS_OK;
  } 
#endif 

  nsNavHistory* history = nsNavHistory::GetHistoryService();

  
  
  
  
  
  
  
  
  
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRBool canAdd;
  nsresult rv = history->CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  nsAutoPtr<SetTitleData> data(new SetTitleData());
  NS_ENSURE_STATE(data);

  data->uri = aURI;

  if (aTitle.IsEmpty()) {
    data->title.SetIsVoid(PR_TRUE);
  }
  else {
    data->title.Assign(aTitle);
  }

  nsCOMPtr<Step> task(new StartSetURITitleStep(data));

  return NS_OK;
}




NS_IMETHODIMP
History::Observe(nsISupports* aSubject, const char* aTopic,
                 const PRUnichar* aData)
{
  if (strcmp(aTopic, TOPIC_PLACES_SHUTDOWN) == 0) {
    Shutdown();

    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      (void)os->RemoveObserver(this, TOPIC_PLACES_SHUTDOWN);
    }
  }

  return NS_OK;
}




NS_IMPL_ISUPPORTS2(
  History
, IHistory
, nsIObserver
)

} 
} 
