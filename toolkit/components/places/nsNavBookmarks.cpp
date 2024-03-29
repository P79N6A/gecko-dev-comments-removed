




#include "nsNavBookmarks.h"

#include "nsNavHistory.h"
#include "nsAnnotationService.h"
#include "nsPlacesMacros.h"
#include "Helpers.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"
#include "prprf.h"
#include "mozilla/storage.h"

#include "GeckoProfiler.h"

using namespace mozilla;


const int32_t nsNavBookmarks::kGetChildrenIndex_Guid = 15;
const int32_t nsNavBookmarks::kGetChildrenIndex_Position = 16;
const int32_t nsNavBookmarks::kGetChildrenIndex_Type = 17;
const int32_t nsNavBookmarks::kGetChildrenIndex_PlaceID = 18;

using namespace mozilla::places;

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsNavBookmarks, gBookmarksService)

#define BOOKMARKS_ANNO_PREFIX "bookmarks/"
#define BOOKMARKS_TOOLBAR_FOLDER_ANNO NS_LITERAL_CSTRING(BOOKMARKS_ANNO_PREFIX "toolbarFolder")
#define FEED_URI_ANNO NS_LITERAL_CSTRING("livemark/feedURI")


namespace {

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
    nsRefPtr<Database> DB = Database::GetDatabase();
    if (DB) {
      nsCOMPtr<mozIStorageAsyncStatement> stmt = DB->GetAsyncStatement(
        "/* do not warn (bug 1175249) */ "
        "SELECT b.id, b.guid, b.parent, b.lastModified, t.guid, t.parent "
        "FROM moz_bookmarks b "
        "JOIN moz_bookmarks t on t.id = b.parent "
        "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
        "ORDER BY b.lastModified DESC, b.id DESC "
      );
      if (stmt) {
        (void)URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                              mData.bookmark.url);
        nsCOMPtr<mozIStoragePendingStatement> pendingStmt;
        (void)stmt->ExecuteAsync(this, getter_AddRefs(pendingStmt));
      }
    }
  }

  NS_IMETHOD HandleResult(mozIStorageResultSet* aResultSet)
  {
    nsCOMPtr<mozIStorageRow> row;
    while (NS_SUCCEEDED(aResultSet->GetNextRow(getter_AddRefs(row))) && row) {
      
      int64_t grandParentId, tagsFolderId;
      nsresult rv = row->GetInt64(5, &grandParentId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mBookmarksSvc->GetTagsFolder(&tagsFolderId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (grandParentId == tagsFolderId) {
        continue;
      }

      mData.bookmark.grandParentId = grandParentId;
      rv = row->GetInt64(0, &mData.bookmark.id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = row->GetUTF8String(1, mData.bookmark.guid);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = row->GetInt64(2, &mData.bookmark.parentId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = row->GetUTF8String(4, mData.bookmark.parentGuid);
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


nsNavBookmarks::nsNavBookmarks()
  : mItemCount(0)
  , mRoot(0)
  , mMenuRoot(0)
  , mTagsRoot(0)
  , mUnfiledRoot(0)
  , mToolbarRoot(0)
  , mCanNotify(false)
  , mCacheObservers("bookmark-observers")
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
    gBookmarksService = nullptr;
}


NS_IMPL_ISUPPORTS(nsNavBookmarks
, nsINavBookmarksService
, nsINavHistoryObserver
, nsIAnnotationObserver
, nsIObserver
, nsISupportsWeakReference
)


nsresult
nsNavBookmarks::Init()
{
  mDB = Database::GetDatabase();
  NS_ENSURE_STATE(mDB);

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    (void)os->AddObserver(this, TOPIC_PLACES_SHUTDOWN, true);
    (void)os->AddObserver(this, TOPIC_PLACES_CONNECTION_CLOSED, true);
  }

  nsresult rv = ReadRoots();
  NS_ENSURE_SUCCESS(rv, rv);

  mCanNotify = true;

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  annosvc->AddObserver(this);

  
  
  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);
  history->AddObserver(this, true);

  

  return NS_OK;
}

nsresult
nsNavBookmarks::ReadRoots()
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDB->MainConn()->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT guid, id FROM moz_bookmarks WHERE guid IN ( "
      "'root________', 'menu________', 'toolbar_____', "
      "'tags________', 'unfiled_____' )"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    nsAutoCString guid;
    rv = stmt->GetUTF8String(0, guid);
    NS_ENSURE_SUCCESS(rv, rv);
    int64_t id;
    rv = stmt->GetInt64(1, &id);
    NS_ENSURE_SUCCESS(rv, rv);

    if (guid.EqualsLiteral("root________")) {
      mRoot = id;
    }
    else if (guid.EqualsLiteral("menu________")) {
      mMenuRoot = id;
    }
    else if (guid.EqualsLiteral("toolbar_____")) {
      mToolbarRoot = id;
    }
    else if (guid.EqualsLiteral("tags________")) {
      mTagsRoot = id;
    }
    else if (guid.EqualsLiteral("unfiled_____")) {
      mUnfiledRoot = id;
    }
  }

  if (!mRoot || !mMenuRoot || !mToolbarRoot || !mTagsRoot || !mUnfiledRoot)
    return NS_ERROR_FAILURE;

  return NS_OK;
}





nsresult
nsNavBookmarks::IsBookmarkedInDatabase(int64_t aPlaceId,
                                       bool* aIsBookmarked)
{
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT 1 FROM moz_bookmarks WHERE fk = :page_id"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->ExecuteStep(aIsBookmarked);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


nsresult
nsNavBookmarks::AdjustIndices(int64_t aFolderId,
                              int32_t aStartIndex,
                              int32_t aEndIndex,
                              int32_t aDelta)
{
  NS_ASSERTION(aStartIndex >= 0 && aEndIndex <= INT32_MAX &&
               aStartIndex <= aEndIndex, "Bad indices");

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "UPDATE moz_bookmarks SET position = position + :delta "
      "WHERE parent = :parent "
        "AND position BETWEEN :from_index AND :to_index"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

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
nsNavBookmarks::GetPlacesRoot(int64_t* aRoot)
{
  *aRoot = mRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarksMenuFolder(int64_t* aRoot)
{
  *aRoot = mMenuRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetToolbarFolder(int64_t* aFolderId)
{
  *aFolderId = mToolbarRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetTagsFolder(int64_t* aRoot)
{
  *aRoot = mTagsRoot;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetUnfiledBookmarksFolder(int64_t* aRoot)
{
  *aRoot = mUnfiledRoot;
  return NS_OK;
}


nsresult
nsNavBookmarks::InsertBookmarkInDB(int64_t aPlaceId,
                                   enum ItemType aItemType,
                                   int64_t aParentId,
                                   int32_t aIndex,
                                   const nsACString& aTitle,
                                   PRTime aDateAdded,
                                   PRTime aLastModified,
                                   const nsACString& aParentGuid,
                                   int64_t aGrandParentId,
                                   nsIURI* aURI,
                                   int64_t* _itemId,
                                   nsACString& _guid)
{
  
  MOZ_ASSERT(_itemId && (*_itemId == -1 || *_itemId > 0));
  
  MOZ_ASSERT(aPlaceId && (aPlaceId == -1 || aPlaceId > 0));

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "INSERT INTO moz_bookmarks "
      "(id, fk, type, parent, position, title, "
       "dateAdded, lastModified, guid) "
    "VALUES (:item_id, :page_id, :item_type, :parent, :item_index, "
            ":item_title, :date_added, :last_modified, "
            "IFNULL(:item_guid, GENERATE_GUID()))"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv;
  if (*_itemId != -1)
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), *_itemId);
  else
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aPlaceId != -1)
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

  
  
  if (_guid.Length() == 12) {
    MOZ_ASSERT(IsValidGUID(_guid));
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("item_guid"), _guid);
  }
  else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("item_guid"));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  if (*_itemId == -1) {
    
    nsCOMPtr<mozIStorageStatement> lastInsertIdStmt = mDB->GetStatement(
      "SELECT id, guid "
      "FROM moz_bookmarks "
      "ORDER BY ROWID DESC "
      "LIMIT 1"
    );
    NS_ENSURE_STATE(lastInsertIdStmt);
    mozStorageStatementScoper lastInsertIdScoper(lastInsertIdStmt);

    bool hasResult;
    rv = lastInsertIdStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);
    rv = lastInsertIdStmt->GetInt64(0, _itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = lastInsertIdStmt->GetUTF8String(1, _guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aParentId > 0) {
    
    
    
    rv = SetItemDateInternal(LAST_MODIFIED, aParentId, aDateAdded);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  BookmarkData bookmark;
  bookmark.id = *_itemId;
  bookmark.guid.Assign(_guid);
  if (aTitle.IsVoid()) {
    bookmark.title.SetIsVoid(true);
  }
  else {
    bookmark.title.Assign(aTitle);
  }
  bookmark.position = aIndex;
  bookmark.placeId = aPlaceId;
  bookmark.parentId = aParentId;
  bookmark.type = aItemType;
  bookmark.dateAdded = aDateAdded;
  if (aLastModified)
    bookmark.lastModified = aLastModified;
  else
    bookmark.lastModified = aDateAdded;
  if (aURI) {
    rv = aURI->GetSpec(bookmark.url);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  bookmark.parentGuid = aParentGuid;
  bookmark.grandParentId = aGrandParentId;

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::InsertBookmark(int64_t aFolder,
                               nsIURI* aURI,
                               int32_t aIndex,
                               const nsACString& aTitle,
                               const nsACString& aGUID,
                               int64_t* aNewBookmarkId)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aNewBookmarkId);
  NS_ENSURE_ARG_MIN(aIndex, nsINavBookmarksService::DEFAULT_INDEX);

  if (!aGUID.IsEmpty() && !IsValidGUID(aGUID))
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDB->MainConn(), false);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  int64_t placeId;
  nsAutoCString placeGuid;
  nsresult rv = history->GetOrCreateIdForPage(aURI, &placeId, placeGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int32_t index, folderCount;
  int64_t grandParentId;
  nsAutoCString folderGuid;
  rv = FetchFolderInfo(aFolder, &folderCount, folderGuid, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    index = folderCount;
  }
  else {
    index = aIndex;
    
    rv = AdjustIndices(aFolder, index, INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aNewBookmarkId = -1;
  PRTime dateAdded = RoundedPRNow();
  nsAutoCString guid(aGUID);
  nsCString title;
  TruncateTitle(aTitle, title);

  rv = InsertBookmarkInDB(placeId, BOOKMARK, aFolder, index, title, dateAdded,
                          0, folderGuid, grandParentId, aURI,
                          aNewBookmarkId, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (grandParentId != mTagsRoot) {
    rv = history->UpdateFrecency(placeId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewBookmarkId, aFolder, index, TYPE_BOOKMARK,
                               aURI, title, dateAdded, guid, folderGuid));

  
  
  
  if (grandParentId == mTagsRoot) {
    
    nsTArray<BookmarkData> bookmarks;
    rv = GetBookmarksForURI(aURI, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
      
      MOZ_ASSERT(bookmarks[i].id != *aNewBookmarkId);

      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavBookmarkObserver,
                       OnItemChanged(bookmarks[i].id,
                                     NS_LITERAL_CSTRING("tags"),
                                     false,
                                     EmptyCString(),
                                     bookmarks[i].lastModified,
                                     TYPE_BOOKMARK,
                                     bookmarks[i].parentId,
                                     bookmarks[i].guid,
                                     bookmarks[i].parentGuid));
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RemoveItem(int64_t aItemId)
{
  PROFILER_LABEL("nsNavBookmarks", "RemoveItem",
    js::ProfileEntry::Category::OTHER);

  NS_ENSURE_ARG(!IsRoot(aItemId));

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageTransaction transaction(mDB->MainConn(), false);

  
  if (bookmark.parentId != mTagsRoot &&
      bookmark.grandParentId != mTagsRoot) {
    nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
    NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
    rv = annosvc->RemoveItemAnnotations(bookmark.id);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (bookmark.type == TYPE_FOLDER) {
    
    rv = RemoveFolderChildren(bookmark.id);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "DELETE FROM moz_bookmarks WHERE id = :item_id"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), bookmark.id);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (bookmark.position != DEFAULT_INDEX) {
    rv = AdjustIndices(bookmark.parentId,
                       bookmark.position + 1, INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  bookmark.lastModified = RoundedPRNow();
  rv = SetItemDateInternal(LAST_MODIFIED, bookmark.parentId,
                           bookmark.lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  if (bookmark.type == TYPE_BOOKMARK) {
    
    if (bookmark.grandParentId != mTagsRoot) {
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      rv = history->UpdateFrecency(bookmark.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    (void)NS_NewURI(getter_AddRefs(uri), bookmark.url);
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemRemoved(bookmark.id,
                                 bookmark.parentId,
                                 bookmark.position,
                                 bookmark.type,
                                 uri,
                                 bookmark.guid,
                                 bookmark.parentGuid));

  if (bookmark.type == TYPE_BOOKMARK && bookmark.grandParentId == mTagsRoot &&
      uri) {
    
    
    nsTArray<BookmarkData> bookmarks;
    rv = GetBookmarksForURI(uri, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavBookmarkObserver,
                       OnItemChanged(bookmarks[i].id,
                                     NS_LITERAL_CSTRING("tags"),
                                     false,
                                     EmptyCString(),
                                     bookmarks[i].lastModified,
                                     TYPE_BOOKMARK,
                                     bookmarks[i].parentId,
                                     bookmarks[i].guid,
                                     bookmarks[i].parentGuid));
    }

  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::CreateFolder(int64_t aParent, const nsACString& aName,
                             int32_t aIndex, const nsACString& aGUID,
                             int64_t* aNewFolder)
{
  
  NS_ENSURE_ARG_POINTER(aNewFolder);

  if (!aGUID.IsEmpty() && !IsValidGUID(aGUID))
    return NS_ERROR_INVALID_ARG;

  
  
  
  
  int32_t localIndex = aIndex;
  nsresult rv = CreateContainerWithID(-1, aParent, aName, true, &localIndex,
                                      aGUID, aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

bool nsNavBookmarks::IsLivemark(int64_t aFolderId)
{
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, false);
  bool isLivemark;
  nsresult rv = annosvc->ItemHasAnnotation(aFolderId,
                                           FEED_URI_ANNO,
                                           &isLivemark);
  NS_ENSURE_SUCCESS(rv, false);
  return isLivemark;
}

nsresult
nsNavBookmarks::CreateContainerWithID(int64_t aItemId,
                                      int64_t aParent,
                                      const nsACString& aTitle,
                                      bool aIsBookmarkFolder,
                                      int32_t* aIndex,
                                      const nsACString& aGUID,
                                      int64_t* aNewFolder)
{
  NS_ENSURE_ARG_MIN(*aIndex, nsINavBookmarksService::DEFAULT_INDEX);

  
  int32_t index, folderCount;
  int64_t grandParentId;
  nsAutoCString folderGuid;
  nsresult rv = FetchFolderInfo(aParent, &folderCount, folderGuid, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageTransaction transaction(mDB->MainConn(), false);

  if (*aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      *aIndex >= folderCount) {
    index = folderCount;
  } else {
    index = *aIndex;
    
    rv = AdjustIndices(aParent, index, INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aNewFolder = aItemId;
  PRTime dateAdded = RoundedPRNow();
  nsAutoCString guid(aGUID);
  nsCString title;
  TruncateTitle(aTitle, title);

  rv = InsertBookmarkInDB(-1, FOLDER, aParent, index,
                          title, dateAdded, 0, folderGuid, grandParentId,
                          nullptr, aNewFolder, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewFolder, aParent, index, FOLDER,
                               nullptr, title, dateAdded, guid, folderGuid));

  *aIndex = index;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::InsertSeparator(int64_t aParent,
                                int32_t aIndex,
                                const nsACString& aGUID,
                                int64_t* aNewItemId)
{
  NS_ENSURE_ARG_MIN(aParent, 1);
  NS_ENSURE_ARG_MIN(aIndex, nsINavBookmarksService::DEFAULT_INDEX);
  NS_ENSURE_ARG_POINTER(aNewItemId);

  if (!aGUID.IsEmpty() && !IsValidGUID(aGUID))
    return NS_ERROR_INVALID_ARG;

  
  int32_t index, folderCount;
  int64_t grandParentId;
  nsAutoCString folderGuid;
  nsresult rv = FetchFolderInfo(aParent, &folderCount, folderGuid, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageTransaction transaction(mDB->MainConn(), false);

  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    index = folderCount;
  }
  else {
    index = aIndex;
    
    rv = AdjustIndices(aParent, index, INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aNewItemId = -1;
  
  nsCString voidString;
  voidString.SetIsVoid(true);
  nsAutoCString guid(aGUID);
  PRTime dateAdded = RoundedPRNow();
  rv = InsertBookmarkInDB(-1, SEPARATOR, aParent, index, voidString, dateAdded,
                          0, folderGuid, grandParentId, nullptr,
                          aNewItemId, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemAdded(*aNewItemId, aParent, index, TYPE_SEPARATOR,
                               nullptr, voidString, dateAdded, guid, folderGuid));

  return NS_OK;
}


nsresult
nsNavBookmarks::GetLastChildId(int64_t aFolderId, int64_t* aItemId)
{
  NS_ASSERTION(aFolderId > 0, "Invalid folder id");
  *aItemId = -1;

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT id FROM moz_bookmarks WHERE parent = :parent "
    "ORDER BY position DESC LIMIT 1"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);
  bool found;
  rv = stmt->ExecuteStep(&found);
  NS_ENSURE_SUCCESS(rv, rv);
  if (found) {
    rv = stmt->GetInt64(0, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetIdForItemAt(int64_t aFolder,
                               int32_t aIndex,
                               int64_t* aItemId)
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
    
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT id, fk, type FROM moz_bookmarks "
      "WHERE parent = :parent AND position = :item_index"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolder);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), aIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    bool found;
    rv = stmt->ExecuteStep(&found);
    NS_ENSURE_SUCCESS(rv, rv);
    if (found) {
      rv = stmt->GetInt64(0, aItemId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsNavBookmarks::RemoveFolderTransaction, nsITransaction)

NS_IMETHODIMP
nsNavBookmarks::GetRemoveFolderTransaction(int64_t aFolderId, nsITransaction** aResult)
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
nsNavBookmarks::GetDescendantFolders(int64_t aFolderId,
                                     nsTArray<int64_t>& aDescendantFoldersArray) {
  nsresult rv;
  
  uint32_t startIndex = aDescendantFoldersArray.Length();
  {
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT id "
      "FROM moz_bookmarks "
      "WHERE parent = :parent "
      "AND type = :item_type "
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_type"), TYPE_FOLDER);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore = false;
    while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      int64_t itemId;
      rv = stmt->GetInt64(0, &itemId);
      NS_ENSURE_SUCCESS(rv, rv);
      aDescendantFoldersArray.AppendElement(itemId);
    }
  }

  
  
  
  uint32_t childCount = aDescendantFoldersArray.Length();
  for (uint32_t i = startIndex; i < childCount; ++i) {
    GetDescendantFolders(aDescendantFoldersArray[i], aDescendantFoldersArray);
  }

  return NS_OK;
}


nsresult
nsNavBookmarks::GetDescendantChildren(int64_t aFolderId,
                                      const nsACString& aFolderGuid,
                                      int64_t aGrandParentId,
                                      nsTArray<BookmarkData>& aFolderChildrenArray) {
  
  uint32_t startIndex = aFolderChildrenArray.Length();
  nsresult rv;
  {
    
    
    
    
    
    
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT h.id, h.url, IFNULL(b.title, h.title), h.rev_host, h.visit_count, "
             "h.last_visit_date, f.url, b.id, b.dateAdded, b.lastModified, "
             "b.parent, null, h.frecency, h.hidden, h.guid, b.guid, "
             "b.position, b.type, b.fk "
      "FROM moz_bookmarks b "
      "LEFT JOIN moz_places h ON b.fk = h.id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.parent = :parent "
      "ORDER BY b.position ASC"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      BookmarkData child;
      rv = stmt->GetInt64(nsNavHistory::kGetInfoIndex_ItemId, &child.id);
      NS_ENSURE_SUCCESS(rv, rv);
      child.parentId = aFolderId;
      child.grandParentId = aGrandParentId;
      child.parentGuid = aFolderGuid;
      rv = stmt->GetInt32(kGetChildrenIndex_Type, &child.type);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt64(kGetChildrenIndex_PlaceID, &child.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt32(kGetChildrenIndex_Position, &child.position);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetUTF8String(kGetChildrenIndex_Guid, child.guid);
      NS_ENSURE_SUCCESS(rv, rv);

      if (child.type == TYPE_BOOKMARK) {
        rv = stmt->GetUTF8String(nsNavHistory::kGetInfoIndex_URL, child.url);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      aFolderChildrenArray.AppendElement(child);
    }
  }

  
  
  
  uint32_t childCount = aFolderChildrenArray.Length();
  for (uint32_t i = startIndex; i < childCount; ++i) {
    if (aFolderChildrenArray[i].type == TYPE_FOLDER) {
      
      
      
      
      nsCString guid = aFolderChildrenArray[i].guid;
      GetDescendantChildren(aFolderChildrenArray[i].id,
                            guid,
                            aFolderId,
                            aFolderChildrenArray);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RemoveFolderChildren(int64_t aFolderId)
{
  PROFILER_LABEL("nsNavBookmarks", "RemoveFolderChilder",
    js::ProfileEntry::Category::OTHER);

  NS_ENSURE_ARG_MIN(aFolderId, 1);
  NS_ENSURE_ARG(aFolderId != mRoot);

  BookmarkData folder;
  nsresult rv = FetchItemInfo(aFolderId, folder);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG(folder.type == TYPE_FOLDER);

  
  nsTArray<BookmarkData> folderChildrenArray;
  rv = GetDescendantChildren(folder.id, folder.guid, folder.parentId,
                             folderChildrenArray);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCString foldersToRemove;
  for (uint32_t i = 0; i < folderChildrenArray.Length(); ++i) {
    BookmarkData& child = folderChildrenArray[i];

    if (child.type == TYPE_FOLDER) {
      foldersToRemove.Append(',');
      foldersToRemove.AppendInt(child.id);
    }
  }

  
  mozStorageTransaction transaction(mDB->MainConn(), false);

  nsCOMPtr<mozIStorageStatement> deleteStatement = mDB->GetStatement(
    NS_LITERAL_CSTRING(
      "DELETE FROM moz_bookmarks "
      "WHERE parent IN (:parent") + foldersToRemove + NS_LITERAL_CSTRING(")")
  );
  NS_ENSURE_STATE(deleteStatement);
  mozStorageStatementScoper deleteStatementScoper(deleteStatement);

  rv = deleteStatement->BindInt64ByName(NS_LITERAL_CSTRING("parent"), folder.id);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDB->MainConn()->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING(
      "DELETE FROM moz_items_annos "
      "WHERE id IN ("
        "SELECT a.id from moz_items_annos a "
        "LEFT JOIN moz_bookmarks b ON a.item_id = b.id "
        "WHERE b.id ISNULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetItemDateInternal(LAST_MODIFIED, folder.id, RoundedPRNow());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (int32_t i = folderChildrenArray.Length() - 1; i >= 0; --i) {
    BookmarkData& child = folderChildrenArray[i];

    nsCOMPtr<nsIURI> uri;
    if (child.type == TYPE_BOOKMARK) {
      
      if (child.grandParentId != mTagsRoot) {
        nsNavHistory* history = nsNavHistory::GetHistoryService();
        NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
        rv = history->UpdateFrecency(child.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      
      (void)NS_NewURI(getter_AddRefs(uri), child.url);
    }

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavBookmarkObserver,
                     OnItemRemoved(child.id,
                                   child.parentId,
                                   child.position,
                                   child.type,
                                   uri,
                                   child.guid,
                                   child.parentGuid));

    if (child.type == TYPE_BOOKMARK && child.grandParentId == mTagsRoot &&
        uri) {
      
      
      
      nsTArray<BookmarkData> bookmarks;
      rv = GetBookmarksForURI(uri, bookmarks);
      NS_ENSURE_SUCCESS(rv, rv);

      for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
        NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                         nsINavBookmarkObserver,
                         OnItemChanged(bookmarks[i].id,
                                       NS_LITERAL_CSTRING("tags"),
                                       false,
                                       EmptyCString(),
                                       bookmarks[i].lastModified,
                                       TYPE_BOOKMARK,
                                       bookmarks[i].parentId,
                                       bookmarks[i].guid,
                                       bookmarks[i].parentGuid));
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::MoveItem(int64_t aItemId, int64_t aNewParent, int32_t aIndex)
{
  NS_ENSURE_ARG(!IsRoot(aItemId));
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_MIN(aNewParent, 1);
  
  NS_ENSURE_ARG_MIN(aIndex, -1);
  
  NS_ENSURE_ARG(aItemId != aNewParent);

  mozStorageTransaction transaction(mDB->MainConn(), false);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (bookmark.parentId == aNewParent && bookmark.position == aIndex)
    return NS_OK;

  
  
  if (bookmark.type == TYPE_FOLDER) {
    int64_t ancestorId = aNewParent;

    while (ancestorId) {
      if (ancestorId == bookmark.id) {
        return NS_ERROR_INVALID_ARG;
      }
      rv = GetFolderIdForItem(ancestorId, &ancestorId);
      if (NS_FAILED(rv)) {
        break;
      }
    }
  }

  
  int32_t newIndex, folderCount;
  int64_t grandParentId;
  nsAutoCString newParentGuid;
  rv = FetchFolderInfo(aNewParent, &folderCount, newParentGuid, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == nsINavBookmarksService::DEFAULT_INDEX ||
      aIndex >= folderCount) {
    newIndex = folderCount;
    
    
    if (bookmark.parentId == aNewParent) {
      --newIndex;
    }
  } else {
    newIndex = aIndex;

    if (bookmark.parentId == aNewParent && newIndex > bookmark.position) {
      
      
      
      --newIndex;
    }
  }

  
  
  
  if (aNewParent == bookmark.parentId && newIndex == bookmark.position) {
    
    return NS_OK;
  }

  
  
  
  if (bookmark.parentId == aNewParent) {
    
    
    
    if (bookmark.position > newIndex) {
      rv = AdjustIndices(bookmark.parentId, newIndex, bookmark.position - 1, 1);
    }
    else {
      rv = AdjustIndices(bookmark.parentId, bookmark.position + 1, newIndex, -1);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    
    rv = AdjustIndices(bookmark.parentId, bookmark.position + 1, INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = AdjustIndices(aNewParent, newIndex, INT32_MAX, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "UPDATE moz_bookmarks SET parent = :parent, position = :item_index "
      "WHERE id = :item_id "
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aNewParent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), newIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), bookmark.id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRTime now = RoundedPRNow();
  rv = SetItemDateInternal(LAST_MODIFIED, bookmark.parentId, now);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemDateInternal(LAST_MODIFIED, aNewParent, now);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemMoved(bookmark.id,
                               bookmark.parentId,
                               bookmark.position,
                               aNewParent,
                               newIndex,
                               bookmark.type,
                               bookmark.guid,
                               bookmark.parentGuid,
                               newParentGuid));
  return NS_OK;
}

nsresult
nsNavBookmarks::FetchItemInfo(int64_t aItemId,
                              BookmarkData& _bookmark)
{
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT b.id, h.url, b.title, b.position, b.fk, b.parent, b.type, "
           "b.dateAdded, b.lastModified, b.guid, t.guid, t.parent "
    "FROM moz_bookmarks b "
    "LEFT JOIN moz_bookmarks t ON t.id = b.parent "
    "LEFT JOIN moz_places h ON h.id = b.fk "
    "WHERE b.id = :item_id"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasResult) {
    return NS_ERROR_INVALID_ARG;
  }

  _bookmark.id = aItemId;
  rv = stmt->GetUTF8String(1, _bookmark.url);
  NS_ENSURE_SUCCESS(rv, rv);
  bool isNull;
  rv = stmt->GetIsNull(2, &isNull);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isNull) {
    _bookmark.title.SetIsVoid(true);
  }
  else {
    rv = stmt->GetUTF8String(2, _bookmark.title);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = stmt->GetInt32(3, &_bookmark.position);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt64(4, &_bookmark.placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt64(5, &_bookmark.parentId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt32(6, &_bookmark.type);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt64(7, reinterpret_cast<int64_t*>(&_bookmark.dateAdded));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetInt64(8, reinterpret_cast<int64_t*>(&_bookmark.lastModified));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->GetUTF8String(9, _bookmark.guid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = stmt->GetIsNull(10, &isNull);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isNull) {
    rv = stmt->GetUTF8String(10, _bookmark.parentGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetInt64(11, &_bookmark.grandParentId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    _bookmark.grandParentId = -1;
  }

  return NS_OK;
}

nsresult
nsNavBookmarks::SetItemDateInternal(enum BookmarkDate aDateType,
                                    int64_t aItemId,
                                    PRTime aValue)
{
  aValue = RoundToMilliseconds(aValue);

  nsCOMPtr<mozIStorageStatement> stmt;
  if (aDateType == DATE_ADDED) {
    
    
    
    stmt = mDB->GetStatement(
      "UPDATE moz_bookmarks SET dateAdded = :date, lastModified = :date "
      "WHERE id = :item_id"
    );
  }
  else {
    stmt = mDB->GetStatement(
      "UPDATE moz_bookmarks SET lastModified = :date WHERE id = :item_id"
    );
  }
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("date"), aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemDateAdded(int64_t aItemId, PRTime aDateAdded)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bookmark.dateAdded = RoundToMilliseconds(aDateAdded);

  rv = SetItemDateInternal(DATE_ADDED, bookmark.id, bookmark.dateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(bookmark.id,
                                 NS_LITERAL_CSTRING("dateAdded"),
                                 false,
                                 nsPrintfCString("%lld", bookmark.dateAdded),
                                 bookmark.dateAdded,
                                 bookmark.type,
                                 bookmark.parentId,
                                 bookmark.guid,
                                 bookmark.parentGuid));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemDateAdded(int64_t aItemId, PRTime* _dateAdded)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_dateAdded);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  *_dateAdded = bookmark.dateAdded;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemLastModified(int64_t aItemId, PRTime aLastModified)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bookmark.lastModified = RoundToMilliseconds(aLastModified);

  rv = SetItemDateInternal(LAST_MODIFIED, bookmark.id, bookmark.lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(bookmark.id,
                                 NS_LITERAL_CSTRING("lastModified"),
                                 false,
                                 nsPrintfCString("%lld", bookmark.lastModified),
                                 bookmark.lastModified,
                                 bookmark.type,
                                 bookmark.parentId,
                                 bookmark.guid,
                                 bookmark.parentGuid));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemLastModified(int64_t aItemId, PRTime* _lastModified)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_lastModified);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  *_lastModified = bookmark.lastModified;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetItemTitle(int64_t aItemId, const nsACString& aTitle)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement = mDB->GetStatement(
    "UPDATE moz_bookmarks SET title = :item_title, lastModified = :date "
    "WHERE id = :item_id "
  );
  NS_ENSURE_STATE(statement);
  mozStorageStatementScoper scoper(statement);

  nsCString title;
  TruncateTitle(aTitle, title);

  
  if (title.IsVoid()) {
    rv = statement->BindNullByName(NS_LITERAL_CSTRING("item_title"));
  }
  else {
    rv = statement->BindUTF8StringByName(NS_LITERAL_CSTRING("item_title"),
                                         title);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  bookmark.lastModified = RoundToMilliseconds(RoundedPRNow());
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("date"),
                                  bookmark.lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), bookmark.id);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(bookmark.id,
                                 NS_LITERAL_CSTRING("title"),
                                 false,
                                 title,
                                 bookmark.lastModified,
                                 bookmark.type,
                                 bookmark.parentId,
                                 bookmark.guid,
                                 bookmark.parentGuid));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemTitle(int64_t aItemId,
                             nsACString& _title)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  _title = bookmark.title;
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetBookmarkURI(int64_t aItemId,
                               nsIURI** _URI)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_URI);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(_URI, bookmark.url);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemType(int64_t aItemId, uint16_t* _type)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_type);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  *_type = static_cast<uint16_t>(bookmark.type);
  return NS_OK;
}


nsresult
nsNavBookmarks::ResultNodeForContainer(int64_t aItemId,
                                       nsNavHistoryQueryOptions* aOptions,
                                       nsNavHistoryResultNode** aNode)
{
  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  if (bookmark.type == TYPE_FOLDER) { 
    *aNode = new nsNavHistoryFolderResultNode(bookmark.title,
                                              aOptions,
                                              bookmark.id);
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  (*aNode)->mDateAdded = bookmark.dateAdded;
  (*aNode)->mLastModified = bookmark.lastModified;
  (*aNode)->mBookmarkGuid = bookmark.guid;
  (*aNode)->GetAsFolder()->mTargetFolderGuid = bookmark.guid;

  NS_ADDREF(*aNode);
  return NS_OK;
}


nsresult
nsNavBookmarks::QueryFolderChildren(
  int64_t aFolderId,
  nsNavHistoryQueryOptions* aOptions,
  nsCOMArray<nsNavHistoryResultNode>* aChildren)
{
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ENSURE_ARG_POINTER(aChildren);

  
  
  
  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT h.id, h.url, IFNULL(b.title, h.title), h.rev_host, h.visit_count, "
           "h.last_visit_date, f.url, b.id, b.dateAdded, b.lastModified, "
           "b.parent, null, h.frecency, h.hidden, h.guid, b.guid, "
           "b.position, b.type, b.fk "
    "FROM moz_bookmarks b "
    "LEFT JOIN moz_places h ON b.fk = h.id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE b.parent = :parent "
    "ORDER BY b.position ASC"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(stmt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t index = -1;
  bool hasResult;
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
  int32_t& aCurrentIndex)
{
  NS_ENSURE_ARG_POINTER(aRow);
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ENSURE_ARG_POINTER(aChildren);

  
  
  
  aCurrentIndex++;

  int32_t itemType;
  nsresult rv = aRow->GetInt32(kGetChildrenIndex_Type, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);
  int64_t id;
  rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemId, &id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsNavHistoryResultNode> node;

  if (itemType == TYPE_BOOKMARK) {
    nsNavHistory* history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->RowToResult(aRow, aOptions, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t nodeType;
    node->GetType(&nodeType);
    if ((nodeType == nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
         aOptions->ExcludeQueries()) ||
        (nodeType != nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
         nodeType != nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT &&
         aOptions->ExcludeItems())) {
      return NS_OK;
    }
  }
  else if (itemType == TYPE_FOLDER) {
    
    
    if (aOptions->ExcludeReadOnlyFolders()) {
      if (IsLivemark(id))
        return NS_OK;
    }

    nsAutoCString title;
    rv = aRow->GetUTF8String(nsNavHistory::kGetInfoIndex_Title, title);
    NS_ENSURE_SUCCESS(rv, rv);

    node = new nsNavHistoryFolderResultNode(title, aOptions, id);

    rv = aRow->GetUTF8String(kGetChildrenIndex_Guid, node->mBookmarkGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    node->GetAsFolder()->mTargetFolderGuid = node->mBookmarkGuid;

    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemDateAdded,
                        reinterpret_cast<int64_t*>(&node->mDateAdded));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemLastModified,
                        reinterpret_cast<int64_t*>(&node->mLastModified));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    if (aOptions->ExcludeItems()) {
      return NS_OK;
    }
    node = new nsNavHistorySeparatorResultNode();

    node->mItemId = id;
    rv = aRow->GetUTF8String(kGetChildrenIndex_Guid, node->mBookmarkGuid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemDateAdded,
                        reinterpret_cast<int64_t*>(&node->mDateAdded));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aRow->GetInt64(nsNavHistory::kGetInfoIndex_ItemLastModified,
                        reinterpret_cast<int64_t*>(&node->mLastModified));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  node->mBookmarkIndex = aCurrentIndex;

  NS_ENSURE_TRUE(aChildren->AppendObject(node), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}


nsresult
nsNavBookmarks::QueryFolderChildrenAsync(
  nsNavHistoryFolderResultNode* aNode,
  int64_t aFolderId,
  mozIStoragePendingStatement** _pendingStmt)
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_ARG_POINTER(_pendingStmt);

  
  
  
  
  
  nsCOMPtr<mozIStorageAsyncStatement> stmt = mDB->GetAsyncStatement(
    "SELECT h.id, h.url, IFNULL(b.title, h.title), h.rev_host, h.visit_count, "
           "h.last_visit_date, f.url, b.id, b.dateAdded, b.lastModified, "
           "b.parent, null, h.frecency, h.hidden, h.guid, b.guid, "
           "b.position, b.type, b.fk "
    "FROM moz_bookmarks b "
    "LEFT JOIN moz_places h ON b.fk = h.id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE b.parent = :parent "
    "ORDER BY b.position ASC"
  );
  NS_ENSURE_STATE(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pendingStmt;
  rv = stmt->ExecuteAsync(aNode, getter_AddRefs(pendingStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_pendingStmt = pendingStmt);
  return NS_OK;
}


nsresult
nsNavBookmarks::FetchFolderInfo(int64_t aFolderId,
                                int32_t* _folderCount,
                                nsACString& _guid,
                                int64_t* _parentId)
{
  *_folderCount = 0;
  *_parentId = -1;

  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT count(*), "
            "(SELECT guid FROM moz_bookmarks WHERE id = :parent), "
            "(SELECT parent FROM moz_bookmarks WHERE id = :parent) "
    "FROM moz_bookmarks "
    "WHERE parent = :parent"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("parent"), aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);

  
  
  bool isNull;
  rv = stmt->GetIsNull(2, &isNull);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && (!isNull || aFolderId == 0),
                 NS_ERROR_INVALID_ARG);

  rv = stmt->GetInt32(0, _folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isNull) {
    rv = stmt->GetUTF8String(1, _guid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetInt64(2, _parentId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::IsBookmarked(nsIURI* aURI, bool* aBookmarked)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aBookmarked);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT 1 FROM moz_bookmarks b "
    "JOIN moz_places h ON b.fk = h.id "
    "WHERE h.url = :page_url"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

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

  *_retval = nullptr;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  int64_t placeId;
  nsAutoCString placeGuid;
  nsresult rv = history->GetIdForPage(aURI, &placeId, placeGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!placeId) {
    
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCString query = nsPrintfCString(
    "SELECT url FROM moz_places WHERE id = ( "
      "SELECT :page_id FROM moz_bookmarks WHERE fk = :page_id "
      "UNION ALL "
      "SELECT COALESCE(grandparent.place_id, parent.place_id) AS r_place_id "
      "FROM moz_historyvisits dest "
      "LEFT JOIN moz_historyvisits parent ON parent.id = dest.from_visit "
                                        "AND dest.visit_type IN (%d, %d) "
      "LEFT JOIN moz_historyvisits grandparent ON parent.from_visit = grandparent.id "
                                             "AND parent.visit_type IN (%d, %d) "
      "WHERE dest.place_id = :page_id "
      "AND EXISTS(SELECT 1 FROM moz_bookmarks WHERE fk = r_place_id) "
      "LIMIT 1 "
    ")",
    nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
    nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY,
    nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
    nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY
  );

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(query);
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  bool hasBookmarkedOrigin;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasBookmarkedOrigin)) &&
      hasBookmarkedOrigin) {
    nsAutoCString spec;
    rv = stmt->GetUTF8String(0, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewURI(_retval, spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::ChangeBookmarkURI(int64_t aBookmarkId, nsIURI* aNewURI)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
  NS_ENSURE_ARG(aNewURI);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aBookmarkId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG(bookmark.type == TYPE_BOOKMARK);

  mozStorageTransaction transaction(mDB->MainConn(), false);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  int64_t newPlaceId;
  nsAutoCString newPlaceGuid;
  rv = history->GetOrCreateIdForPage(aNewURI, &newPlaceId, newPlaceGuid);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!newPlaceId)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<mozIStorageStatement> statement = mDB->GetStatement(
    "UPDATE moz_bookmarks SET fk = :page_id, lastModified = :date "
    "WHERE id = :item_id "
  );
  NS_ENSURE_STATE(statement);
  mozStorageStatementScoper scoper(statement);

  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), newPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);
  bookmark.lastModified = RoundedPRNow();
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("date"),
                                  bookmark.lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), bookmark.id);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = history->UpdateFrecency(newPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = history->UpdateFrecency(bookmark.placeId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString spec;
  rv = aNewURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(bookmark.id,
                                 NS_LITERAL_CSTRING("uri"),
                                 false,
                                 spec,
                                 bookmark.lastModified,
                                 bookmark.type,
                                 bookmark.parentId,
                                 bookmark.guid,
                                 bookmark.parentGuid));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetFolderIdForItem(int64_t aItemId, int64_t* _parentId)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_parentId);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(bookmark.id != bookmark.parentId, NS_ERROR_UNEXPECTED);

  *_parentId = bookmark.parentId;
  return NS_OK;
}


nsresult
nsNavBookmarks::GetBookmarkIdsForURITArray(nsIURI* aURI,
                                           nsTArray<int64_t>& aResult,
                                           bool aSkipTags)
{
  NS_ENSURE_ARG(aURI);

  
  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT b.id, b.guid, b.parent, b.lastModified, t.guid, t.parent "
    "FROM moz_bookmarks b "
    "JOIN moz_bookmarks t on t.id = b.parent "
    "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
    "ORDER BY b.lastModified DESC, b.id DESC "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool more;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&more))) && more) {
    if (aSkipTags) {
      
      int64_t grandParentId;
      nsresult rv = stmt->GetInt64(5, &grandParentId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (grandParentId == mTagsRoot) {
        continue;
      }
    }
    int64_t bookmarkId;
    rv = stmt->GetInt64(0, &bookmarkId);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(aResult.AppendElement(bookmarkId), NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavBookmarks::GetBookmarksForURI(nsIURI* aURI,
                                   nsTArray<BookmarkData>& aBookmarks)
{
  NS_ENSURE_ARG(aURI);

  
  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT b.id, b.guid, b.parent, b.lastModified, t.guid, t.parent "
    "FROM moz_bookmarks b "
    "JOIN moz_bookmarks t on t.id = b.parent "
    "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
    "ORDER BY b.lastModified DESC, b.id DESC "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool more;
  nsAutoString tags;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&more))) && more) {
    
    int64_t grandParentId;
    nsresult rv = stmt->GetInt64(5, &grandParentId);
    NS_ENSURE_SUCCESS(rv, rv);
    if (grandParentId == mTagsRoot) {
      continue;
    }

    BookmarkData bookmark;
    bookmark.grandParentId = grandParentId;
    rv = stmt->GetInt64(0, &bookmark.id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetUTF8String(1, bookmark.guid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetInt64(2, &bookmark.parentId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetInt64(3, reinterpret_cast<int64_t*>(&bookmark.lastModified));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetUTF8String(4, bookmark.parentGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(aBookmarks.AppendElement(bookmark), NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkIdsForURI(nsIURI* aURI, uint32_t* aCount,
                                     int64_t** aBookmarks)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aBookmarks);

  *aCount = 0;
  *aBookmarks = nullptr;
  nsTArray<int64_t> bookmarks;

  
  
  nsresult rv = GetBookmarkIdsForURITArray(aURI, bookmarks, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (bookmarks.Length()) {
    *aBookmarks =
      static_cast<int64_t*>(moz_xmalloc(sizeof(int64_t) * bookmarks.Length()));
    if (!*aBookmarks)
      return NS_ERROR_OUT_OF_MEMORY;
    for (uint32_t i = 0; i < bookmarks.Length(); i ++)
      (*aBookmarks)[i] = bookmarks[i];
  }

  *aCount = bookmarks.Length();
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetItemIndex(int64_t aItemId, int32_t* _index)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(_index);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  
  if (NS_FAILED(rv)) {
    *_index = -1;
    return NS_OK;
  }

  *_index = bookmark.position;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::SetItemIndex(int64_t aItemId, int32_t aNewIndex)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_MIN(aNewIndex, 0);

  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int32_t folderCount;
  int64_t grandParentId;
  nsAutoCString folderGuid;
  rv = FetchFolderInfo(bookmark.parentId, &folderCount, folderGuid, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aNewIndex < folderCount, NS_ERROR_INVALID_ARG);
  
  MOZ_ASSERT(bookmark.parentGuid == folderGuid);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "UPDATE moz_bookmarks SET position = :item_index WHERE id = :item_id"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), aItemId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_index"), aNewIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemMoved(bookmark.id,
                               bookmark.parentId,
                               bookmark.position,
                               bookmark.parentId,
                               aNewIndex,
                               bookmark.type,
                               bookmark.guid,
                               bookmark.parentGuid,
                               bookmark.parentGuid));

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::SetKeywordForBookmark(int64_t aBookmarkId,
                                      const nsAString& aUserCasedKeyword)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);

  
  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aBookmarkId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), bookmark.url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString keyword(aUserCasedKeyword);
  ToLowerCase(keyword);

  
  
  nsTArray<nsString> oldKeywords;
  {
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT keyword FROM moz_keywords WHERE place_id = :place_id"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("place_id"), bookmark.placeId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      nsString oldKeyword;
      rv = stmt->GetString(0, oldKeyword);
      NS_ENSURE_SUCCESS(rv, rv);
      oldKeywords.AppendElement(oldKeyword);
    }
  }

  
  if (keyword.IsEmpty() && oldKeywords.Length() == 0) {
    return NS_OK;
  }

  if (keyword.IsEmpty()) {
    
    for (uint32_t i = 0; i < oldKeywords.Length(); ++i) {
      nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
        "DELETE FROM moz_keywords WHERE keyword = :old_keyword"
      );
      NS_ENSURE_STATE(stmt);
      mozStorageStatementScoper scoper(stmt);
      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("old_keyword"),
                                  oldKeywords[i]);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsTArray<BookmarkData> bookmarks;
    rv = GetBookmarksForURI(uri, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);
    for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavBookmarkObserver,
                       OnItemChanged(bookmarks[i].id,
                                     NS_LITERAL_CSTRING("keyword"),
                                     false,
                                     EmptyCString(),
                                     bookmarks[i].lastModified,
                                     TYPE_BOOKMARK,
                                     bookmarks[i].parentId,
                                     bookmarks[i].guid,
                                     bookmarks[i].parentGuid));
    }

    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIURI> oldUri;
  {
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT url "
      "FROM moz_keywords "
      "JOIN moz_places h ON h.id = place_id "
      "WHERE keyword = :keyword"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("keyword"), keyword);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    if (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      nsAutoCString spec;
      rv = stmt->GetUTF8String(0, spec);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = NS_NewURI(getter_AddRefs(oldUri), spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  if (oldUri) {
    
    nsTArray<BookmarkData> bookmarks;
    rv = GetBookmarksForURI(oldUri, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);
    for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavBookmarkObserver,
                       OnItemChanged(bookmarks[i].id,
                                     NS_LITERAL_CSTRING("keyword"),
                                     false,
                                     EmptyCString(),
                                     bookmarks[i].lastModified,
                                     TYPE_BOOKMARK,
                                     bookmarks[i].parentId,
                                     bookmarks[i].guid,
                                     bookmarks[i].parentGuid));
    }

    stmt = mDB->GetStatement(
      "UPDATE moz_keywords SET place_id = :place_id WHERE keyword = :keyword"
    );
    NS_ENSURE_STATE(stmt);
  }
  else {
    stmt = mDB->GetStatement(
      "INSERT INTO moz_keywords (keyword, place_id) "
      "VALUES (:keyword, :place_id)"
    );
  }
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("place_id"), bookmark.placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("keyword"), keyword);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsTArray<BookmarkData> bookmarks;
  rv = GetBookmarksForURI(uri, bookmarks);
  NS_ENSURE_SUCCESS(rv, rv);
  for (uint32_t i = 0; i < bookmarks.Length(); ++i) {
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavBookmarkObserver,
                     OnItemChanged(bookmarks[i].id,
                                   NS_LITERAL_CSTRING("keyword"),
                                   false,
                                   NS_ConvertUTF16toUTF8(keyword),
                                   bookmarks[i].lastModified,
                                   TYPE_BOOKMARK,
                                   bookmarks[i].parentId,
                                   bookmarks[i].guid,
                                   bookmarks[i].parentGuid));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetKeywordForBookmark(int64_t aBookmarkId, nsAString& aKeyword)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
  aKeyword.Truncate(0);

  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(NS_LITERAL_CSTRING(
    "SELECT k.keyword "
    "FROM moz_bookmarks b "
    "JOIN moz_keywords k ON k.place_id = b.fk "
    "WHERE b.id = :item_id "
    "ORDER BY k.ROWID DESC "
    "LIMIT 1"
  ));
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"),
                             aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString keyword;
    rv = stmt->GetString(0, keyword);
    NS_ENSURE_SUCCESS(rv, rv);
    aKeyword = keyword;
    return NS_OK;
  }

  aKeyword.SetIsVoid(true);

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::GetURIForKeyword(const nsAString& aUserCasedKeyword,
                                 nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_TRUE(!aUserCasedKeyword.IsEmpty(), NS_ERROR_INVALID_ARG);
  *aURI = nullptr;

  PLACES_WARN_DEPRECATED();

  
  nsAutoString keyword(aUserCasedKeyword);
  ToLowerCase(keyword);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(NS_LITERAL_CSTRING(
    "SELECT h.url "
    "FROM moz_places h "
    "JOIN moz_keywords k ON k.place_id = h.id "
    "WHERE k.keyword = :keyword"
  ));
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindStringByName(NS_LITERAL_CSTRING("keyword"), keyword);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoCString spec;
    rv = stmt->GetUTF8String(0, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), spec);
    NS_ENSURE_SUCCESS(rv, rv);
    uri.forget(aURI);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                               nsISupports* aUserData) {
  PROFILER_LABEL("nsNavBookmarks", "RunInBatchMode",
    js::ProfileEntry::Category::OTHER);

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
                            bool aOwnsWeak)
{
  NS_ENSURE_ARG(aObserver);
  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}


NS_IMETHODIMP
nsNavBookmarks::RemoveObserver(nsINavBookmarkObserver* aObserver)
{
  return mObservers.RemoveWeakElement(aObserver);
}

NS_IMETHODIMP
nsNavBookmarks::GetObservers(uint32_t* _count,
                             nsINavBookmarkObserver*** _observers)
{
  NS_ENSURE_ARG_POINTER(_count);
  NS_ENSURE_ARG_POINTER(_observers);

  *_count = 0;
  *_observers = nullptr;

  if (!mCanNotify)
    return NS_OK;

  nsCOMArray<nsINavBookmarkObserver> observers;

  
  mCacheObservers.GetEntries(observers);

  
  for (uint32_t i = 0; i < mObservers.Length(); ++i) {
    const nsCOMPtr<nsINavBookmarkObserver> &observer = mObservers.ElementAt(i);
    
    if (observer)
      observers.AppendElement(observer);
  }

  if (observers.Count() == 0)
    return NS_OK;

  *_observers = static_cast<nsINavBookmarkObserver**>
    (moz_xmalloc(observers.Count() * sizeof(nsINavBookmarkObserver*)));
  NS_ENSURE_TRUE(*_observers, NS_ERROR_OUT_OF_MEMORY);

  *_count = observers.Count();
  for (uint32_t i = 0; i < *_count; ++i) {
    NS_ADDREF((*_observers)[i] = observers[i]);
  }

  return NS_OK;
}

void
nsNavBookmarks::NotifyItemVisited(const ItemVisitData& aData)
{
  nsCOMPtr<nsIURI> uri;
  (void)NS_NewURI(getter_AddRefs(uri), aData.bookmark.url);
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemVisited(aData.bookmark.id,
                                 aData.visitId,
                                 aData.time,
                                 aData.transitionType,
                                 uri,
                                 aData.bookmark.parentId,
                                 aData.bookmark.guid,
                                 aData.bookmark.parentGuid));
}

void
nsNavBookmarks::NotifyItemChanged(const ItemChangeData& aData)
{
  
  MOZ_ASSERT(!aData.bookmark.guid.IsEmpty());
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(aData.bookmark.id,
                                 aData.property,
                                 aData.isAnnotation,
                                 aData.newValue,
                                 aData.bookmark.lastModified,
                                 aData.bookmark.type,
                                 aData.bookmark.parentId,
                                 aData.bookmark.guid,
                                 aData.bookmark.parentGuid));
}




NS_IMETHODIMP
nsNavBookmarks::Observe(nsISupports *aSubject, const char *aTopic,
                        const char16_t *aData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  if (strcmp(aTopic, TOPIC_PLACES_SHUTDOWN) == 0) {
    
    nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
    if (annosvc) {
      annosvc->RemoveObserver(this);
    }
  }
  else if (strcmp(aTopic, TOPIC_PLACES_CONNECTION_CLOSED) == 0) {
    
    
    mCanNotify = false;
  }

  return NS_OK;
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
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver, OnEndUpdateBatch());
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnVisit(nsIURI* aURI, int64_t aVisitId, PRTime aTime,
                        int64_t aSessionID, int64_t aReferringID,
                        uint32_t aTransitionType, const nsACString& aGUID,
                        bool aHidden)
{
  
  ItemVisitData visitData;
  nsresult rv = aURI->GetSpec(visitData.bookmark.url);
  NS_ENSURE_SUCCESS(rv, rv);
  visitData.visitId = aVisitId;
  visitData.time = aTime;
  visitData.transitionType = aTransitionType;

  nsRefPtr< AsyncGetBookmarksForURI<ItemVisitMethod, ItemVisitData> > notifier =
    new AsyncGetBookmarksForURI<ItemVisitMethod, ItemVisitData>(this, &nsNavBookmarks::NotifyItemVisited, visitData);
  notifier->Init();
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnDeleteURI(nsIURI* aURI,
                            const nsACString& aGUID,
                            uint16_t aReason)
{
#ifdef DEBUG
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  int64_t placeId;
  nsAutoCString placeGuid;
  MOZ_ASSERT(
    history && NS_SUCCEEDED(history->GetIdForPage(aURI, &placeId, placeGuid)) && !placeId,
    "OnDeleteURI was notified for a page that still exists?"
  );
#endif
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnClearHistory()
{
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnTitleChanged(nsIURI* aURI,
                               const nsAString& aPageTitle,
                               const nsACString& aGUID)
{
  
  
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnFrecencyChanged(nsIURI* aURI,
                                  int32_t aNewFrecency,
                                  const nsACString& aGUID,
                                  bool aHidden,
                                  PRTime aLastVisitDate)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnManyFrecenciesChanged()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnPageChanged(nsIURI* aURI,
                              uint32_t aChangedAttribute,
                              const nsAString& aNewValue,
                              const nsACString& aGUID)
{
  nsresult rv;
  if (aChangedAttribute == nsINavHistoryObserver::ATTRIBUTE_FAVICON) {
    ItemChangeData changeData;
    rv = aURI->GetSpec(changeData.bookmark.url);
    NS_ENSURE_SUCCESS(rv, rv);
    changeData.property = NS_LITERAL_CSTRING("favicon");
    changeData.isAnnotation = false;
    changeData.newValue = NS_ConvertUTF16toUTF8(aNewValue);
    changeData.bookmark.lastModified = 0;
    changeData.bookmark.type = TYPE_BOOKMARK;

    
    bool isPlaceURI;
    rv = aURI->SchemeIs("place", &isPlaceURI);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isPlaceURI) {
      nsNavHistory* history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

      nsCOMArray<nsNavHistoryQuery> queries;
      nsCOMPtr<nsNavHistoryQueryOptions> options;
      rv = history->QueryStringToQueryArray(changeData.bookmark.url,
                                            &queries, getter_AddRefs(options));
      NS_ENSURE_SUCCESS(rv, rv);

      if (queries.Count() == 1 && queries[0]->Folders().Length() == 1) {
        
        rv = FetchItemInfo(queries[0]->Folders()[0], changeData.bookmark);
        NS_ENSURE_SUCCESS(rv, rv);
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
nsNavBookmarks::OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime,
                               const nsACString& aGUID,
                               uint16_t aReason, uint32_t aTransitionType)
{
  
  if (!aVisitTime) {
    
    ItemChangeData changeData;
    nsresult rv = aURI->GetSpec(changeData.bookmark.url);
    NS_ENSURE_SUCCESS(rv, rv);
    changeData.property = NS_LITERAL_CSTRING("cleartime");
    changeData.isAnnotation = false;
    changeData.bookmark.lastModified = 0;
    changeData.bookmark.type = TYPE_BOOKMARK;

    nsRefPtr< AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData> > notifier =
      new AsyncGetBookmarksForURI<ItemChangeMethod, ItemChangeData>(this, &nsNavBookmarks::NotifyItemChanged, changeData);
    notifier->Init();
  }
  return NS_OK;
}




NS_IMETHODIMP
nsNavBookmarks::OnPageAnnotationSet(nsIURI* aPage, const nsACString& aName)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnItemAnnotationSet(int64_t aItemId, const nsACString& aName)
{
  BookmarkData bookmark;
  nsresult rv = FetchItemInfo(aItemId, bookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  bookmark.lastModified = RoundedPRNow();
  rv = SetItemDateInternal(LAST_MODIFIED, bookmark.id, bookmark.lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavBookmarkObserver,
                   OnItemChanged(bookmark.id,
                                 aName,
                                 true,
                                 EmptyCString(),
                                 bookmark.lastModified,
                                 bookmark.type,
                                 bookmark.parentId,
                                 bookmark.guid,
                                 bookmark.parentGuid));
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnPageAnnotationRemoved(nsIURI* aPage, const nsACString& aName)
{
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::OnItemAnnotationRemoved(int64_t aItemId, const nsACString& aName)
{
  
  
  nsresult rv = OnItemAnnotationSet(aItemId, aName);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
