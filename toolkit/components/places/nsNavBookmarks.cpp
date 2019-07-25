








































#include "nsAppDirectoryServiceDefs.h"
#include "nsNavBookmarks.h"
#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsIDynamicContainer.h"
#include "nsUnicharUtils.h"
#include "nsFaviconService.h"
#include "nsAnnotationService.h"
#include "nsPrintfCString.h"
#include "nsIUUIDGenerator.h"
#include "prprf.h"
#include "nsILivemarkService.h"
#include "nsPlacesTriggers.h"
#include "nsPlacesTables.h"
#include "nsPlacesIndexes.h"
#include "nsPlacesMacros.h"
#include "Helpers.h"

#include "mozilla/FunctionTimer.h"

#define BOOKMARKS_TO_KEYWORDS_INITIAL_CACHE_SIZE 64

const PRInt32 nsNavBookmarks::kFindBookmarksIndex_ID = 0;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Type = 1;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_PlaceID = 2;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Parent = 3;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Position = 4;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Title = 5;


const PRInt32 nsNavBookmarks::kGetChildrenIndex_Position = 14;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_Type = 15;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_PlaceID = 16;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_ServiceContractId = 17;

const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_ID = 0;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_URI = 1;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_Title = 2;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_Position = 3;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_PlaceID = 4;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_Parent = 5;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_Type = 6;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_ServiceContractId = 7;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_DateAdded = 8;
const PRInt32 nsNavBookmarks::kGetItemPropertiesIndex_LastModified = 9;

using namespace mozilla::places;

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsNavBookmarks, gBookmarksService)

#define BOOKMARKS_ANNO_PREFIX "bookmarks/"
#define BOOKMARKS_TOOLBAR_FOLDER_ANNO NS_LITERAL_CSTRING(BOOKMARKS_ANNO_PREFIX "toolbarFolder")
#define GUID_ANNO NS_LITERAL_CSTRING("placesInternal/GUID")
#define READ_ONLY_ANNO NS_LITERAL_CSTRING("placesInternal/READ_ONLY")


namespace {

struct keywordSearchData
{
  PRInt64 itemId;
  nsString keyword;
};

PLDHashOperator
SearchBookmarkForKeyword(nsTrimInt64HashKey::KeyType aKey,
                         const nsString aValue,
                         void* aUserArg)
{
  keywordSearchData* data = reinterpret_cast<keywordSearchData*>(aUserArg);
  if (data->keyword.Equals(aValue)) {
    data->itemId = aKey;
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

template<typename Method, typename DataType>
class AsyncGetBookmarksForURI : public AsyncStatementCallback
{
public:
  AsyncGetBookmarksForURI(nsNavBookmarks* aBookmarksSvc,
                          Method aCallback,
                          const DataType& aData)
  : mBookmarksSvc(aBookmarksSvc)
  , mCallback(aCallback)
  , mData(aData)
  {
  }

  void Init()
  {
    nsCOMPtr<mozIStorageStatement> stmt =
      mBookmarksSvc->GetStatementById(DB_GET_BOOKMARKS_FOR_URI);
    if (stmt) {
      (void)URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mData.uri);
      nsCOMPtr<mozIStoragePendingStatement> pendingStmt;
      (void)stmt->ExecuteAsync(this, getter_AddRefs(pendingStmt));
    }
  }

  NS_IMETHOD HandleResult(mozIStorageResultSet* aResultSet)
  {
    nsCOMPtr<mozIStorageRow> row;
    while (NS_SUCCEEDED(aResultSet->GetNextRow(getter_AddRefs(row))) && row) {
      nsresult rv = row->GetInt64(0, &mData.itemId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mCallback) {
        ((*mBookmarksSvc).*mCallback)(mData);
      }
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsNavBookmarks> mBookmarksSvc;
  Method mCallback;
  DataType mData;
};

} 


nsNavBookmarks::nsNavBookmarks() : mItemCount(0)
                                 , mRoot(0)
                                 , mMenuRoot(0)
                                 , mTagsRoot(0)
                                 , mUnfiledRoot(0)
                                 , mToolbarRoot(0)
                                 , mCanNotify(false)
                                 , mCacheObservers("bookmark-observers")
                                 , mShuttingDown(false)
                                 , mBatching(false)
{
  NS_ASSERTION(!gBookmarksService,
               "Attempting to create two instances of the service!");
  gBookmarksService = this;
}


nsNavBookmarks::~nsNavBookmarks()
{
  NS_ASSERTION(gBookmarksService == this,
               "Deleting a non-singleton instance of the service");
  if (gBookmarksService == this)
    gBookmarksService = nsnull;
}


NS_IMPL_ISUPPORTS3(nsNavBookmarks,
                   nsINavBookmarksService,
                   nsINavHistoryObserver,
                   nsIAnnotationObserver)


nsresult
nsNavBookmarks::Init()
{
  NS_TIME_FUNCTION;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  mDBConn = history->GetStorageConnection();
  NS_ENSURE_STATE(mDBConn);

  
  nsresult rv = mDBConn->Clone(PR_TRUE, getter_AddRefs(mDBReadOnlyConn));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint16 dbStatus;
  rv = history->GetDatabaseStatus(&dbStatus);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = InitRoots(dbStatus != nsINavHistoryService::DATABASE_STATUS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  mCanNotify = true;

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  annosvc->AddObserver(this);

  
  
  
  history->AddObserver(this, PR_FALSE);

  

  return NS_OK;
}







nsresult 
nsNavBookmarks::InitTables(mozIStorageConnection* aDBConn)
{
  PRBool exists;
  nsresult rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_bookmarks"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_BOOKMARKS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACETYPE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PARENTPOSITION);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_bookmarks_roots"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_BOOKMARKS_ROOTS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_keywords"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_KEYWORDS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_KEYWORD_VALIDITY_TRIGGER);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


mozIStorageStatement*
nsNavBookmarks::GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt)
{
  if (mShuttingDown)
    return nsnull;

  
  
  
  RETURN_IF_STMT(mDBFindURIBookmarks, NS_LITERAL_CSTRING(
    "SELECT b.id "
    "FROM moz_bookmarks b "
    "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
    "ORDER BY b.lastModified DESC, b.id DESC "));

  
  
  
  
  
  RETURN_IF_STMT(mDBGetChildren, NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, IFNULL(b.title, h.title), h.rev_host, h.visit_count, "
           "h.last_visit_date, f.url, null, b.id, b.dateAdded, b.lastModified, "
           "b.parent, null, h.frecency, b.position, b.type, b.fk, b.folder_type "
    "FROM moz_bookmarks b "
    "LEFT JOIN moz_places h ON b.fk = h.id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE b.parent = :parent "
    "ORDER BY b.position ASC"));

  
  RETURN_IF_STMT(mDBFolderCount, NS_LITERAL_CSTRING(
    "SELECT COUNT(*), "
    "(SELECT id FROM moz_bookmarks WHERE id = :parent) "
    "FROM moz_bookmarks WHERE parent = :parent"));

  RETURN_IF_STMT(mDBGetChildAt, NS_LITERAL_CSTRING(
    "SELECT id, fk, type FROM moz_bookmarks "
    "WHERE parent = :parent AND position = :item_index"));

  
  
  RETURN_IF_STMT(mDBGetItemProperties, NS_LITERAL_CSTRING(
    "SELECT b.id, h.url, b.title, b.position, b.fk, b.parent, b.type, "
           "b.folder_type, b.dateAdded, b.lastModified "
    "FROM moz_bookmarks b "
    "LEFT JOIN moz_places h ON h.id = b.fk "
    "WHERE b.id = :item_id"));

  RETURN_IF_STMT(mDBGetItemIdForGUID, NS_LITERAL_CSTRING(
    "SELECT item_id FROM moz_items_annos "
    "WHERE content = :guid "
    "LIMIT 1"));

  RETURN_IF_STMT(mDBInsertBookmark, NS_LITERAL_CSTRING(
    "INSERT INTO moz_bookmarks "
      "(id, fk, type, parent, position, title, folder_type, "
       "dateAdded, lastModified, guid) "
    "VALUES (:item_id, :page_id, :item_type, :parent, :item_index, "
            ":item_title, :folder_type, :date_added, :last_modified, "
            "GENERATE_GUID())"));

  
  
  RETURN_IF_STMT(mDBIsBookmarkedInDatabase, NS_LITERAL_CSTRING(
    "SELECT 1 FROM moz_bookmarks WHERE fk = :page_id"));

  RETURN_IF_STMT(mDBIsURIBookmarkedInDatabase, NS_LITERAL_CSTRING(
    "SELECT 1 FROM moz_bookmarks b "
    "JOIN moz_places h ON b.fk = h.id "
    "WHERE h.url = :page_url"));

  
  RETURN_IF_STMT(mDBIsRealBookmark, NS_LITERAL_CSTRING(
    "SELECT id "
    "FROM moz_bookmarks "
    "WHERE fk = :page_id "
      "AND type = :item_type "
      "AND parent NOT IN ("
        "SELECT a.item_id "
        "FROM moz_items_annos a "
        "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
        "WHERE n.name = :anno_name"
      ") "));

  RETURN_IF_STMT(mDBGetLastBookmarkID, NS_LITERAL_CSTRING(
    "SELECT id "
    "FROM moz_bookmarks "
    "ORDER BY ROWID DESC "
    "LIMIT 1"));

  
  
  
  RETURN_IF_STMT(mDBSetItemDateAdded, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET dateAdded = :date, lastModified = :date "
    "WHERE id = :item_id"));

  RETURN_IF_STMT(mDBSetItemLastModified, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET lastModified = :date WHERE id = :item_id"));

  RETURN_IF_STMT(mDBSetItemIndex, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET position = :item_index WHERE id = :item_id"));

  RETURN_IF_STMT(mDBGetKeywordForURI, NS_LITERAL_CSTRING(
    "SELECT k.keyword "
    "FROM moz_places h "
    "JOIN moz_bookmarks b ON b.fk = h.id "
    "JOIN moz_keywords k ON k.id = b.keyword_id "
    "WHERE h.url = :page_url "));

  RETURN_IF_STMT(mDBAdjustPosition, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET position = position + :delta "
    "WHERE parent = :parent "
      "AND position BETWEEN :from_index AND :to_index"));

  RETURN_IF_STMT(mDBRemoveItem, NS_LITERAL_CSTRING(
    "DELETE FROM moz_bookmarks WHERE id = :item_id"));

  RETURN_IF_STMT(mDBGetLastChildId, NS_LITERAL_CSTRING(
    "SELECT id FROM moz_bookmarks WHERE parent = :parent "
    "ORDER BY position DESC LIMIT 1"));

  RETURN_IF_STMT(mDBMoveItem, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET parent = :parent, position = :item_index "
    "WHERE id = :item_id "));

  RETURN_IF_STMT(mDBSetItemTitle, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET title = :item_title, lastModified = :date "
    "WHERE id = :item_id "));

  RETURN_IF_STMT(mDBChangeBookmarkURI, NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET fk = :page_id, lastModified = :date "
    "WHERE id = :item_id "));

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#define COALESCE_PLACEID \
  "COALESCE(greatgrandparent.place_id, grandparent.place_id, parent.place_id) "

