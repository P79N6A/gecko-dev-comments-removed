





































#include "nsAppDirectoryServiceDefs.h"
#include "nsNavBookmarks.h"
#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsIRemoteContainer.h"
#include "nsUnicharUtils.h"
#include "nsFaviconService.h"
#include "nsAnnotationService.h"

const PRInt32 nsNavBookmarks::kFindBookmarksIndex_ID = 0;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_ItemChild = 1;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_FolderChild = 2;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Parent = 3;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Position = 4;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Title = 5;

const PRInt32 nsNavBookmarks::kGetFolderInfoIndex_FolderID = 0;
const PRInt32 nsNavBookmarks::kGetFolderInfoIndex_Title = 1;
const PRInt32 nsNavBookmarks::kGetFolderInfoIndex_Type = 2;


const PRInt32 nsNavBookmarks::kGetChildrenIndex_Position = 9;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_ItemChild = 10;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_FolderChild = 11;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_FolderTitle = 12;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_ID = 13;

const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_ID = 0;
const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_URI = 1;
const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_Title = 2;
const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_Position = 3;
const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_PlaceID = 4;
const PRInt32 nsNavBookmarks::kGetBookmarkPropertiesIndex_Parent = 5;

nsNavBookmarks* nsNavBookmarks::sInstance = nsnull;

#define BOOKMARKS_ANNO_PREFIX "bookmarks/"
#define ANNO_FOLDER_READONLY BOOKMARKS_ANNO_PREFIX "readonly"

nsNavBookmarks::nsNavBookmarks()
  : mRoot(0), mBookmarksRoot(0), mTagRoot(0), mToolbarFolder(0), mBatchLevel(0),
    mBatchHasTransaction(PR_FALSE)
{
  NS_ASSERTION(!sInstance, "Multiple nsNavBookmarks instances!");
  sInstance = this;
}

nsNavBookmarks::~nsNavBookmarks()
{
  NS_ASSERTION(sInstance == this, "Expected sInstance == this");
  sInstance = nsnull;
}

NS_IMPL_ISUPPORTS2(nsNavBookmarks,
                   nsINavBookmarksService, nsINavHistoryObserver)

nsresult
nsNavBookmarks::Init()
{
  nsNavHistory *history = History();
  NS_ENSURE_TRUE(history, NS_ERROR_UNEXPECTED);
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT id, name, type FROM moz_bookmarks_folders WHERE id = ?1"),
                               getter_AddRefs(mDBGetFolderInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsCOMPtr<mozIStorageStatement> statement;
    rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT folder_child FROM moz_bookmarks WHERE parent IS NULL"),
                                 getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    rv = statement->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    if (results) {
      mRoot = statement->AsInt64(0);
    }
  }

  nsCAutoString buffer;

  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = bundleService->CreateBundle(
      "chrome://browser/locale/places/places.properties",
      getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT a.* "
      "FROM moz_bookmarks a, moz_places h "
      "WHERE h.url = ?1 AND a.item_child = h.id"),
    getter_AddRefs(mDBFindURIBookmarks));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  NS_NAMED_LITERAL_CSTRING(selectItemChildren,
    "SELECT h.id, h.url, a.title, "
      "(SELECT title FROM moz_bookmarks WHERE item_child = h.id), "
      "h.rev_host, h.visit_count, "
      "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
      "f.url, null, a.position, a.item_child, a.folder_child, null, a.id "
    "FROM moz_bookmarks a "
    "JOIN moz_places h ON a.item_child = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE a.parent = ?1 AND a.position >= ?2 AND a.position <= ?3 ");

  
  
  
  
  
  NS_NAMED_LITERAL_CSTRING(selectFolderChildren,
    "SELECT null, null, null, null, null, null, null, null, null, a.position, a.item_child, a.folder_child, c.name, a.id "
    "FROM moz_bookmarks a "
    "JOIN moz_bookmarks_folders c ON c.id = a.folder_child "
    "WHERE a.parent = ?1 AND a.position >= ?2 AND a.position <= ?3");

  
  
  
  
  
  
  NS_NAMED_LITERAL_CSTRING(selectSeparatorChildren,
    "SELECT null, null, null, null, null, null, null, null, null, a.position, null, null, null, a.id "
    "FROM moz_bookmarks a "
    "WHERE a.parent = ?1 AND a.position >= ?2 AND a.position <= ?3 AND "
    "a.item_child ISNULL and a.folder_child ISNULL");

  NS_NAMED_LITERAL_CSTRING(orderByPosition, " ORDER BY a.position");

  
  rv = dbConn->CreateStatement(selectItemChildren +
                               NS_LITERAL_CSTRING(" UNION ALL ") +
                               selectFolderChildren +
                               NS_LITERAL_CSTRING(" UNION ALL ") +
                               selectSeparatorChildren + orderByPosition,
                               getter_AddRefs(mDBGetChildren));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT COUNT(*) FROM moz_bookmarks WHERE parent = ?1"),
                               getter_AddRefs(mDBFolderCount));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT position FROM moz_bookmarks WHERE folder_child = ?1 AND parent = ?2"),
                               getter_AddRefs(mDBIndexOfFolder));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT item_child, folder_child, id FROM moz_bookmarks WHERE parent = ?1 AND position = ?2"),
                               getter_AddRefs(mDBGetChildAt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.id, p.url, b.title, b.position, b.item_child, b.parent "
      "FROM moz_bookmarks b "
      "JOIN moz_places p ON b.item_child = p.id "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBGetBookmarkProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT dest_v.place_id "
      "FROM moz_historyvisits source_v "
      "LEFT JOIN moz_historyvisits dest_v ON dest_v.from_visit = source_v.id "
      "WHERE source_v.place_id = ?1 "
      "AND source_v.visit_date >= ?2 "
      "AND (dest_v.visit_type = 5 OR dest_v.visit_type = 6) "
      "GROUP BY dest_v.place_id"),
    getter_AddRefs(mDBGetRedirectDestinations));
  NS_ENSURE_SUCCESS(rv, rv);

  FillBookmarksHash();

  
  

  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT k.keyword FROM moz_bookmarks b "
      "JOIN moz_keywords k ON k.id = b.keyword_id "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBGetKeywordForBookmark));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT k.keyword " 
      "FROM moz_places p "
      "JOIN moz_bookmarks b ON b.item_child = p.id "
      "JOIN moz_keywords k ON k.id = b.keyword_id "
      "WHERE p.url = ?1"),
    getter_AddRefs(mDBGetKeywordForURI));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT p.url FROM moz_keywords k "
      "JOIN moz_bookmarks b ON b.keyword_id = k.id "
      "JOIN moz_places p ON b.item_child = p.id "
      "WHERE k.keyword = ?1"),
    getter_AddRefs(mDBGetURIForKeyword));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitRoots();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitToolbarFolder();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  history->AddObserver(this, PR_FALSE);

  

  return NS_OK;
}