  nsCString redirectsFragment =
    nsPrintfCString(3, "%d,%d",
                    nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                    nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY);

  RETURN_IF_STMT(mDBFindRedirectedBookmark, NS_LITERAL_CSTRING(
    "SELECT "
      "(SELECT url FROM moz_places WHERE id = :page_id) "
    "FROM moz_bookmarks b "
    "WHERE b.fk = :page_id "
    "UNION ALL " 
    "SELECT "
      "(SELECT url FROM moz_places WHERE id = " COALESCE_PLACEID ") "
    "FROM moz_historyvisits self "
    "JOIN moz_bookmarks b ON b.fk = " COALESCE_PLACEID
    "LEFT JOIN moz_historyvisits parent ON parent.id = self.from_visit "
    "LEFT JOIN moz_historyvisits grandparent ON parent.from_visit = grandparent.id "
      "AND parent.visit_type IN (") + redirectsFragment + NS_LITERAL_CSTRING(") "
    "LEFT JOIN moz_historyvisits greatgrandparent ON grandparent.from_visit = greatgrandparent.id "
      "AND grandparent.visit_type IN (") + redirectsFragment + NS_LITERAL_CSTRING(") "
    "WHERE self.visit_type IN (") + redirectsFragment + NS_LITERAL_CSTRING(") "
      "AND self.place_id = :page_id "
    "LIMIT 1 " 
  ));
#undef COALESCE_PLACEID

  return nsnull;
}


nsresult
nsNavBookmarks::FinalizeStatements() {
  mShuttingDown = true;

  mozIStorageStatement* stmts[] = {
    mDBGetChildren,
    mDBFindURIBookmarks,
    mDBFolderCount,
    mDBGetChildAt,
    mDBGetItemProperties,
    mDBGetItemIdForGUID,
    mDBInsertBookmark,
    mDBIsBookmarkedInDatabase,
    mDBIsRealBookmark,
    mDBGetLastBookmarkID,
    mDBSetItemDateAdded,
    mDBSetItemLastModified,
    mDBSetItemIndex,
    mDBGetKeywordForURI,
    mDBAdjustPosition,
    mDBRemoveItem,
    mDBGetLastChildId,
    mDBMoveItem,
    mDBSetItemTitle,
    mDBChangeBookmarkURI,
    mDBIsURIBookmarkedInDatabase,
    mDBFindRedirectedBookmark,
  };

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(stmts); i++) {
    nsresult rv = nsNavHistory::FinalizeStatement(stmts[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  (void)mDBReadOnlyConn->AsyncClose(nsnull);

#ifdef DEBUG
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * "
    "FROM moz_bookmarks "
    "WHERE guid IS NULL "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool haveNullGuids;
  rv = stmt->ExecuteStep(&haveNullGuids);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(!haveNullGuids,
               "Someone added a bookmark without adding a GUID!");
#endif

  return NS_OK;
}


nsresult
nsNavBookmarks::InitRoots(bool aForceCreate)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBReadOnlyConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT root_name, folder_id FROM moz_bookmarks_roots"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    nsCAutoString rootName;
    rv = stmt->GetUTF8String(0, rootName);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 rootId;
    rv = stmt->GetInt64(1, &rootId);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ABORT_IF_FALSE(rootId != 0, "Root id is 0, that is an invalid value.");

    if (rootName.EqualsLiteral("places")) {
      mRoot = rootId;
    }
    else if (rootName.EqualsLiteral("menu")) {
      mMenuRoot = rootId;
    }
    else if (rootName.EqualsLiteral("toolbar")) {
      mToolbarRoot = rootId;
    }
    else if (rootName.EqualsLiteral("tags")) {
      mTagsRoot = rootId;
    }
    else if (rootName.EqualsLiteral("unfiled")) {
      mUnfiledRoot = rootId;
    }
  }

  if (aForceCreate) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    nsIStringBundle* bundle = history->GetBundle();
    NS_ENSURE_TRUE(bundle, NS_ERROR_OUT_OF_MEMORY);

    mozStorageTransaction transaction(mDBConn, PR_FALSE);

    rv = CreateRoot(NS_LITERAL_CSTRING("places"), &mRoot, 0,
                    nsnull, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CreateRoot(NS_LITERAL_CSTRING("menu"), &mMenuRoot, mRoot, bundle,
                    NS_LITERAL_STRING("BookmarksMenuFolderTitle").get());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CreateRoot(NS_LITERAL_CSTRING("toolbar"), &mToolbarRoot, mRoot, bundle,
                    NS_LITERAL_STRING("BookmarksToolbarFolderTitle").get());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CreateRoot(NS_LITERAL_CSTRING("tags"), &mTagsRoot, mRoot, bundle,
                    NS_LITERAL_STRING("TagsFolderTitle").get());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CreateRoot(NS_LITERAL_CSTRING("unfiled"), &mUnfiledRoot, mRoot, bundle,
                    NS_LITERAL_STRING("UnsortedBookmarksFolderTitle").get());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mBatching) {
      ForceWALCheckpoint(mDBConn);
    }
  }

  return NS_OK;
}


nsresult
nsNavBookmarks::CreateRoot(const nsCString& name,
                           PRInt64* _itemId,
                           PRInt64 aParentId,
                           nsIStringBundle* aBundle,
                           const PRUnichar* aTitleStringId)
{
  nsresult rv;

  if (*_itemId == 0) {
    
    rv = CreateFolder(aParentId, EmptyCString(), DEFAULT_INDEX, _itemId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_bookmarks_roots (root_name, folder_id) "
      "VALUES (:root_name, :item_id)"
    ), getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("root_name"), name);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), *_itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (aTitleStringId) {
    nsXPIDLString title;
    rv = aBundle->GetStringFromName(aTitleStringId, getter_Copies(title));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = SetItemTitle(*_itemId, NS_ConvertUTF16toUTF8(title));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


PRBool
nsNavBookmarks::IsRealBookmark(PRInt64 aPlaceId)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBIsRealBookmark);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlaceId);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Binding failed");
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_type"), TYPE_BOOKMARK);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Binding failed");
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  NS_LITERAL_CSTRING(LMANNO_FEEDURI));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Binding failed");

  
  
  PRBool isBookmark;
  rv = stmt->ExecuteStep(&isBookmark);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExecuteStep failed");
  if (NS_SUCCEEDED(rv))
    return isBookmark;

  return PR_FALSE;
}






nsresult
nsNavBookmarks::IsBookmarkedInDatabase(PRInt64 aPlaceId,
                                       PRBool* aIsBookmarked)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBIsBookmarkedInDatabase);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->ExecuteStep(aIsBookmarked);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