nsresult 
nsNavBookmarks::InitTables(mozIStorageConnection* aDBConn)
{
  PRBool exists;
  nsresult rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_bookmarks"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_bookmarks ("
        "id INTEGER PRIMARY KEY,"
        "item_child INTEGER, "
        "folder_child INTEGER, "
        "parent INTEGER, "
        "position INTEGER, "
        "title LONGVARCHAR, "
        "keyword_id INTEGER)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_bookmarks_itemindex ON moz_bookmarks (item_child)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_bookmarks_parentindex ON moz_bookmarks (parent)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  
  
  
  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_bookmarks_folders"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_bookmarks_folders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name LONGVARCHAR, "
        "type LONGVARCHAR)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_bookmarks_roots"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_bookmarks_roots ("
        "root_name VARCHAR(16) UNIQUE, "
        "folder_id INTEGER)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_keywords"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE TABLE moz_keywords ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "keyword TEXT UNIQUE)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


struct RenumberItem {
  PRInt64 folderChild;
  nsCOMPtr<nsIURI> itemURI;
  PRInt32 position;
};

struct RenumberItemsArray {
  nsVoidArray items;
  ~RenumberItemsArray();
};

RenumberItemsArray::~RenumberItemsArray()
{
  for (PRInt32 i = 0; i < items.Count(); ++i) {
    delete NS_STATIC_CAST(RenumberItem*, items[i]);
  }
}






















nsresult
nsNavBookmarks::InitRoots()
{
  nsCOMPtr<mozIStorageStatement> getRootStatement;
  nsresult rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING("SELECT folder_id FROM moz_bookmarks_roots WHERE root_name = ?1"),
                                 getter_AddRefs(getRootStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool importDefaults = PR_FALSE;
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("places"), &mRoot, &importDefaults);
  NS_ENSURE_SUCCESS(rv, rv);

  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("menu"), &mBookmarksRoot, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("tags"), &mTagRoot, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_PLACES_BOOKMARKS
  if (importDefaults) {
    
    
    nsCOMPtr<nsIURI> defaultPlaces;
    rv = NS_NewURI(getter_AddRefs(defaultPlaces),
                   NS_LITERAL_CSTRING("chrome://browser/locale/places/default_places.html"),
                   nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ImportBookmarksHTMLInternal(defaultPlaces, PR_TRUE, 0, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIFile> bookmarksFile;
    rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                                getter_AddRefs(bookmarksFile));
    if (bookmarksFile) {
      PRBool bookmarksFileExists;
      rv = bookmarksFile->Exists(&bookmarksFileExists);
      if (NS_SUCCEEDED(rv) && bookmarksFileExists) {
        nsCOMPtr<nsIIOService> ioservice = do_GetService(
                                    "@mozilla.org/network/io-service;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIURI> bookmarksFileURI;
        rv = ioservice->NewFileURI(bookmarksFile,
                                   getter_AddRefs(bookmarksFileURI));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = ImportBookmarksHTMLInternal(bookmarksFileURI, PR_FALSE,
                                         0, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
#endif
  return NS_OK;
}




nsresult
nsNavBookmarks::InitToolbarFolder()
{
  mozIStorageConnection *dbConn = DBConn();

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT id from moz_bookmarks_folders WHERE type = 'toolbar'"),
                                        getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = statement->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasResult) {
    rv = statement->GetInt64(0, &mToolbarFolder);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}










nsresult
nsNavBookmarks::CreateRoot(mozIStorageStatement* aGetRootStatement,
                           const nsCString& name, PRInt64* aID,
                           PRBool* aWasCreated)
{
  PRBool hasResult = PR_FALSE;
  nsresult rv = aGetRootStatement->BindUTF8StringParameter(0, name);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aGetRootStatement->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasResult) {
    if (aWasCreated)
      *aWasCreated = PR_FALSE;
    rv = aGetRootStatement->GetInt64(0, aID);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(*aID != 0, "Root is 0 for some reason, folders can't have 0 ID");
    return NS_OK;
  }
  if (aWasCreated)
    *aWasCreated = PR_TRUE;

  
  nsCOMPtr<mozIStorageStatement> insertStatement;
  rv = CreateFolder(0, NS_LITERAL_STRING(""), nsINavBookmarksService::DEFAULT_INDEX, aID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_bookmarks_roots (root_name,folder_id) VALUES (?1, ?2)"),
                                 getter_AddRefs(insertStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insertStatement->BindUTF8StringParameter(0, name);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insertStatement->BindInt64Parameter(1, *aID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insertStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}











nsresult
nsNavBookmarks::FillBookmarksHash()
{
  PRBool hasMore;

  
  NS_ENSURE_TRUE(mBookmarksHash.Init(1024), NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id "
      "FROM moz_bookmarks b "
      "LEFT JOIN moz_places h ON b.item_child = h.id where b.item_child IS NOT NULL"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 pageID;
    rv = statement->GetInt64(0, &pageID);
    NS_ENSURE_TRUE(mBookmarksHash.Put(pageID, pageID), NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  
  
  
  
  
  rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT v1.place_id, v2.place_id "
      "FROM moz_bookmarks b "
      "LEFT JOIN moz_historyvisits v1 on b.item_child = v1.place_id "
      "LEFT JOIN moz_historyvisits v2 on v2.from_visit = v1.id "
      "WHERE b.item_child IS NOT NULL "
      "AND v2.visit_type = 5 OR v2.visit_type = 6 " 
      "GROUP BY v2.place_id"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 fromId, toId;
    statement->GetInt64(0, &fromId);
    statement->GetInt64(1, &toId);

    NS_ENSURE_TRUE(mBookmarksHash.Put(toId, fromId), NS_ERROR_OUT_OF_MEMORY);

    
    rv = RecursiveAddBookmarkHash(fromId, toId, 0);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}













nsresult
nsNavBookmarks::AddBookmarkToHash(PRInt64 aBookmarkId, PRTime aMinTime)
{
  
  
  
  if (! mBookmarksHash.IsInitialized())
    return NS_OK;
  if (! mBookmarksHash.Put(aBookmarkId, aBookmarkId))
    return NS_ERROR_OUT_OF_MEMORY;
  return RecursiveAddBookmarkHash(aBookmarkId, aBookmarkId, aMinTime);
}














nsresult
nsNavBookmarks::RecursiveAddBookmarkHash(PRInt64 aBookmarkID,
                                         PRInt64 aCurrentSource,
                                         PRTime aMinTime)
{
  nsresult rv;
  nsTArray<PRInt64> found;

  
  
  
  {
    mozStorageStatementScoper scoper(mDBGetRedirectDestinations);
    rv = mDBGetRedirectDestinations->BindInt64Parameter(0, aCurrentSource);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetRedirectDestinations->BindInt64Parameter(1, aMinTime);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    while (NS_SUCCEEDED(mDBGetRedirectDestinations->ExecuteStep(&hasMore)) &&
           hasMore) {

      
      PRInt64 curID;
      rv = mDBGetRedirectDestinations->GetInt64(0, &curID);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      PRInt64 alreadyExistingOne;
      if (mBookmarksHash.Get(curID, &alreadyExistingOne))
        continue;

      if (! mBookmarksHash.Put(curID, aBookmarkID))
        return NS_ERROR_OUT_OF_MEMORY;

      
      found.AppendElement(curID);
    }
  }

  
  for (PRUint32 i = 0; i < found.Length(); i ++) {
    rv = RecursiveAddBookmarkHash(aBookmarkID, found[i], aMinTime);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}












PR_STATIC_CALLBACK(PLDHashOperator)
RemoveBookmarkHashCallback(nsTrimInt64HashKey::KeyType aKey,
                           PRInt64& aBookmark, void* aUserArg)
{
  const PRInt64* removeThisOne = NS_REINTERPRET_CAST(const PRInt64*, aUserArg);
  if (aBookmark == *removeThisOne)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}
nsresult
nsNavBookmarks::UpdateBookmarkHashOnRemove(PRInt64 aPlaceId)
{
  
  
  PRBool inDB;
  nsresult rv = IsBookmarkedInDatabase(aPlaceId, &inDB);
  NS_ENSURE_SUCCESS(rv, rv);
  if (inDB)
    return NS_OK; 

  
  mBookmarksHash.Enumerate(RemoveBookmarkHashCallback,
                           NS_REINTERPRET_CAST(void*, &aPlaceId));
  return NS_OK;
}







nsresult
nsNavBookmarks::IsBookmarkedInDatabase(PRInt64 aPlaceId,
                                       PRBool *aIsBookmarked)
{
  
  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT position FROM moz_bookmarks WHERE item_child = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  return statement->ExecuteStep(aIsBookmarked);
}


nsresult
nsNavBookmarks::AdjustIndices(PRInt64 aFolder,
                              PRInt32 aStartIndex, PRInt32 aEndIndex,
                              PRInt32 aDelta)
{
  NS_ASSERTION(aStartIndex <= aEndIndex, "start index must be <= end index");

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  nsCAutoString buffer;
  buffer.AssignLiteral("UPDATE moz_bookmarks SET position = position + ");
  buffer.AppendInt(aDelta);
  buffer.AppendLiteral(" WHERE parent = ");
  buffer.AppendInt(aFolder);

  if (aStartIndex != 0) {
    buffer.AppendLiteral(" AND position >= ");
    buffer.AppendInt(aStartIndex);
  }
  if (aEndIndex != PR_INT32_MAX) {
    buffer.AppendLiteral(" AND position <= ");
    buffer.AppendInt(aEndIndex);
  }

  nsresult rv = DBConn()->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  RenumberItemsArray itemsArray;
  nsVoidArray *items = &itemsArray.items;
  {
    mozStorageStatementScoper scope(mDBGetChildren);
 
    rv = mDBGetChildren->BindInt64Parameter(0, aFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBGetChildren->BindInt32Parameter(1, aStartIndex + aDelta);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBGetChildren->BindInt32Parameter(2, aEndIndex + aDelta);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    while (NS_SUCCEEDED(mDBGetChildren->ExecuteStep(&results)) && results) {
      RenumberItem *item = new RenumberItem();
      if (!item) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      if (mDBGetChildren->IsNull(kGetChildrenIndex_ItemChild)) {
        item->folderChild = mDBGetChildren->AsInt64(kGetChildrenIndex_FolderChild);
      } else {
        nsCAutoString spec;
        mDBGetChildren->GetUTF8String(nsNavHistory::kGetInfoIndex_URL, spec);
        rv = NS_NewURI(getter_AddRefs(item->itemURI), spec, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      item->position = mDBGetChildren->AsInt32(kGetChildrenIndex_Position);
      if (!items->AppendElement(item)) {
        delete item;
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  nsBookmarksUpdateBatcher batch;

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetPlacesRoot(PRInt64 *aRoot)
{
  *aRoot = mRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarksRoot(PRInt64 *aRoot)
{
  *aRoot = mBookmarksRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetToolbarFolder(PRInt64 *aFolderId)
{
  *aFolderId = mToolbarFolder;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::SetToolbarFolder(PRInt64 aFolderId)
{
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  

  
  nsCAutoString buffer;
  buffer.AssignLiteral("UPDATE moz_bookmarks_folders SET type = '' WHERE id = ");
  buffer.AppendInt(mToolbarFolder);

  nsresult rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  buffer = "";
  buffer.AssignLiteral("UPDATE moz_bookmarks_folders SET type = 'toolbar' ");
  buffer.AppendLiteral("WHERE id = ");
  buffer.AppendInt(aFolderId);

  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  mToolbarFolder = aFolderId;

  
  nsCOMPtr<nsIURI> folderURI;
  rv = GetFolderURI(aFolderId, getter_AddRefs(folderURI));
  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnItemChanged(mToolbarFolder, folderURI, NS_LITERAL_CSTRING("became_toolbar_folder"),
                                    EmptyString()));
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetTagRoot(PRInt64 *aRoot)
{
  *aRoot = mTagRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::InsertItem(PRInt64 aFolder, nsIURI *aItem, PRInt32 aIndex, PRInt64 *aNewBookmarkId)
{
  
  if (aIndex < nsINavBookmarksService::DEFAULT_INDEX)
    return NS_ERROR_INVALID_ARG;
  NS_ENSURE_ARG_POINTER(aNewBookmarkId);

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  PRInt64 childID;
  nsresult rv = History()->GetUrlIdFor(aItem, &childID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index = (aIndex == -1) ? FolderCount(aFolder) : aIndex;

  rv = AdjustIndices(aFolder, index, PR_INT32_MAX, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString buffer;
  buffer.AssignLiteral("INSERT INTO moz_bookmarks (item_child, parent, position) VALUES (");
  buffer.AppendInt(childID);
  buffer.AppendLiteral(", ");
  buffer.AppendInt(aFolder);
  buffer.AppendLiteral(", ");
  buffer.AppendInt(index);
  buffer.AppendLiteral(")");

  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 rowId;
  rv = dbConn->GetLastInsertRowID(&rowId);
  NS_ENSURE_SUCCESS(rv, rv);
  *aNewBookmarkId = rowId;

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  AddBookmarkToHash(childID, 0);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnItemAdded(rowId, aItem, aFolder, index))

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::RemoveItem(PRInt64 aItemId)
{
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  nsresult rv;
  PRInt32 childIndex;
  PRInt64 placeId, folderId;
  nsCOMPtr<nsIURI> uri;
  nsCAutoString buffer, spec;
  { 
    mozStorageStatementScoper scope(mDBGetBookmarkProperties);
    mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);

    PRBool results;
    rv = mDBGetBookmarkProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!results)
      return NS_ERROR_INVALID_ARG; 

    childIndex = mDBGetBookmarkProperties->AsInt32(kGetBookmarkPropertiesIndex_Position);
    placeId = mDBGetBookmarkProperties->AsInt64(kGetBookmarkPropertiesIndex_PlaceID);
    folderId = mDBGetBookmarkProperties->AsInt64(kGetBookmarkPropertiesIndex_Parent);
    rv = mDBGetBookmarkProperties->GetUTF8String(kGetBookmarkPropertiesIndex_URI, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewURI(getter_AddRefs(uri), spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  buffer.AssignLiteral("DELETE FROM moz_bookmarks WHERE id = ");
  buffer.AppendInt(aItemId);

  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  if (childIndex != -1) {
    rv = AdjustIndices(folderId, childIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateBookmarkHashOnRemove(placeId);
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnItemRemoved(aItemId, uri, folderId, childIndex))

  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::CreateFolder(PRInt64 aParent, const nsAString &aName,
                             PRInt32 aIndex, PRInt64 *aNewFolder)
{
  
  
  
  
  PRInt32 localIndex = aIndex;
  return CreateFolderWithID(-1, aParent, aName, PR_TRUE, &localIndex, aNewFolder);
}

NS_IMETHODIMP
nsNavBookmarks::CreateContainer(PRInt64 aParent, const nsAString &aName,
                                const nsAString &aType, PRInt32 aIndex,
                                PRInt64 *aNewFolder)
{
  return CreateContainerWithID(-1, aParent, aName, aType, aIndex, aNewFolder);
}

nsresult
nsNavBookmarks::CreateFolderWithID(PRInt64 aFolder, PRInt64 aParent,
                                   const nsAString& aName,
                                   PRBool aSendNotifications,
                                   PRInt32* aIndex, PRInt64* aNewFolder)
{
  
  if (*aIndex < -1)
    return NS_ERROR_INVALID_ARG;

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  PRInt32 index = (*aIndex == -1) ? FolderCount(aParent) : *aIndex;

  nsresult rv = AdjustIndices(aParent, index, PR_INT32_MAX, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsCOMPtr<mozIStorageStatement> statement;
    if (aFolder == -1) {
      rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_bookmarks_folders (name, type) VALUES (?1, null)"),
                                   getter_AddRefs(statement));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = statement->BindStringParameter(0, aName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_bookmarks_folders (id, name, type) VALUES (?1, ?2, null)"),
                                   getter_AddRefs(statement));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = statement->BindInt64Parameter(0, aFolder);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindStringParameter(1, aName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = statement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt64 child;
  rv = dbConn->GetLastInsertRowID(&child);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString buffer;
  buffer.AssignLiteral("INSERT INTO moz_bookmarks (folder_child, parent, position) VALUES (");
  buffer.AppendInt(child);
  buffer.AppendLiteral(", ");
  buffer.AppendInt(aParent);
  buffer.AppendLiteral(", ");
  buffer.AppendInt(index);
  buffer.AppendLiteral(")");
  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (aSendNotifications) {
    ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                        OnFolderAdded(child, aParent, index))
  }

  *aIndex = index;
  *aNewFolder = child;
  return NS_OK;
}

nsresult 
nsNavBookmarks::CreateContainerWithID(PRInt64 aFolder, PRInt64 aParent, 
                                      const nsAString &aName, const nsAString &aType, 
                                      PRInt32 aIndex, PRInt64 *aNewFolder)
{
  
  
  
  
  
  PRInt32 localIndex = aIndex;
  nsresult rv = CreateFolderWithID(aFolder, aParent, aName, PR_FALSE, &localIndex, aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_bookmarks_folders SET type = ?2 WHERE id = ?1"),
                                 getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, *aNewFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(1, aType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnFolderAdded(*aNewFolder, aParent, localIndex))

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::InsertSeparator(PRInt64 aParent, PRInt32 aIndex)
{
  
  if (aIndex < -1)
    return NS_ERROR_INVALID_ARG;

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  PRInt32 index = (aIndex == -1) ? FolderCount(aParent) : aIndex;

  nsresult rv = AdjustIndices(aParent, index, PR_INT32_MAX, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_bookmarks "
                                          "(parent, position) VALUES (?1,?2)"),
                               getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aParent);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt32Parameter(1, index);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnSeparatorAdded(aParent, index))

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::RemoveChildAt(PRInt64 aParent, PRInt32 aIndex)
{
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);
  nsresult rv;
  PRInt64 item, folder;

  {
    mozStorageStatementScoper scope(mDBGetChildAt);
    rv = mDBGetChildAt->BindInt64Parameter(0, aParent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetChildAt->BindInt32Parameter(1, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    rv = mDBGetChildAt->ExecuteStep(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMore) {
      
      return NS_ERROR_INVALID_ARG;
    }

    if (mDBGetChildAt->IsNull(0)) {
      item = 0;
      folder = mDBGetChildAt->AsInt64(1);
    } else {
      folder = 0;
      item = mDBGetChildAt->AsInt64(2);
    }
  }

  if (item != 0) {
    
    rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    return RemoveItem(item);
  }
  if (folder != 0) {
    
    rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    return RemoveFolder(folder);
  }

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_bookmarks WHERE parent = ?1 AND position = ?2"),
                               getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aParent);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt32Parameter(1, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AdjustIndices(aParent, aIndex + 1, PR_INT32_MAX, -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnSeparatorRemoved(aParent, aIndex))
  return NS_OK;
}

nsresult 
nsNavBookmarks::GetParentAndIndexOfFolder(PRInt64 aFolder, PRInt64* aParent, 
                                          PRInt32* aIndex)
{
  nsCAutoString buffer;
  buffer.AssignLiteral("SELECT parent, position FROM moz_bookmarks WHERE folder_child = ");
  buffer.AppendInt(aFolder);

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = DBConn()->CreateStatement(buffer, getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = statement->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!results) {
    return NS_ERROR_INVALID_ARG; 
  }

  *aParent = statement->AsInt64(0);
  *aIndex = statement->AsInt32(1);
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::RemoveFolder(PRInt64 aFolder)
{
  
  nsCAutoString folderType;
  nsresult rv = GetFolderType(aFolder, folderType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (folderType.Length() > 0) {
    
    nsCOMPtr<nsIRemoteContainer> bmcServ = do_GetService(folderType.get());
    if (bmcServ) {
      rv = bmcServ->OnContainerRemoving(aFolder);
      if (NS_FAILED(rv))
        NS_WARNING("Remove folder container notification failed.");
    }
  }

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  PRInt64 parent;
  PRInt32 index;
  rv = GetParentAndIndexOfFolder(aFolder, &parent, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  
  RemoveFolderChildren(aFolder);

  
  nsCAutoString buffer;
  buffer.AssignLiteral("DELETE FROM moz_bookmarks WHERE folder_child = ");
  buffer.AppendInt(aFolder);
  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  buffer.AssignLiteral("DELETE FROM moz_bookmarks_folders WHERE id = ");
  buffer.AppendInt(aFolder);
  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AdjustIndices(parent, index + 1, PR_INT32_MAX, -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnFolderRemoved(aFolder, parent, index))

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsNavBookmarks::RemoveFolderTransaction, nsITransaction)

NS_IMETHODIMP
nsNavBookmarks::GetRemoveFolderTransaction(PRInt64 aFolder, nsITransaction** aResult)
{
  
  

  nsAutoString title;
  nsresult rv = GetFolderTitle(aFolder, title);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 parent;
  PRInt32 index;
  rv = GetParentAndIndexOfFolder(aFolder, &parent, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString type;
  rv = GetFolderType(aFolder, type);
  NS_ENSURE_SUCCESS(rv, rv);

  RemoveFolderTransaction* rft = 
    new RemoveFolderTransaction(aFolder, parent, title, index, type);
  if (!rft)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rft);
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::RemoveFolderChildren(PRInt64 aFolder)
{
  mozStorageTransaction transaction(DBConn(), PR_FALSE);

  nsTArray<PRInt32> separatorChildren; 
  nsTArray<PRInt64> folderChildren;
  nsTArray<PRInt64> itemChildren;
  nsresult rv;
  {
    mozStorageStatementScoper scope(mDBGetChildren);
    rv = mDBGetChildren->BindInt64Parameter(0, aFolder);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetChildren->BindInt32Parameter(1, 0);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetChildren->BindInt32Parameter(2, PR_INT32_MAX);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    while (NS_SUCCEEDED(mDBGetChildren->ExecuteStep(&hasMore)) && hasMore) {
      PRBool isFolder = ! mDBGetChildren->IsNull(kGetChildrenIndex_FolderChild);
      if (isFolder) {
        
        folderChildren.AppendElement(
            mDBGetChildren->AsInt64(kGetChildrenIndex_FolderChild));
      } else if (mDBGetChildren->IsNull(kGetChildrenIndex_ItemChild)) {
        
        
        separatorChildren.AppendElement(mDBGetChildren->AsInt32(kGetChildrenIndex_Position));
      } else {
        
        itemChildren.AppendElement(mDBGetChildren->AsInt32(kGetChildrenIndex_ID));
      }
    }
  }

  
  
  
  PRUint32 i;
  for (i = separatorChildren.Length() - 1; i != PRInt32(-1); --i) {
    rv = RemoveChildAt(aFolder, separatorChildren[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  for (i = 0; i < folderChildren.Length(); ++i) {
    rv = RemoveFolder(folderChildren[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  for (i = 0; i < itemChildren.Length(); ++i) {
    rv = RemoveItem(itemChildren[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  transaction.Commit();
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::MoveFolder(PRInt64 aFolder, PRInt64 aNewParent, PRInt32 aIndex)
{
  
  if (aIndex < -1)
    return NS_ERROR_INVALID_ARG;

  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("SELECT parent, position FROM moz_bookmarks WHERE folder_child = ?1"),
                                        getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 parent;
  PRInt32 oldIndex;
  {
    mozStorageStatementScoper scope(statement);
    rv = statement->BindInt64Parameter(0, aFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    rv = statement->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!results) {
      return NS_ERROR_INVALID_ARG; 
    }

    parent = statement->AsInt64(0);
    oldIndex = statement->AsInt32(1);
  }

  
  {
    mozStorageStatementScoper scope(statement);
    PRInt64 p = aNewParent;

    while (p) {
      if (p == aFolder) {
        return NS_ERROR_INVALID_ARG;
      }

      rv = statement->BindInt64Parameter(0, p);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool results;
      rv = statement->ExecuteStep(&results);
      NS_ENSURE_SUCCESS(rv, rv);
      p = results ? statement->AsInt64(0) : 0;
    }
  }

  PRInt32 newIndex;
  if (aIndex == -1) {
    newIndex = FolderCount(aNewParent);
    
    
    if (parent == aNewParent) {
      --newIndex;
    }
  } else {
    newIndex = aIndex;

    if (parent == aNewParent && newIndex > oldIndex) {
      
      
      
      --newIndex;
    }
  }

  if (aNewParent == parent && newIndex == oldIndex) {
    
    return NS_OK;
  }

  
  nsCAutoString buffer;
  buffer.AssignLiteral("DELETE FROM moz_bookmarks WHERE folder_child = ");
  buffer.AppendInt(aFolder);
  rv = dbConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (parent == aNewParent) {
    
    
    
    if (oldIndex > newIndex) {
      rv = AdjustIndices(parent, newIndex, oldIndex - 1, 1);
    } else {
      rv = AdjustIndices(parent, oldIndex + 1, newIndex, -1);
    }
  } else {
    
    
    rv = AdjustIndices(parent, oldIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = AdjustIndices(aNewParent, newIndex, PR_INT32_MAX, 1);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsCOMPtr<mozIStorageStatement> statement;
    rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
         "INSERT INTO moz_bookmarks (folder_child, parent, position) "
         "VALUES (?1, ?2, ?3)"),
                                 getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = statement->BindInt64Parameter(0, aFolder);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = statement->BindInt64Parameter(1, aNewParent);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = statement->BindInt32Parameter(2, newIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = statement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnFolderMoved(aFolder, parent, oldIndex,
                                    aNewParent, newIndex))

  
  nsCAutoString type;
  rv = GetFolderType(aFolder, type);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! type.IsEmpty()) {
    nsCOMPtr<nsIRemoteContainer> container =
      do_GetService(type.get(), &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = container->OnContainerMoved(aFolder, aNewParent, newIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetChildFolder(PRInt64 aFolder, const nsAString& aSubFolder,
                               PRInt64* _result)
{
  
  nsresult rv;
  if (aFolder == 0)
    return NS_ERROR_INVALID_ARG;

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING("SELECT c.id FROM moz_bookmarks a JOIN moz_bookmarks_folders c ON a.folder_child = c.id WHERE a.parent = ?1 AND c.name = ?2"),
                                 getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  statement->BindInt64Parameter(0, aFolder);
  statement->BindStringParameter(1, aSubFolder);

  PRBool hasResult = PR_FALSE;
  rv = statement->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (! hasResult) {
    
    *_result = 0;
    return NS_OK;
  }

  return statement->GetInt64(0, _result);
}

NS_IMETHODIMP
nsNavBookmarks::SetItemTitle(PRInt64 aItemId, const nsAString &aTitle)
{
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_bookmarks SET title = ?1 WHERE id = ?2"),
                               getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(0, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(1, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> uri;
  nsCAutoString spec;
  PRBool results;
  {
    mozStorageStatementScoper scope(mDBGetBookmarkProperties);
    rv = mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetBookmarkProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    mDBGetBookmarkProperties->GetUTF8String(kGetBookmarkPropertiesIndex_URI, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewURI(getter_AddRefs(uri), spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnItemChanged(aItemId, uri, NS_LITERAL_CSTRING("title"),
                                    aTitle));

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemTitle(PRInt64 aItemId, nsAString &aTitle)
{
  mozStorageStatementScoper scope(mDBGetBookmarkProperties);

  nsresult rv = mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetBookmarkProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  return mDBGetBookmarkProperties->GetString(kGetBookmarkPropertiesIndex_Title, aTitle);
}

NS_IMETHODIMP
nsNavBookmarks::GetItemURI(PRInt64 aItemId, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  
  
  
  nsCAutoString spec("place:moz_bookmarks.id=");
  spec.AppendInt(aItemId);
  spec.AppendLiteral("&group=3"); 
  return NS_NewURI(aURI, spec);
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkURI(PRInt64 aItemId, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  mozStorageStatementScoper scope(mDBGetBookmarkProperties);
  nsresult rv = mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetBookmarkProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  nsCAutoString spec;
  rv = mDBGetBookmarkProperties->GetUTF8String(kGetBookmarkPropertiesIndex_URI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aURI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemIndex(PRInt64 aItemId, PRInt32 *aIndex)
{
  mozStorageStatementScoper scope(mDBGetBookmarkProperties);
  nsresult rv = mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetBookmarkProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  rv = mDBGetBookmarkProperties->GetInt32(kGetBookmarkPropertiesIndex_Position, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::SetFolderTitle(PRInt64 aFolder, const nsAString &aTitle)
{
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_bookmarks_folders SET name = ?2 WHERE id = ?1"),
                                 getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(1, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                      OnFolderChanged(aFolder, NS_LITERAL_CSTRING("title")))

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderTitle(PRInt64 aFolder, nsAString &aTitle)
{
  mozStorageStatementScoper scope(mDBGetFolderInfo);
  nsresult rv = mDBGetFolderInfo->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetFolderInfo->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results) {
    return NS_ERROR_INVALID_ARG;
  }

  return mDBGetFolderInfo->GetString(kGetFolderInfoIndex_Title, aTitle);
}

nsresult
nsNavBookmarks::GetFolderType(PRInt64 aFolder, nsACString &aType)
{
  mozStorageStatementScoper scope(mDBGetFolderInfo);
  nsresult rv = mDBGetFolderInfo->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetFolderInfo->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results) {
    return NS_ERROR_INVALID_ARG;
  }

  return mDBGetFolderInfo->GetUTF8String(kGetFolderInfoIndex_Type, aType);
}

nsresult
nsNavBookmarks::ResultNodeForFolder(PRInt64 aID,
                                    nsNavHistoryQueryOptions *aOptions,
                                    nsNavHistoryResultNode **aNode)
{
  mozStorageStatementScoper scope(mDBGetFolderInfo);
  mDBGetFolderInfo->BindInt64Parameter(0, aID);

  PRBool results;
  nsresult rv = mDBGetFolderInfo->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(results, "ResultNodeForFolder expects a valid folder id");

  
  nsCAutoString folderType;
  rv = mDBGetFolderInfo->GetUTF8String(kGetFolderInfoIndex_Type, folderType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString title;
  rv = mDBGetFolderInfo->GetUTF8String(kGetFolderInfoIndex_Title, title);

  *aNode = new nsNavHistoryFolderResultNode(title, aOptions, aID, folderType);
  if (! *aNode)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aNode);
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderURI(PRInt64 aFolder, nsIURI **aURI)
{
  
  
  
  
  
  
  
  
  
  nsCAutoString spec("place:folder=");
  spec.AppendInt(aFolder);
  spec.AppendLiteral("&group=3"); 
  return NS_NewURI(aURI, spec);
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderReadonly(PRInt64 aFolder, PRBool *aResult)
{
  
  *aResult = PR_FALSE;
  nsCAutoString type;
  nsresult rv = GetFolderType(aFolder, type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!type.IsEmpty()) {
    nsCOMPtr<nsIRemoteContainer> container =
      do_GetService(type.get(), &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = container->GetChildrenReadOnly(aResult);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsNavBookmarks::QueryFolderChildren(PRInt64 aFolderId,
                                    nsNavHistoryQueryOptions *aOptions,
                                    nsCOMArray<nsNavHistoryResultNode> *aChildren)
{
  mozStorageStatementScoper scope(mDBGetChildren);

  nsresult rv = mDBGetChildren->BindInt64Parameter(0, aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetChildren->BindInt32Parameter(1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetChildren->BindInt32Parameter(2, PR_INT32_MAX);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions, &rv);
  PRInt32 index = -1;
  while (NS_SUCCEEDED(mDBGetChildren->ExecuteStep(&results)) && results) {

    
    
    
    
    index ++;

    PRBool isFolder = !mDBGetChildren->IsNull(kGetChildrenIndex_FolderChild);
    nsCOMPtr<nsNavHistoryResultNode> node;
    if (isFolder) {
      PRInt64 folder = mDBGetChildren->AsInt64(kGetChildrenIndex_FolderChild);

      if (options->ExcludeReadOnlyFolders()) {
        
        PRBool readOnly = PR_FALSE;
        GetFolderReadonly(folder, &readOnly);
        if (readOnly)
          continue; 
      }

      rv = ResultNodeForFolder(folder, aOptions, getter_AddRefs(node));
      if (NS_FAILED(rv))
        continue;
    } else if (mDBGetChildren->IsNull(kGetChildrenIndex_ItemChild)) {
      
      if (aOptions->ExcludeItems()) {
        continue;
      }
      node = new nsNavHistorySeparatorResultNode();
      NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);
    } else {
      rv = History()->RowToResult(mDBGetChildren, options,
                                  getter_AddRefs(node));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 nodeType;
      node->GetType(&nodeType);
      if ((nodeType == nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
           aOptions->ExcludeQueries()) ||
          (nodeType != nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
           nodeType != nsINavHistoryResultNode::RESULT_TYPE_FOLDER &&
           aOptions->ExcludeItems())) {
        continue;
      }
    }

    
    
    node->mBookmarkIndex = index;

    
    node->mBookmarkId = mDBGetChildren->AsInt64(kGetChildrenIndex_ID);

    NS_ENSURE_TRUE(aChildren->AppendObject(node), NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}

PRInt32
nsNavBookmarks::FolderCount(PRInt64 aFolder)
{
  mozStorageStatementScoper scope(mDBFolderCount);

  nsresult rv = mDBFolderCount->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, 0);

  PRBool results;
  rv = mDBFolderCount->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  return mDBFolderCount->AsInt32(0);
}

NS_IMETHODIMP
nsNavBookmarks::IsBookmarked(nsIURI *aURI, PRBool *aBookmarked)
{
  nsNavHistory* history = History();
  NS_ENSURE_TRUE(history, NS_ERROR_UNEXPECTED);

  
  PRInt64 urlID;
  nsresult rv = history->GetUrlIdFor(aURI, &urlID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! urlID) {
    
    *aBookmarked = PR_FALSE;
    return NS_OK;
  }

  PRInt64 bookmarkedID;
  PRBool foundOne = mBookmarksHash.Get(urlID, &bookmarkedID);

  
  
  if (foundOne)
    *aBookmarked = (urlID == bookmarkedID);
  else
    *aBookmarked = PR_FALSE;

#ifdef DEBUG
  
  PRBool realBookmarked;
  rv = IsBookmarkedInDatabase(urlID, &realBookmarked);
  NS_ASSERTION(realBookmarked == *aBookmarked,
               "Bookmark hash table out-of-sync with the database");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkedURIFor(nsIURI* aURI, nsIURI** _retval)
{
  *_retval = nsnull;

  nsNavHistory* history = History();
  NS_ENSURE_TRUE(history, NS_ERROR_UNEXPECTED);

  
  PRInt64 urlID;
  nsresult rv = history->GetUrlIdFor(aURI, &urlID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! urlID) {
    
    return NS_OK;
  }

  PRInt64 bookmarkID;
  if (mBookmarksHash.Get(urlID, &bookmarkID)) {
    
    mozIStorageStatement* statement = history->DBGetIdPageInfo();
    NS_ENSURE_TRUE(statement, NS_ERROR_UNEXPECTED);
    mozStorageStatementScoper scoper(statement);

    rv = statement->BindInt64Parameter(0, bookmarkID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    if (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
      nsCAutoString spec;
      statement->GetUTF8String(nsNavHistory::kGetInfoIndex_URL, spec);
      return NS_NewURI(_retval, spec);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::ChangeBookmarkURI(PRInt64 aBookmarkId, nsIURI *aNewURI)
{
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);

  PRInt64 placeId;
  nsresult rv = History()->GetUrlIdFor(aNewURI, &placeId, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!placeId)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<mozIStorageStatement> statement;
  rv = dbConn->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_bookmarks SET item_child = ?1 WHERE id = ?2"),
                               getter_AddRefs(statement));
  statement->BindInt64Parameter(0, placeId);
  statement->BindInt64Parameter(1, aBookmarkId);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
    OnItemChanged(aBookmarkId, aNewURI, NS_LITERAL_CSTRING("uri"),
                  EmptyString()))

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderIdForItem(PRInt64 aItemId, PRInt64 *aFolderId)
{
  NS_ENSURE_ARG_POINTER(aFolderId);

  mozStorageStatementScoper scope(mDBGetBookmarkProperties);
  nsresult rv = mDBGetBookmarkProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetBookmarkProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  return mDBGetBookmarkProperties->GetInt64(kGetBookmarkPropertiesIndex_Parent, aFolderId);
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkIdsForURITArray(nsIURI *aURI,
                                         nsTArray<PRInt64> *aResult) 
{
  mozStorageStatementScoper scope(mDBFindURIBookmarks);
  mozStorageTransaction transaction(DBConn(), PR_FALSE);

  nsresult rv = BindStatementURI(mDBFindURIBookmarks, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  while (NS_SUCCEEDED((rv = mDBFindURIBookmarks->ExecuteStep(&more))) && more) {
    if (! aResult->AppendElement(
        mDBFindURIBookmarks->AsInt64(kFindBookmarksIndex_ID)))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  return transaction.Commit();
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkIdsForURI(nsIURI *aURI, PRUint32 *aCount,
                                   PRInt64 **aBookmarks)
{
  *aCount = 0;
  *aBookmarks = nsnull;
  nsTArray<PRInt64> bookmarks;

  
  nsresult rv = GetBookmarkIdsForURITArray(aURI, &bookmarks);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (bookmarks.Length()) {
    *aBookmarks = NS_STATIC_CAST(PRInt64*,
                           nsMemory::Alloc(sizeof(PRInt64) * bookmarks.Length()));
    if (! *aBookmarks)
      return NS_ERROR_OUT_OF_MEMORY;
    for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
      (*aBookmarks)[i] = bookmarks[i];
  }
  *aCount = bookmarks.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::IndexOfFolder(PRInt64 aParent,
                              PRInt64 aFolder, PRInt32 *aIndex)
{
  mozStorageTransaction transaction(DBConn(), PR_FALSE);

  mozStorageStatementScoper scope(mDBIndexOfFolder);
  mDBIndexOfFolder->BindInt64Parameter(0, aFolder);
  mDBIndexOfFolder->BindInt64Parameter(1, aParent);
  PRBool results;
  nsresult rv = mDBIndexOfFolder->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!results) {
    *aIndex = -1;
    return NS_OK;
  }

  *aIndex = mDBIndexOfFolder->AsInt32(0);
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::SetKeywordForBookmark(PRInt64 aBookmarkId, const nsAString& aKeyword)
{
  if (aBookmarkId < 1)
    return NS_ERROR_INVALID_ARG; 

  
  nsAutoString kwd(aKeyword);
  ToLowerCase(kwd);

  mozStorageTransaction transaction(DBConn(), PR_FALSE);
  nsresult rv;
  PRBool results;
  PRInt64 keywordId;

  if (!kwd.IsEmpty()) {
    
    nsCOMPtr<mozIStorageStatement> getKeywordStmnt;
    rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT id from moz_keywords WHERE keyword = ?1"),
      getter_AddRefs(getKeywordStmnt));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = getKeywordStmnt->BindStringParameter(0, kwd);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getKeywordStmnt->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);

    if (results) {
      rv = getKeywordStmnt->GetInt64(0, &keywordId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      nsCOMPtr<mozIStorageStatement> addKeywordStmnt;
      rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
          "INSERT INTO moz_keywords (keyword) VALUES (?1)"),
        getter_AddRefs(addKeywordStmnt));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = addKeywordStmnt->BindStringParameter(0, kwd);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = addKeywordStmnt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
      rv = DBConn()->GetLastInsertRowID(&keywordId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  nsCOMPtr<mozIStorageStatement> updateKeywordStmnt;
  rv = DBConn()->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET keyword_id = ?1 WHERE id = ?2"),
    getter_AddRefs(updateKeywordStmnt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->BindInt64Parameter(0, keywordId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->BindInt64Parameter(1, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetKeywordForURI(nsIURI* aURI, nsAString& aKeyword)
{
  aKeyword.Truncate(0);

  mozStorageStatementScoper scoper(mDBGetKeywordForURI);
  nsresult rv = BindStatementURI(mDBGetKeywordForURI, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetKeywordForURI->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || ! hasMore) {
    aKeyword.SetIsVoid(PR_TRUE);
    return NS_OK; 
  }

  
  return mDBGetKeywordForURI->GetString(0, aKeyword);
}

NS_IMETHODIMP
nsNavBookmarks::GetKeywordForBookmark(PRInt64 aBookmarkId, nsAString& aKeyword)
{
  aKeyword.Truncate(0);

  mozStorageStatementScoper scoper(mDBGetKeywordForBookmark);
  nsresult rv = mDBGetKeywordForBookmark->BindInt64Parameter(0, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetKeywordForBookmark->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || ! hasMore) {
    aKeyword.SetIsVoid(PR_TRUE);
    return NS_OK; 
  }

  
  return mDBGetKeywordForBookmark->GetString(0, aKeyword);
}

NS_IMETHODIMP
nsNavBookmarks::GetURIForKeyword(const nsAString& aKeyword, nsIURI** aURI)
{
  *aURI = nsnull;
  if (aKeyword.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  
  nsAutoString kwd(aKeyword);
  ToLowerCase(kwd);

  mozStorageStatementScoper scoper(mDBGetURIForKeyword);
  nsresult rv = mDBGetURIForKeyword->BindStringParameter(0, kwd);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetURIForKeyword->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || ! hasMore)
    return NS_OK; 

  
  nsCAutoString spec;
  rv = mDBGetURIForKeyword->GetUTF8String(0, spec);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_NewURI(aURI, spec);
}

NS_IMETHODIMP
nsNavBookmarks::BeginUpdateBatch()
{
  if (mBatchLevel++ == 0) {
    mozIStorageConnection* conn = DBConn();
    PRBool transactionInProgress = PR_TRUE; 
    conn->GetTransactionInProgress(&transactionInProgress);
    mBatchHasTransaction = ! transactionInProgress;
    if (mBatchHasTransaction)
      conn->BeginTransaction();

    ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                        OnBeginUpdateBatch())
  }
  mozIStorageConnection *dbConn = DBConn();
  mozStorageTransaction transaction(dbConn, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::EndUpdateBatch()
{
  if (--mBatchLevel == 0) {
    if (mBatchHasTransaction)
      DBConn()->CommitTransaction();
    mBatchHasTransaction = PR_FALSE;
    ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                        OnEndUpdateBatch())
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::AddObserver(nsINavBookmarkObserver *aObserver,
                            PRBool aOwnsWeak)
{
  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}

NS_IMETHODIMP
nsNavBookmarks::RemoveObserver(nsINavBookmarkObserver *aObserver)
{
  return mObservers.RemoveWeakElement(aObserver);
}



NS_IMETHODIMP
nsNavBookmarks::OnBeginUpdateBatch()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnEndUpdateBatch()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnVisit(nsIURI *aURI, PRInt64 aVisitID, PRTime aTime,
                        PRInt64 aSessionID, PRInt64 aReferringID,
                        PRUint32 aTransitionType)
{
  
  PRBool bookmarked = PR_FALSE;
  IsBookmarked(aURI, &bookmarked);
  if (bookmarked) {
    
    nsTArray<PRInt64> bookmarks;

    nsresult rv = GetBookmarkIdsForURITArray(aURI, &bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i++)
        ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                            OnItemVisited(bookmarks[i], aURI, aVisitID, aTime))
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnDeleteURI(nsIURI *aURI)
{
  
  PRBool bookmarked = PR_FALSE;
  IsBookmarked(aURI, &bookmarked);
  if (bookmarked) {
    
    nsTArray<PRInt64> bookmarks;

    nsresult rv = GetBookmarkIdsForURITArray(aURI, &bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
        ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                            OnItemChanged(bookmarks[i], aURI, NS_LITERAL_CSTRING("cleartime"),
                                          EmptyString()))
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnClearHistory()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle,
                               const nsAString& aUserTitle,
                               PRBool aIsUserTitleChanged)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnPageChanged(nsIURI *aURI, PRUint32 aWhat,
                              const nsAString &aValue)
{
  PRBool bookmarked = PR_FALSE;
  IsBookmarked(aURI, &bookmarked);
  if (bookmarked) {
    if (aWhat == nsINavHistoryObserver::ATTRIBUTE_FAVICON) {
      
      nsresult rv;
      nsTArray<PRInt64> bookmarks;

      rv = GetBookmarkIdsForURITArray(aURI, &bookmarks);
      NS_ENSURE_SUCCESS(rv, rv);

      if (bookmarks.Length()) {
        for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
          ENUMERATE_WEAKARRAY(mObservers, nsINavBookmarkObserver,
                              OnItemChanged(bookmarks[i], aURI, NS_LITERAL_CSTRING("favicon"),
                                            aValue));
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnPageExpired(nsIURI* aURI, PRTime aVisitTime,
                              PRBool aWholeEntry)
{
  
  return NS_OK;
}