nsresult
nsNavBookmarks::AdjustIndices(PRInt64 aFolderId,
                              PRInt32 aStartIndex,
                              PRInt32 aEndIndex,
                              PRInt32 aDelta)
{
  NS_ASSERTION(aStartIndex >= 0 && aEndIndex <= PR_INT32_MAX &&
               aStartIndex <= aEndIndex, "Bad indices");

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBAdjustPosition);
  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("delta"), aDelta);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("from_index"), aStartIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("to_index"), aEndIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
 
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetPlacesRoot(PRInt64* aRoot)
{
  *aRoot = mRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarksMenuFolder(PRInt64* aRoot)
{
  *aRoot = mMenuRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetToolbarFolder(PRInt64* aFolderId)
{
  *aFolderId = mToolbarRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetTagsFolder(PRInt64* aRoot)
{
  *aRoot = mTagsRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetUnfiledBookmarksFolder(PRInt64* aRoot)
{
  *aRoot = mUnfiledRoot;
  return NS_OK;
}


nsresult
nsNavBookmarks::InsertBookmarkInDB(PRInt64 aItemId,
                                   PRInt64 aPlaceId,
                                   enum ItemType aItemType,
                                   PRInt64 aParentId,
                                   PRInt32 aIndex,
                                   const nsACString& aTitle,
                                   PRTime aDateAdded,
                                   PRTime aLastModified,
                                   const nsAString& aServiceContractId,
                                   PRInt64* _newItemId)
{
  NS_ASSERTION(_newItemId, "Null pointer passed to InsertBookmarkInDB!");

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBInsertBookmark);
  nsresult rv;
  if (aItemId && aItemId != -1)
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  else
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aPlaceId && aPlaceId != -1)
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlaceId);
  else
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_type"), aItemType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTitle.IsVoid())
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("item_title"));
  else
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("item_title"), aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aServiceContractId.IsEmpty()) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("folder_type"));
  }
  else {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("folder_type"),
                                aServiceContractId);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("date_added"), aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aLastModified) {
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("last_modified"),
                               aLastModified);
  }
  else {
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("last_modified"), aDateAdded);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aItemId || aItemId == -1) {
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(lastInsertIdStmt, mDBGetLastBookmarkID);
    PRBool hasResult;
    rv = lastInsertIdStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);
    rv = lastInsertIdStmt->GetInt64(0, _newItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    *_newItemId = aItemId;
  }

  
  
  
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aParentId, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::InsertBookmark(PRInt64 aFolder,
                               nsIURI* aURI,
                               PRInt32 aIndex,
                               const nsACString& aTitle,
                               PRInt64* aNewBookmarkId)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aNewBookmarkId);

  
  if (aIndex < nsINavBookmarksService::DEFAULT_INDEX)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 childID;
  nsresult rv = history->GetUrlIdFor(aURI, &childID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index;
  PRInt32 folderCount;
  rv = FolderCount(aFolder, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    index = folderCount;
  }
  else {
    index = aIndex;
    rv = AdjustIndices(aFolder, index, PR_INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = InsertBookmarkInDB(-1, childID, BOOKMARK, aFolder, index,
                          aTitle, PR_Now(), nsnull, EmptyString(),
                          aNewBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  

  nsCAutoString url;
  rv = aURI->GetSpec(url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = history->UpdateFrecency(childID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBatching) {
    ForceWALCheckpoint(mDBConn);
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewBookmarkId, aFolder, index, TYPE_BOOKMARK,
                               aURI));

  
  
  
  PRInt64 grandParentId;
  rv = GetFolderIdForItem(aFolder, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  if (grandParentId == mTagsRoot) {
    
    nsTArray<PRInt64> bookmarks;
    rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
        
        if (bookmarks[i] == *aNewBookmarkId) {
          continue;
        }

        NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                         nsINavBookmarkObserver,
                         OnItemChanged(bookmarks[i], NS_LITERAL_CSTRING("tags"),
                                       PR_FALSE, EmptyCString(), 0,
                                       TYPE_BOOKMARK));
      }
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RemoveItem(PRInt64 aItemId)
{
  NS_ENSURE_TRUE(aItemId != mRoot, NS_ERROR_INVALID_ARG);

  nsresult rv;
  PRInt32 childIndex;
  PRInt64 placeId, folderId;
  PRInt32 itemType;
  nsCAutoString buffer;
  nsCAutoString spec;

  { 
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getInfoStmt, mDBGetItemProperties);
    rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = getInfoStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasResult)
      return NS_ERROR_INVALID_ARG; 

    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Position, &childIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_PlaceID, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &folderId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (itemType == TYPE_BOOKMARK) {
      rv = getInfoStmt->GetUTF8String(kGetItemPropertiesIndex_URI, spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (itemType == TYPE_FOLDER) {
    rv = RemoveFolder(aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnBeforeItemRemoved(aItemId, itemType));

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  rv = annosvc->RemoveItemAnnotations(aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBRemoveItem);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (childIndex != -1) {
    rv = AdjustIndices(folderId, childIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           folderId, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBatching) {
    ForceWALCheckpoint(mDBConn);
  }

  if (itemType == TYPE_BOOKMARK) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->UpdateFrecency(placeId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdateKeywordsHashForRemovedBookmark(aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  bool isTagEntry = false;
  if (itemType == TYPE_BOOKMARK) {
    
    
    
    PRInt64 grandParentId;
    rv = GetFolderIdForItem(folderId, &grandParentId);
    NS_ENSURE_SUCCESS(rv, rv);
    isTagEntry = grandParentId == mTagsRoot;
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemRemoved(aItemId, folderId, childIndex, itemType));

  if (isTagEntry) {
    
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), spec);
    NS_ENSURE_SUCCESS(rv, rv);
    nsTArray<PRInt64> bookmarks;
    rv = GetBookmarkIdsForURITArray(uri, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavBookmarkObserver,
                       OnItemChanged(bookmarks[i],
                                     NS_LITERAL_CSTRING("tags"), PR_FALSE,
                                     EmptyCString(), 0, TYPE_BOOKMARK));
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::CreateFolder(PRInt64 aParent, const nsACString& aName,
                             PRInt32 aIndex, PRInt64* aNewFolder)
{
  
  NS_ENSURE_ARG_POINTER(aNewFolder);

  
  
  
  
  PRInt32 localIndex = aIndex;
  nsresult rv = CreateContainerWithID(-1, aParent, aName, EmptyString(),
                                      PR_TRUE, &localIndex, aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::CreateDynamicContainer(PRInt64 aParent,
                                       const nsACString& aName,
                                       const nsAString& aContractId,
                                       PRInt32 aIndex,
                                       PRInt64* aNewFolder)
{
  NS_ENSURE_FALSE(aContractId.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsresult rv = CreateContainerWithID(-1, aParent, aName, aContractId,
                                      PR_FALSE, &aIndex, aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetFolderReadonly(PRInt64 aFolder, PRBool* aResult)
{
  NS_ENSURE_ARG_MIN(aFolder, 1);
  NS_ENSURE_ARG_POINTER(aResult);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = annosvc->ItemHasAnnotation(aFolder, READ_ONLY_ANNO, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetFolderReadonly(PRInt64 aFolder, PRBool aReadOnly)
{
  NS_ENSURE_ARG_MIN(aFolder, 1);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv;
  if (aReadOnly) {
    rv = annosvc->SetItemAnnotationInt32(aFolder, READ_ONLY_ANNO, 1, 0,
                                         nsAnnotationService::EXPIRE_NEVER);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    PRBool hasAnno;
    rv = annosvc->ItemHasAnnotation(aFolder, READ_ONLY_ANNO, &hasAnno);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasAnno) {
      rv = annosvc->RemoveItemAnnotation(aFolder, READ_ONLY_ANNO);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


nsresult
nsNavBookmarks::CreateContainerWithID(PRInt64 aItemId,
                                      PRInt64 aParent,
                                      const nsACString& aName,
                                      const nsAString& aContractId,
                                      PRBool aIsBookmarkFolder,
                                      PRInt32* aIndex,
                                      PRInt64* aNewFolder)
{
  
  if (*aIndex < -1)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRInt32 index;
  PRInt32 folderCount;
  nsresult rv = FolderCount(aParent, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      *aIndex >= folderCount) {
    index = folderCount;
  } else {
    index = *aIndex;
    rv = AdjustIndices(aParent, index, PR_INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ItemType containerType = aIsBookmarkFolder ? FOLDER
                                             : DYNAMIC_CONTAINER;
  rv = InsertBookmarkInDB(aItemId, nsnull, containerType, aParent, index,
                          aName, PR_Now(), nsnull, aContractId, aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBatching) {
    ForceWALCheckpoint(mDBConn);
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewFolder, aParent, index, containerType,
                               nsnull));

  *aIndex = index;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::InsertSeparator(PRInt64 aParent,
                                PRInt32 aIndex,
                                PRInt64* aNewItemId)
{
  NS_ENSURE_ARG_MIN(aParent, 1);
  
  NS_ENSURE_ARG_MIN(aIndex, -1);
  NS_ENSURE_ARG_POINTER(aNewItemId);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRInt32 index;
  PRInt32 folderCount;
  nsresult rv = FolderCount(aParent, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    index = folderCount;
  }
  else {
    index = aIndex;
    rv = AdjustIndices(aParent, index, PR_INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCString voidString;
  voidString.SetIsVoid(PR_TRUE);
  rv = InsertBookmarkInDB(-1, nsnull, SEPARATOR, aParent, index,
                          voidString, PR_Now(), nsnull, EmptyString(),
                          aNewItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewItemId, aParent, index, TYPE_SEPARATOR,
                               nsnull));

  return NS_OK;
}


nsresult
nsNavBookmarks::GetLastChildId(PRInt64 aFolderId, PRInt64* aItemId)
{
  NS_ASSERTION(aFolderId > 0, "Invalid folder id");
  *aItemId = -1;

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetLastChildId);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool found;
  rv = stmt->ExecuteStep(&found);
  NS_ENSURE_SUCCESS(rv, rv);
  if (found) {
    rv = stmt->GetInt64(0, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetIdForItemAt(PRInt64 aFolder,
                               PRInt32 aIndex,
                               PRInt64* aItemId)
{
  NS_ENSURE_ARG_MIN(aFolder, 1);
  NS_ENSURE_ARG_POINTER(aItemId);

  *aItemId = -1;

  nsresult rv;
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX) {
    
    rv = GetLastChildId(aFolder, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetChildAt);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolder);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), aIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool found;
    rv = stmt->ExecuteStep(&found);
    NS_ENSURE_SUCCESS(rv, rv);
    if (found) {
      rv = stmt->GetInt64(0, aItemId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


nsresult 
nsNavBookmarks::GetParentAndIndexOfFolder(PRInt64 aFolderId,
                                          PRInt64* _aParent,
                                          PRInt32* _aIndex)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_INVALID_ARG);

  rv = stmt->GetInt64(kGetItemPropertiesIndex_Parent, _aParent);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt32(kGetItemPropertiesIndex_Position, _aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsNavBookmarks::RemoveFolder(PRInt64 aFolderId)
{
  NS_ENSURE_TRUE(aFolderId != mRoot, NS_ERROR_INVALID_ARG);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnBeforeItemRemoved(aFolderId, TYPE_FOLDER));

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsresult rv;
  PRInt64 parent;
  PRInt32 index, type;
  nsCAutoString folderType;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getInfoStmt, mDBGetItemProperties);
    rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = getInfoStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasResult) {
      return NS_ERROR_INVALID_ARG; 
    }

    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Type, &type);
    NS_ENSURE_SUCCESS(rv, rv);
     rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &parent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Position, &index);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId,
                                    folderType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  NS_ENSURE_TRUE(type == TYPE_FOLDER, NS_ERROR_INVALID_ARG);

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  rv = annosvc->RemoveItemAnnotations(aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (folderType.Length() > 0) {
    
    nsCOMPtr<nsIDynamicContainer> bmcServ = do_GetService(folderType.get());
    if (bmcServ) {
      rv = bmcServ->OnContainerRemoving(aFolderId);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                       "Remove folder container notification failed.");
    }
  }

  
  rv = RemoveFolderChildren(aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBRemoveItem);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AdjustIndices(parent, index + 1, PR_INT32_MAX, -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           parent, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBatching) {
    ForceWALCheckpoint(mDBConn);
  }

  if (aFolderId == mToolbarRoot) {
    mToolbarRoot = 0;
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemRemoved(aFolderId, parent, index, TYPE_FOLDER));

  return NS_OK;
}


NS_IMPL_ISUPPORTS1(nsNavBookmarks::RemoveFolderTransaction, nsITransaction)

NS_IMETHODIMP
nsNavBookmarks::GetRemoveFolderTransaction(PRInt64 aFolderId, nsITransaction** aResult)
{
  NS_ENSURE_ARG_MIN(aFolderId, 1);
  NS_ENSURE_ARG_POINTER(aResult);

  
  

  RemoveFolderTransaction* rft = 
    new RemoveFolderTransaction(aFolderId);
  if (!rft)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rft);
  return NS_OK;
}


nsresult
nsNavBookmarks::GetDescendantChildren(PRInt64 aFolderId,
                                      PRInt64 aGrandParentId,
                                      nsTArray<folderChildrenInfo>& aFolderChildrenArray) {
  
  PRUint32 startIndex = aFolderChildrenArray.Length();
  nsresult rv;
  {
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetChildren);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      folderChildrenInfo child;
      rv = stmt->GetInt64(nsNavHistory::kGetInfoIndex_ItemId, &child.itemId);
      NS_ENSURE_SUCCESS(rv, rv);
      child.parentId = aFolderId;
      child.grandParentId = aGrandParentId;
      PRInt32 itemType;
      rv = stmt->GetInt32(kGetChildrenIndex_Type, &itemType);
      child.itemType = (PRUint16)itemType;
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt64(kGetChildrenIndex_PlaceID, &child.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt32(kGetChildrenIndex_Position, &child.index);
      NS_ENSURE_SUCCESS(rv, rv);

      if (child.itemType == TYPE_BOOKMARK) {
        nsCAutoString URIString;
        rv = stmt->GetUTF8String(nsNavHistory::kGetInfoIndex_URL, URIString);
        NS_ENSURE_SUCCESS(rv, rv);
        child.url = URIString;
      }
      else if (child.itemType == TYPE_FOLDER) {
        nsCAutoString folderType;
        rv = stmt->GetUTF8String(kGetChildrenIndex_ServiceContractId,
                                 folderType);
        NS_ENSURE_SUCCESS(rv, rv);
        child.folderType = folderType;
      }
      
      aFolderChildrenArray.AppendElement(child);
    }
  }

  
  
  
  PRUint32 childCount = aFolderChildrenArray.Length();
  for (PRUint32 i = startIndex; i < childCount; i++) {
    if (aFolderChildrenArray[i].itemType == TYPE_FOLDER) {
      GetDescendantChildren(aFolderChildrenArray[i].itemId,
                            aFolderId,
                            aFolderChildrenArray);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RemoveFolderChildren(PRInt64 aFolderId)
{
  NS_ENSURE_ARG_MIN(aFolderId, 1);

  nsresult rv;
  PRInt32 itemType;
  PRInt64 grandParentId;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getInfoStmt, mDBGetItemProperties);
    rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRBool folderExists;
    if (NS_FAILED(getInfoStmt->ExecuteStep(&folderExists)) || !folderExists)
      return NS_ERROR_INVALID_ARG;

    
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (itemType != TYPE_FOLDER)
      return NS_ERROR_INVALID_ARG;

    
    
    
    rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &grandParentId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsTArray<folderChildrenInfo> folderChildrenArray;
  rv = GetDescendantChildren(aFolderId, grandParentId, folderChildrenArray);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCString foldersToRemove;
  for (PRUint32 i = 0; i < folderChildrenArray.Length(); i++) {
    folderChildrenInfo child = folderChildrenArray[i];

    
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavBookmarkObserver,
                     OnBeforeItemRemoved(child.itemId, child.itemType));

    if (child.itemType == TYPE_FOLDER) {
      foldersToRemove.AppendLiteral(",");
      foldersToRemove.AppendInt(child.itemId);

      
      
      
      if (child.folderType.Length() > 0) {
        nsCOMPtr<nsIDynamicContainer> bmcServ =
          do_GetService(child.folderType.get());
        if (bmcServ) {
          rv = bmcServ->OnContainerRemoving(child.itemId);
          if (NS_FAILED(rv))
            NS_WARNING("Remove folder container notification failed.");
        }
      }
    }
  }

  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsCOMPtr<mozIStorageStatement> deleteStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_bookmarks "
      "WHERE parent IN (:parent") +
        foldersToRemove +
      NS_LITERAL_CSTRING(")"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteStatement->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING(
      "DELETE FROM moz_items_annos "
      "WHERE id IN ("
        "SELECT a.id from moz_items_annos a "
        "LEFT JOIN moz_bookmarks b ON a.item_id = b.id "
        "WHERE b.id ISNULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aFolderId, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < folderChildrenArray.Length(); i++) {
    folderChildrenInfo child = folderChildrenArray[i];
    if (child.itemType == TYPE_BOOKMARK) {
      PRInt64 placeId = child.placeId;
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      rv = history->UpdateFrecency(placeId);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = UpdateKeywordsHashForRemovedBookmark(child.itemId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mBatching) {
    ForceWALCheckpoint(mDBConn);
  }

  
  for (PRInt32 i = folderChildrenArray.Length() - 1; i >= 0 ; i--) {
    folderChildrenInfo child = folderChildrenArray[i];

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavBookmarkObserver,
                     OnItemRemoved(child.itemId, child.parentId, child.index,
                                   child.itemType));

    if (child.itemType == TYPE_BOOKMARK) {
      
      
      

      if (child.grandParentId == mTagsRoot) {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), child.url);
        NS_ENSURE_SUCCESS(rv, rv);

        nsTArray<PRInt64> bookmarks;
        rv = GetBookmarkIdsForURITArray(uri, bookmarks);
        NS_ENSURE_SUCCESS(rv, rv);

        if (bookmarks.Length()) {
          for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
            NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                             nsINavBookmarkObserver,
                             OnItemChanged(bookmarks[i],
                                           NS_LITERAL_CSTRING("tags"), PR_FALSE,
                                           EmptyCString(), 0, TYPE_BOOKMARK));
          }
        }
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::MoveItem(PRInt64 aItemId, PRInt64 aNewParent, PRInt32 aIndex)
{
  NS_ENSURE_TRUE(aItemId != mRoot, NS_ERROR_INVALID_ARG);
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_MIN(aNewParent, 1);
  
  NS_ENSURE_ARG_MIN(aIndex, -1);
  
  NS_ENSURE_TRUE(aItemId != aNewParent, NS_ERROR_INVALID_ARG);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsresult rv;
  PRInt64 oldParent;
  PRInt32 oldIndex;
  PRInt32 itemType;
  nsCAutoString folderType;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getInfoStmt, mDBGetItemProperties);
    rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = getInfoStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasResult) {
      return NS_ERROR_INVALID_ARG; 
    }

    rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &oldParent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Position, &oldIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (itemType == TYPE_FOLDER) {
      rv = getInfoStmt->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId,
                                      folderType);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (oldParent == aNewParent && oldIndex == aIndex)
    return NS_OK;

  
  if (itemType == TYPE_FOLDER) {
    PRInt64 ancestorId = aNewParent;

    while (ancestorId) {
      if (ancestorId == aItemId) {
        return NS_ERROR_INVALID_ARG;
      }

      DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getInfoStmt, mDBGetItemProperties);
      rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"),
                                        ancestorId);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasResult;
      rv = getInfoStmt->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (hasResult) {
        rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &ancestorId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        break;
      }
    }
  }

  
  PRInt32 newIndex;
  PRInt32 folderCount;
  rv = FolderCount(aNewParent, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    newIndex = folderCount;
    
    
    if (oldParent == aNewParent) {
      --newIndex;
    }
  } else {
    newIndex = aIndex;

    if (oldParent == aNewParent && newIndex > oldIndex) {
      
      
      
      --newIndex;
    }
  }

  
  
  
  if (aNewParent == oldParent && newIndex == oldIndex) {
    
    return NS_OK;
  }

  
  
  
  if (oldParent == aNewParent) {
    
    
    
    if (oldIndex > newIndex) {
      rv = AdjustIndices(oldParent, newIndex, oldIndex - 1, 1);
    }
    else {
      rv = AdjustIndices(oldParent, oldIndex + 1, newIndex, -1);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    
    rv = AdjustIndices(oldParent, oldIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = AdjustIndices(aNewParent, newIndex, PR_INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBMoveItem);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aNewParent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), newIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRTime now = PR_Now();
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           oldParent, now);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aNewParent, now);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemMoved(aItemId, oldParent, oldIndex, aNewParent,
                               newIndex, itemType));

  
  if (!folderType.IsEmpty()) {
    nsCOMPtr<nsIDynamicContainer> container =
      do_GetService(folderType.get(), &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = container->OnContainerMoved(aItemId, aNewParent, newIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


nsresult
nsNavBookmarks::SetItemDateInternal(mozIStorageStatement* aStatement,
                                    PRInt64 aItemId,
                                    PRTime aValue)
{
  NS_ENSURE_STATE(aStatement);
  mozStorageStatementScoper scoper(aStatement);

  nsresult rv = aStatement->BindInt64ByName(NS_LITERAL_CSTRING("date"), aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStatement->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemDateAdded(PRInt64 aItemId, PRTime aDateAdded)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemDateInternal(GetStatement(mDBSetItemDateAdded),
                           aItemId, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aItemId, NS_LITERAL_CSTRING("dateAdded"),
                                 PR_FALSE,
                                 nsPrintfCString(16, "%lld", aDateAdded),
                                 aDateAdded, itemType));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemDateAdded(PRInt64 aItemId, PRTime* _dateAdded)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_dateAdded);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_INVALID_ARG); 

  rv = stmt->GetInt64(kGetItemPropertiesIndex_DateAdded, _dateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemLastModified(PRInt64 aItemId, PRTime aLastModified)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aItemId, aLastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aItemId, NS_LITERAL_CSTRING("lastModified"),
                                 PR_FALSE,
                                 nsPrintfCString(16, "%lld", aLastModified),
                                 aLastModified, itemType));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemLastModified(PRInt64 aItemId, PRTime* aLastModified)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aLastModified);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult)
    return NS_ERROR_INVALID_ARG; 

  rv = stmt->GetInt64(kGetItemPropertiesIndex_LastModified, aLastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


nsresult
nsNavBookmarks::GetGUIDBase(nsAString &aGUIDBase)
{
  if (!mGUIDBase.IsEmpty()) {
    aGUIDBase = mGUIDBase;
    return NS_OK;
  }

  
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1");
  NS_ENSURE_TRUE(uuidgen, NS_ERROR_OUT_OF_MEMORY);
  nsID GUID;
  nsresult rv = uuidgen->GenerateUUIDInPlace(&GUID);
  NS_ENSURE_SUCCESS(rv, rv);
  char GUIDChars[NSID_LENGTH];
  GUID.ToProvidedString(GUIDChars);
  CopyASCIItoUTF16(GUIDChars, mGUIDBase);
  aGUIDBase = mGUIDBase;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemGUID(PRInt64 aItemId, nsAString& aGUID)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = annosvc->GetItemAnnotationString(aItemId, GUID_ANNO, aGUID);
  if (NS_SUCCEEDED(rv) || rv != NS_ERROR_NOT_AVAILABLE)
    return rv;

  nsAutoString tmp;
  tmp.AppendInt(mItemCount++);
  aGUID.SetCapacity(NSID_LENGTH - 1 + tmp.Length());
  nsString GUIDBase;
  rv = GetGUIDBase(GUIDBase);
  NS_ENSURE_SUCCESS(rv, rv);
  aGUID.Assign(GUIDBase);
  aGUID.Append(tmp);

  rv = SetItemGUID(aItemId, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemGUID(PRInt64 aItemId, const nsAString& aGUID)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  PRInt64 checkId;
  GetItemIdForGUID(aGUID, &checkId);
  if (checkId != -1)
    return NS_ERROR_INVALID_ARG; 

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = annosvc->SetItemAnnotationString(aItemId, GUID_ANNO, aGUID, 0,
                                                 nsIAnnotationService::EXPIRE_NEVER);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemIdForGUID(const nsAString& aGUID, PRInt64* aItemId)
{
  NS_ENSURE_ARG_POINTER(aItemId);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemIdForGUID);
  nsresult rv = stmt->BindStringByName(NS_LITERAL_CSTRING("guid"), aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = stmt->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || ! hasMore) {
    *aItemId = -1;
    return NS_OK; 
  }

  
  rv = stmt->GetInt64(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemTitle(PRInt64 aItemId, const nsACString& aTitle)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(statement, mDBSetItemTitle);
  
  if (aTitle.IsVoid())
    rv = statement->BindNullByName(NS_LITERAL_CSTRING("item_title"));
  else
    rv = statement->BindUTF8StringByName(NS_LITERAL_CSTRING("item_title"), aTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("date"), lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aItemId, NS_LITERAL_CSTRING("title"), PR_FALSE,
                                 aTitle, lastModified, itemType));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemTitle(PRInt64 aItemId, nsACString& aTitle)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult)
    return NS_ERROR_INVALID_ARG; 

  rv = stmt->GetUTF8String(kGetItemPropertiesIndex_Title, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarkURI(PRInt64 aItemId, nsIURI** aURI)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aURI);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult)
    return NS_ERROR_INVALID_ARG; 

  PRInt32 type;
  rv = stmt->GetInt32(kGetItemPropertiesIndex_Type, &type);
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ENSURE_TRUE(type == TYPE_BOOKMARK, NS_ERROR_INVALID_ARG);

  nsCAutoString spec;
  rv = stmt->GetUTF8String(kGetItemPropertiesIndex_URI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aURI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemType(PRInt64 aItemId, PRUint16* _type)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_type);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult) {
    return NS_ERROR_INVALID_ARG; 
  }

  PRInt32 itemType;
  rv = stmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);
  *_type = itemType;

  return NS_OK;
}


nsresult
nsNavBookmarks::GetFolderType(PRInt64 aFolder, nsACString& aType)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult) {
    return NS_ERROR_INVALID_ARG;
  }

  return stmt->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId, aType);
}


nsresult
nsNavBookmarks::ResultNodeForContainer(PRInt64 aID,
                                       nsNavHistoryQueryOptions* aOptions,
                                       nsNavHistoryResultNode** aNode)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_INVALID_ARG);

  nsCAutoString title;
  rv = stmt->GetUTF8String(kGetItemPropertiesIndex_Title, title);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 itemType;
  rv = stmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (itemType == TYPE_DYNAMIC_CONTAINER) {
    
    nsCAutoString contractId;
    rv = stmt->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId,
                             contractId);
    NS_ENSURE_SUCCESS(rv, rv);
    *aNode = new nsNavHistoryContainerResultNode(EmptyCString(), title,
                                                 EmptyCString(),
                                                 nsINavHistoryResultNode::RESULT_TYPE_DYNAMIC_CONTAINER,
                                                 PR_TRUE, contractId, aOptions);
    (*aNode)->mItemId = aID;
  }
  else { 
    *aNode = new nsNavHistoryFolderResultNode(title, aOptions, aID, EmptyCString());
  }
  if (!*aNode)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = stmt->GetInt64(kGetItemPropertiesIndex_DateAdded,
                      &(*aNode)->mDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt64(kGetItemPropertiesIndex_LastModified,
                      &(*aNode)->mLastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aNode);
  return NS_OK;
}


nsresult
nsNavBookmarks::QueryFolderChildren(
  PRInt64 aFolderId,
  nsNavHistoryQueryOptions* aOptions,
  nsCOMArray<nsNavHistoryResultNode>* aChildren)
{
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ENSURE_ARG_POINTER(aChildren);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetChildren);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(stmt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index = -1;
  PRBool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    rv = ProcessFolderNodeRow(row, aOptions, aChildren, index);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


nsresult
nsNavBookmarks::ProcessFolderNodeRow(
  mozIStorageValueArray* aRow,
  nsNavHistoryQueryOptions* aOptions,
  nsCOMArray<nsNavHistoryResultNode>* aChildren,
  PRInt32& aCurrentIndex)
{
  NS_ENSURE_ARG_POINTER(aRow);
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ENSURE_ARG_POINTER(aChildren);

  
  
  
  aCurrentIndex++;

  PRInt32 itemType;
  nsresult rv = aRow->GetInt32(kGetChildrenIndex_Type, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt64 id;
  rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemId, &id);
  NS_ENSURE_SUCCESS(rv, rv);
  nsRefPtr<nsNavHistoryResultNode> node;
  if (itemType == TYPE_BOOKMARK) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->RowToResult(aRow, aOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 nodeType;
    node->GetType(&nodeType);
    if ((nodeType == nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
         aOptions->ExcludeQueries()) ||
        (nodeType != nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
         nodeType != nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT &&
         aOptions->ExcludeItems())) {
      return NS_OK;
    }
  }
  else if (itemType == TYPE_FOLDER || itemType == TYPE_DYNAMIC_CONTAINER) {
    if (aOptions->ExcludeReadOnlyFolders()) {
      
      PRBool readOnly;
      if (itemType == TYPE_DYNAMIC_CONTAINER) {
        readOnly = PR_TRUE;
      }
      else {
        readOnly = PR_FALSE;
        GetFolderReadonly(id, &readOnly);
      }
      if (readOnly)
        return NS_OK;
    }
    rv = ResultNodeForContainer(id, aOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    if (aOptions->ExcludeItems()) {
      return NS_OK;
    }
    node = new nsNavHistorySeparatorResultNode();
    NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);

    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemId, &node->mItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemDateAdded,
                        &node->mDateAdded);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemLastModified,
                        &node->mLastModified);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  node->mBookmarkIndex = aCurrentIndex;

  NS_ENSURE_TRUE(aChildren->AppendObject(node), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}


nsresult
nsNavBookmarks::QueryFolderChildrenAsync(
  nsNavHistoryFolderResultNode* aNode,
  PRInt64 aFolderId,
  mozIStoragePendingStatement** _pendingStmt)
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_ARG_POINTER(_pendingStmt);

  mozStorageStatementScoper scope(mDBGetChildren);

  nsresult rv = mDBGetChildren->BindInt64ByName(NS_LITERAL_CSTRING("parent"),
                                                aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pendingStmt;
  rv = mDBGetChildren->ExecuteAsync(aNode, getter_AddRefs(pendingStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_pendingStmt = pendingStmt);
  return NS_OK;
}


nsresult
nsNavBookmarks::FolderCount(PRInt64 aFolderId, PRInt32* _folderCount)
{
  *_folderCount = 0;
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBFolderCount);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);

  
  PRInt64 confirmFolderId;
  rv = stmt->GetInt64(1, &confirmFolderId);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(confirmFolderId == aFolderId, NS_ERROR_INVALID_ARG);

  rv = stmt->GetInt32(0, _folderCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::IsBookmarked(nsIURI* aURI, PRBool* aBookmarked)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aBookmarked);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBIsURIBookmarkedInDatabase);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->ExecuteStep(aBookmarked);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarkedURIFor(nsIURI* aURI, nsIURI** _retval)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = nsnull;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  PRInt64 placeId;
  nsresult rv = history->GetUrlIdFor(aURI, &placeId, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!placeId) {
    
    return NS_OK;
  }

  
  
  
  
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBFindRedirectedBookmark);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasBookmarkedOrigin;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasBookmarkedOrigin)) &&
      hasBookmarkedOrigin) {
    nsCAutoString spec;
    rv = stmt->GetUTF8String(0, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewURI(_retval, spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::ChangeBookmarkURI(PRInt64 aBookmarkId, nsIURI* aNewURI)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
  NS_ENSURE_ARG(aNewURI);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  PRInt64 placeId;
  nsresult rv = history->GetUrlIdFor(aNewURI, &placeId, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!placeId)
    return NS_ERROR_INVALID_ARG;

  
  
  nsCOMPtr<nsIURI> oldURI;
  PRInt64 oldPlaceId;
  rv = GetBookmarkURI(aBookmarkId, getter_AddRefs(oldURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = history->GetUrlIdFor(oldURI, &oldPlaceId, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(statement, mDBChangeBookmarkURI);
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("date"), lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = history->UpdateFrecency(placeId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = history->UpdateFrecency(oldPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = aNewURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aBookmarkId, NS_LITERAL_CSTRING("uri"),
                                 PR_FALSE, spec, lastModified, TYPE_BOOKMARK));

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetFolderIdForItem(PRInt64 aItemId, PRInt64* aFolderId)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aFolderId);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResult)
    return NS_ERROR_INVALID_ARG; 

  rv = stmt->GetInt64(kGetItemPropertiesIndex_Parent, aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(aItemId != *aFolderId, NS_ERROR_UNEXPECTED);
  return NS_OK;
}


nsresult
nsNavBookmarks::GetBookmarkIdsForURITArray(nsIURI* aURI,
                                           nsTArray<PRInt64>& aResult)
{
  NS_ENSURE_ARG(aURI);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBFindURIBookmarks);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&more))) && more) {
    PRInt64 bookmarkId;
    rv = stmt->GetInt64(kFindBookmarksIndex_ID, &bookmarkId);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(aResult.AppendElement(bookmarkId), NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarkIdsForURI(nsIURI* aURI, PRUint32* aCount,
                                     PRInt64** aBookmarks)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aBookmarks);

  *aCount = 0;
  *aBookmarks = nsnull;
  nsTArray<PRInt64> bookmarks;

  
  nsresult rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (bookmarks.Length()) {
    *aBookmarks =
      static_cast<PRInt64*>(nsMemory::Alloc(sizeof(PRInt64) * bookmarks.Length()));
    if (!*aBookmarks)
      return NS_ERROR_OUT_OF_MEMORY;
    for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
      (*aBookmarks)[i] = bookmarks[i];
  }
  *aCount = bookmarks.Length();

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemIndex(PRInt64 aItemId, PRInt32* _index)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_index);

  *_index = -1;

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasResult)
    return NS_OK;

  rv = stmt->GetInt32(kGetItemPropertiesIndex_Position, _index);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemIndex(PRInt64 aItemId, PRInt32 aNewIndex)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_MIN(aNewIndex, 0);

  nsresult rv;
  PRInt32 oldIndex = 0;
  PRInt64 parent = 0;
  PRInt32 itemType;

  {
    mozIStorageStatement* getInfoStmt(mDBGetItemProperties);
    NS_ENSURE_STATE(getInfoStmt);
    mozStorageStatementScoper scoper(getInfoStmt);
    rv = getInfoStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = getInfoStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasResult)
      return NS_OK;

    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Position, &oldIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt32(kGetItemPropertiesIndex_Type, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getInfoStmt->GetInt64(kGetItemPropertiesIndex_Parent, &parent);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt32 folderCount;
  rv = FolderCount(parent, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aNewIndex < folderCount, NS_ERROR_INVALID_ARG);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBSetItemIndex);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), aNewIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemMoved(aItemId, parent, oldIndex, parent, aNewIndex,
                               itemType));

  return NS_OK;
}


nsresult
nsNavBookmarks::UpdateKeywordsHashForRemovedBookmark(PRInt64 aItemId)
{
  nsAutoString kw;
  if (NS_SUCCEEDED(GetKeywordForBookmark(aItemId, kw)) && !kw.IsEmpty()) {
    nsresult rv = EnsureKeywordsHash();
    NS_ENSURE_SUCCESS(rv, rv);
    mBookmarkToKeywordHash.Remove(aItemId);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetKeywordForBookmark(PRInt64 aBookmarkId,
                                      const nsAString& aUserCasedKeyword)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);

  nsresult rv = EnsureKeywordsHash();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString keyword(aUserCasedKeyword);
  ToLowerCase(keyword);

  
  nsAutoString oldKeyword;
  rv = GetKeywordForBookmark(aBookmarkId, oldKeyword);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (keyword.Equals(oldKeyword) || (keyword.IsEmpty() && oldKeyword.IsEmpty()))
    return NS_OK;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsCOMPtr<mozIStorageStatement> updateBookmarkStmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks "
    "SET keyword_id = (SELECT id FROM moz_keywords WHERE keyword = :keyword), "
        "lastModified = :date "
    "WHERE id = :item_id "
  ), getter_AddRefs(updateBookmarkStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  if (keyword.IsEmpty()) {
    
    mBookmarkToKeywordHash.Remove(aBookmarkId);
    rv = updateBookmarkStmt->BindNullByName(NS_LITERAL_CSTRING("keyword"));
  }
   else {
    
    
    nsCOMPtr<mozIStorageStatement> newKeywordStmt;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT OR IGNORE INTO moz_keywords (keyword) VALUES (:keyword)"
    ), getter_AddRefs(newKeywordStmt));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newKeywordStmt->BindStringByName(NS_LITERAL_CSTRING("keyword"),
                                          keyword);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = newKeywordStmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!oldKeyword.IsEmpty())
      mBookmarkToKeywordHash.Remove(aBookmarkId);
    mBookmarkToKeywordHash.Put(aBookmarkId, keyword);
    rv = updateBookmarkStmt->BindStringByName(NS_LITERAL_CSTRING("keyword"), keyword);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = updateBookmarkStmt->BindInt64ByName(NS_LITERAL_CSTRING("date"),
                                           lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateBookmarkStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"),
                                           aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateBookmarkStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aBookmarkId, NS_LITERAL_CSTRING("keyword"),
                                 PR_FALSE, NS_ConvertUTF16toUTF8(keyword),
                                 lastModified, TYPE_BOOKMARK));

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetKeywordForURI(nsIURI* aURI, nsAString& aKeyword)
{
  NS_ENSURE_ARG(aURI);
  aKeyword.Truncate(0);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetKeywordForURI);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = stmt->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || !hasMore) {
    aKeyword.SetIsVoid(PR_TRUE);
    return NS_OK; 
  }

  
  rv = stmt->GetString(0, aKeyword);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetKeywordForBookmark(PRInt64 aBookmarkId, nsAString& aKeyword)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
  aKeyword.Truncate(0);

  nsresult rv = EnsureKeywordsHash();
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString keyword;
  if (!mBookmarkToKeywordHash.Get(aBookmarkId, &keyword)) {
    aKeyword.SetIsVoid(PR_TRUE);
  }
  else {
    aKeyword.Assign(keyword);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetURIForKeyword(const nsAString& aUserCasedKeyword,
                                 nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_TRUE(!aUserCasedKeyword.IsEmpty(), NS_ERROR_INVALID_ARG);
  *aURI = nsnull;

  
  nsAutoString keyword(aUserCasedKeyword);
  ToLowerCase(keyword);

  nsresult rv = EnsureKeywordsHash();
  NS_ENSURE_SUCCESS(rv, rv);

  keywordSearchData searchData;
  searchData.keyword.Assign(keyword);
  searchData.itemId = -1;
  mBookmarkToKeywordHash.EnumerateRead(SearchBookmarkForKeyword, &searchData);

  if (searchData.itemId == -1) {
    
    return NS_OK;
  }

  rv = GetBookmarkURI(searchData.itemId, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsNavBookmarks::EnsureKeywordsHash() {
  if (mBookmarkToKeywordHash.IsInitialized())
    return NS_OK;

  mBookmarkToKeywordHash.Init(BOOKMARKS_TO_KEYWORDS_INITIAL_CACHE_SIZE);

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT b.id, k.keyword "
    "FROM moz_bookmarks b "
    "JOIN moz_keywords k ON k.id = b.keyword_id "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 itemId;
    rv = stmt->GetInt64(0, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoString keyword;
    rv = stmt->GetString(1, keyword);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBookmarkToKeywordHash.Put(itemId, keyword);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                               nsISupports* aUserData) {
  NS_ENSURE_ARG(aCallback);

  mBatching = true;

  
  
  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = history->RunInBatchMode(aCallback, aUserData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::AddObserver(nsINavBookmarkObserver* aObserver,
                            PRBool aOwnsWeak)
{
  NS_ENSURE_ARG(aObserver);
  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}


NS_IMETHODIMP
nsNavBookmarks::RemoveObserver(nsINavBookmarkObserver* aObserver)
{
  return mObservers.RemoveWeakElement(aObserver);
}

void
nsNavBookmarks::NotifyItemVisited(const ItemVisitData& aData)
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                   OnItemVisited(aData.itemId, aData.visitId, aData.time));
}

void
nsNavBookmarks::NotifyItemChanged(const ItemChangeData& aData)
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                   OnItemChanged(aData.itemId, aData.property,
                                 aData.isAnnotation, aData.newValue,
                                 aData.lastModified, aData.itemType));
}




NS_IMETHODIMP
nsNavBookmarks::OnBeginUpdateBatch()
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver, OnBeginUpdateBatch());
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnEndUpdateBatch()
{
  if (mBatching) {
    mBatching = false;
    ForceWALCheckpoint(mDBConn);
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver, OnEndUpdateBatch());
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnVisit(nsIURI* aURI, PRInt64 aVisitId, PRTime aTime,
                        PRInt64 aSessionID, PRInt64 aReferringID,
                        PRUint32 aTransitionType, PRUint32* aAdded)
{
  
  ItemVisitData visitData;
  visitData.uri = aURI;
  visitData.visitId = aVisitId;
  visitData.time = aTime;

  nsRefPtr< AsyncGetBookmarksForURI<ItemVisitMethod, ItemVisitData> > notifier =
    new AsyncGetBookmarksForURI<ItemVisitMethod, ItemVisitData>(this, &nsNavBookmarks::NotifyItemVisited, visitData);
  notifier->Init();
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnBeforeDeleteURI(nsIURI* aURI)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnDeleteURI(nsIURI* aURI)
{
  
  ItemChangeData changeData;
  changeData.uri = aURI;
  changeData.property = NS_LITERAL_CSTRING("cleartime");
  changeData.isAnnotation = PR_FALSE;
  changeData.lastModified = 0;
  changeData.itemType = TYPE_BOOKMARK;

  nsRefPtr< AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData> > notifier =
    new AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData>(this, &nsNavBookmarks::NotifyItemChanged, changeData);
  notifier->Init();
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnClearHistory()
{
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle)
{
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnPageChanged(nsIURI* aURI, PRUint32 aWhat,
                              const nsAString& aValue)
{
  nsresult rv;
  if (aWhat == nsINavHistoryObserver::ATTRIBUTE_FAVICON) {
    ItemChangeData changeData;
    changeData.uri = aURI;
    changeData.property = NS_LITERAL_CSTRING("favicon");
    changeData.isAnnotation = PR_FALSE;
    changeData.newValue = NS_ConvertUTF16toUTF8(aValue);
    changeData.lastModified = 0;
    changeData.itemType = TYPE_BOOKMARK;

    
    PRBool isPlaceURI;
    rv = aURI->SchemeIs("place", &isPlaceURI);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isPlaceURI) {
      nsCAutoString spec;
      rv = aURI->GetSpec(spec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  
      nsCOMArray<nsNavHistoryQuery> queries;
      nsCOMPtr<nsNavHistoryQueryOptions> options;
      rv = history->QueryStringToQueryArray(spec, &queries, getter_AddRefs(options));
      NS_ENSURE_SUCCESS(rv, rv);

      if (queries.Count() == 1 && queries[0]->Folders().Length() == 1) {
        changeData.itemId = queries[0]->Folders()[0];
        NotifyItemChanged(changeData);
      }
    }
    else {
      nsRefPtr< AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData> > notifier =
        new AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData>(this, &nsNavBookmarks::NotifyItemChanged, changeData);
      notifier->Init();
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime)
{
  
  return NS_OK;
}




NS_IMETHODIMP
nsNavBookmarks::OnPageAnnotationSet(nsIURI* aPage, const nsACString& aName)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnItemAnnotationSet(PRInt64 aItemId, const nsACString& aName)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime lastModified = PR_Now();
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aItemId, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aItemId, aName, PR_TRUE, EmptyCString(),
                                 lastModified, itemType));

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnPageAnnotationRemoved(nsIURI* aPage, const nsACString& aName)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnItemAnnotationRemoved(PRInt64 aItemId, const nsACString& aName)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime lastModified = PR_Now();
  rv = SetItemDateInternal(GetStatement(mDBSetItemLastModified),
                           aItemId, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aItemId, aName, PR_TRUE, EmptyCString(),
                                 lastModified, itemType));

  return NS_OK;
}


PRBool
nsNavBookmarks::ItemExists(PRInt64 aItemId) {
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemProperties);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return hasResult;
}
