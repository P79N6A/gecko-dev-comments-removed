








































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

const PRInt32 nsNavBookmarks::kFindBookmarksIndex_ID = 0;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Type = 1;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_PlaceID = 2;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Parent = 3;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Position = 4;
const PRInt32 nsNavBookmarks::kFindBookmarksIndex_Title = 5;


const PRInt32 nsNavBookmarks::kGetChildrenIndex_Position = 13;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_Type = 14;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_PlaceID = 15;
const PRInt32 nsNavBookmarks::kGetChildrenIndex_ServiceContractId = 16;

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

const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_Id = 0;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_PlaceId = 1;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_Type = 2;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_Parent = 3;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_Position = 4;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_Title = 5;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_ServiceContractId = 6;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_DateAdded = 7;
const PRInt32 nsNavBookmarks::kInsertBookmarkIndex_LastModified = 8;

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsNavBookmarks, gBookmarksService)

#define BOOKMARKS_ANNO_PREFIX "bookmarks/"
#define BOOKMARKS_TOOLBAR_FOLDER_ANNO NS_LITERAL_CSTRING(BOOKMARKS_ANNO_PREFIX "toolbarFolder")
#define GUID_ANNO NS_LITERAL_CSTRING("placesInternal/GUID")
#define READ_ONLY_ANNO NS_LITERAL_CSTRING("placesInternal/READ_ONLY")

nsNavBookmarks::nsNavBookmarks() : mItemCount(0)
                                 , mRoot(0)
                                 , mBookmarksRoot(0)
                                 , mTagRoot(0)
                                 , mToolbarFolder(0)
                                 , mBatchLevel(0)
                                 , mBatchHasTransaction(PR_FALSE)
                                 , mCanNotify(false)
                                 , mCacheObservers("bookmark-observers")
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
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  mDBConn = history->GetStorageConnection();
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsresult rv = InitStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitRoots();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
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
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_BOOKMARKS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACETYPE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PARENTPOSITION);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED);
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
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_KEYWORDS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_KEYWORD_VALIDITY_TRIGGER);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}





nsresult
nsNavBookmarks::InitStatements()
{
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.id "
      "FROM moz_bookmarks b "
      "WHERE b.type = ?2 AND b.fk = ( "
        "SELECT id FROM moz_places_temp "
        "WHERE url = ?1 "
        "UNION "
        "SELECT id FROM moz_places "
        "WHERE url = ?1 "
        "LIMIT 1 "
      ") "
      "ORDER BY b.lastModified DESC, b.id DESC "),
    getter_AddRefs(mDBFindURIBookmarks));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  

  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, COALESCE(b.title, h.title), "
        "h.rev_host, h.visit_count, h.last_visit_date, f.url, null, b.id, "
        "b.dateAdded, b.lastModified, b.parent, null, b.position, b.type, "
        "b.fk, b.folder_type "
      "FROM moz_bookmarks b "
      "JOIN moz_places_temp h ON b.fk = h.id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.parent = ?1 "
      "UNION ALL "
      "SELECT h.id, h.url, COALESCE(b.title, h.title), "
        "h.rev_host, h.visit_count, h.last_visit_date, f.url, null, b.id, "
        "b.dateAdded, b.lastModified, b.parent, null, b.position, b.type, "
        "b.fk, b.folder_type "
      "FROM moz_bookmarks b "
      "LEFT JOIN moz_places h ON b.fk = h.id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.parent = ?1 "
        "AND (b.fk ISNULL OR b.fk NOT IN (select id FROM moz_places_temp)) "
      "ORDER BY position ASC"),
    getter_AddRefs(mDBGetChildren));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT COUNT(*), "
      "(SELECT id FROM moz_bookmarks WHERE id = ?1) "
      "FROM moz_bookmarks WHERE parent = ?1"),
    getter_AddRefs(mDBFolderCount));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT position FROM moz_bookmarks WHERE id = ?1"),
    getter_AddRefs(mDBGetItemIndex));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id, fk, type FROM moz_bookmarks WHERE parent = ?1 AND position = ?2"),
    getter_AddRefs(mDBGetChildAt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.id, "
             "IFNULL((SELECT url FROM moz_places_temp WHERE id = b.fk), "
                    "(SELECT url FROM moz_places WHERE id = b.fk)), "
             "b.title, b.position, b.fk, b.parent, b.type, b.folder_type, "
             "b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBGetItemProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT item_id FROM moz_items_annos "
      "WHERE content = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBGetItemIdForGUID));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT DISTINCT dest_v.place_id "
      "FROM moz_historyvisits_temp source_v "
      "JOIN moz_historyvisits_temp dest_v ON dest_v.from_visit = source_v.id "
      "WHERE source_v.place_id = ?1 "
        "AND source_v.visit_date >= ?2 "
        "AND dest_v.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") "
      "UNION "
      "SELECT DISTINCT dest_v.place_id "
      "FROM moz_historyvisits_temp source_v "
      "JOIN moz_historyvisits dest_v ON dest_v.from_visit = source_v.id "
      "WHERE source_v.place_id = ?1 "
        "AND source_v.visit_date >= ?2 "
        "AND dest_v.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") "
      "UNION "
      "SELECT DISTINCT dest_v.place_id "
      "FROM moz_historyvisits source_v "
      "JOIN moz_historyvisits_temp dest_v ON dest_v.from_visit = source_v.id "
      "WHERE source_v.place_id = ?1 "
        "AND source_v.visit_date >= ?2 "
        "AND dest_v.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") "
      "UNION "      
      "SELECT DISTINCT dest_v.place_id "
      "FROM moz_historyvisits source_v "
      "JOIN moz_historyvisits dest_v ON dest_v.from_visit = source_v.id "
      "WHERE source_v.place_id = ?1 "
        "AND source_v.visit_date >= ?2 "
        "AND dest_v.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") "),
    getter_AddRefs(mDBGetRedirectDestinations));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_bookmarks "
        "(id, fk, type, parent, position, title, folder_type, dateAdded, lastModified) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)"),
    getter_AddRefs(mDBInsertBookmark));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT position FROM moz_bookmarks WHERE fk = ?1 AND type = ?2"),
    getter_AddRefs(mDBIsBookmarkedInDatabase));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id "
      "FROM moz_bookmarks "
      "WHERE fk = ?1 "
        "AND type = ?2 "
        "AND parent NOT IN ("
          "SELECT a.item_id "
          "FROM moz_items_annos a "
          "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
          "WHERE n.name = ?3"
        ") "
      "LIMIT 1"),
    getter_AddRefs(mDBIsRealBookmark));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id "
      "FROM moz_bookmarks "
      "ORDER BY ROWID DESC "
      "LIMIT 1"),
    getter_AddRefs(mDBGetLastBookmarkID));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET dateAdded = ?1, lastModified = ?1 "
      "WHERE id = ?2"),
    getter_AddRefs(mDBSetItemDateAdded));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET lastModified = ?1 WHERE id = ?2"),
    getter_AddRefs(mDBSetItemLastModified));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET position = ?2 WHERE id = ?1"),
    getter_AddRefs(mDBSetItemIndex));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT k.keyword FROM moz_bookmarks b "
      "JOIN moz_keywords k ON k.id = b.keyword_id "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBGetKeywordForBookmark));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT k.keyword "
      "FROM ( "
        "SELECT id FROM moz_places_temp "
        "WHERE url = ?1 "
        "UNION ALL "
        "SELECT id FROM moz_places "
        "WHERE url = ?1 "
        "LIMIT 1 "
      ") AS h "
      "JOIN moz_bookmarks b ON b.fk = h.id "
      "JOIN moz_keywords k ON k.id = b.keyword_id"),
    getter_AddRefs(mDBGetKeywordForURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT url FROM moz_keywords k "
      "JOIN moz_bookmarks b ON b.keyword_id = k.id "
      "JOIN moz_places_temp h ON b.fk = h.id "
      "WHERE k.keyword = ?1 "
      "UNION ALL "
      "SELECT url FROM moz_keywords k "
      "JOIN moz_bookmarks b ON b.keyword_id = k.id "
      "JOIN moz_places h ON b.fk = h.id "
      "WHERE k.keyword = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBGetURIForKeyword));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavBookmarks::FinalizeStatements() {
  mozIStorageStatement* stmts[] = {
    mDBGetChildren,
    mDBFindURIBookmarks,
    mDBFolderCount,
    mDBGetItemIndex,
    mDBGetChildAt,
    mDBGetItemProperties,
    mDBGetItemIdForGUID,
    mDBGetRedirectDestinations,
    mDBInsertBookmark,
    mDBIsBookmarkedInDatabase,
    mDBIsRealBookmark,
    mDBGetLastBookmarkID,
    mDBSetItemDateAdded,
    mDBSetItemLastModified,
    mDBSetItemIndex,
    mDBGetKeywordForURI,
    mDBGetKeywordForBookmark,
    mDBGetURIForKeyword
  };

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(stmts); i++) {
    nsresult rv = nsNavHistory::FinalizeStatement(stmts[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






















nsresult
nsNavBookmarks::InitRoots()
{
  nsCOMPtr<mozIStorageStatement> getRootStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT folder_id FROM moz_bookmarks_roots WHERE root_name = ?1"),
    getter_AddRefs(getRootStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool createdPlacesRoot = PR_FALSE;
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("places"), &mRoot, 0, &createdPlacesRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("menu"), &mBookmarksRoot, mRoot, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool createdToolbarFolder;
  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("toolbar"), &mToolbarFolder, mRoot, &createdToolbarFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!createdPlacesRoot && createdToolbarFolder) {
    nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
    NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);

    nsTArray<PRInt64> folders;
    annosvc->GetItemsWithAnnotationTArray(BOOKMARKS_TOOLBAR_FOLDER_ANNO,
                                          &folders);
    if (folders.Length() > 0) {
      nsCOMPtr<mozIStorageStatement> moveItems;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "UPDATE moz_bookmarks SET parent = ?1 WHERE parent=?2"),
        getter_AddRefs(moveItems));
      rv = moveItems->BindInt64Parameter(0, mToolbarFolder);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = moveItems->BindInt64Parameter(1, folders[0]);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = moveItems->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
      rv = RemoveFolder(folders[0]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("tags"), &mTagRoot, mRoot, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  getRootStatement->Reset();
  rv = CreateRoot(getRootStatement, NS_LITERAL_CSTRING("unfiled"), &mUnfiledRoot, mRoot, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRUint16 databaseStatus = nsINavHistoryService::DATABASE_STATUS_OK;
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  rv = history->GetDatabaseStatus(&databaseStatus);
  if (NS_FAILED(rv) ||
      databaseStatus != nsINavHistoryService::DATABASE_STATUS_OK) {
    rv = InitDefaults();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






nsresult
nsNavBookmarks::InitDefaults()
{
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsIStringBundle *bundle = history->GetBundle();
  NS_ENSURE_TRUE(bundle, NS_ERROR_OUT_OF_MEMORY);

  
  nsXPIDLString bookmarksTitle;
  nsresult rv = bundle->GetStringFromName(NS_LITERAL_STRING("BookmarksMenuFolderTitle").get(),
                                          getter_Copies(bookmarksTitle));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemTitle(mBookmarksRoot, NS_ConvertUTF16toUTF8(bookmarksTitle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString toolbarTitle;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BookmarksToolbarFolderTitle").get(),
                                 getter_Copies(toolbarTitle));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemTitle(mToolbarFolder, NS_ConvertUTF16toUTF8(toolbarTitle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString unfiledTitle;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("UnsortedBookmarksFolderTitle").get(),
                                 getter_Copies(unfiledTitle));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemTitle(mUnfiledRoot, NS_ConvertUTF16toUTF8(unfiledTitle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString tagsTitle;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("TagsFolderTitle").get(),
                                 getter_Copies(tagsTitle));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemTitle(mTagRoot, NS_ConvertUTF16toUTF8(tagsTitle));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










nsresult
nsNavBookmarks::CreateRoot(mozIStorageStatement* aGetRootStatement,
                           const nsCString& name, PRInt64* aID,
                           PRInt64 aParentID, PRBool* aWasCreated)
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
  rv = CreateFolder(aParentID, EmptyCString(), nsINavBookmarksService::DEFAULT_INDEX, aID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_bookmarks_roots (root_name, folder_id) VALUES (?1, ?2)"),
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






nsDataHashtable<nsTrimInt64HashKey, PRInt64>*
nsNavBookmarks::GetBookmarksHash()
{
  if (!mBookmarksHash.IsInitialized()) {
    nsresult rv = FillBookmarksHash();
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "FillBookmarksHash() failed!");
  }

  return &mBookmarksHash;
}







nsresult
nsNavBookmarks::FillBookmarksHash()
{
  PRBool hasMore;

  
  NS_ENSURE_TRUE(mBookmarksHash.Init(1024), NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.fk "
      "FROM moz_bookmarks b "
      "WHERE b.type = ?1 "
      "AND b.fk NOTNULL"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt32Parameter(0, TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 pageID;
    rv = statement->GetInt64(0, &pageID);
    NS_ENSURE_TRUE(mBookmarksHash.Put(pageID, pageID), NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  
  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT v1.place_id, v2.place_id "
        "FROM moz_bookmarks b "
        "LEFT JOIN moz_historyvisits_temp v1 on b.fk = v1.place_id "
        "LEFT JOIN moz_historyvisits v2 on v2.from_visit = v1.id "
        "WHERE b.fk IS NOT NULL AND b.type = ?1 "
        "AND v2.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") GROUP BY v2.place_id "
      "UNION "
      "SELECT v1.place_id, v2.place_id "
        "FROM moz_bookmarks b "
        "LEFT JOIN moz_historyvisits v1 on b.fk = v1.place_id "
        "LEFT JOIN moz_historyvisits_temp v2 on v2.from_visit = v1.id "
        "WHERE b.fk IS NOT NULL AND b.type = ?1 "
        "AND v2.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") GROUP BY v2.place_id "
      "UNION "
      "SELECT v1.place_id, v2.place_id "
        "FROM moz_bookmarks b "
        "LEFT JOIN moz_historyvisits v1 on b.fk = v1.place_id "
        "LEFT JOIN moz_historyvisits v2 on v2.from_visit = v1.id "
        "WHERE b.fk IS NOT NULL AND b.type = ?1 "
        "AND v2.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") GROUP BY v2.place_id "
      "UNION "
        "SELECT v1.place_id, v2.place_id "
        "FROM moz_bookmarks b "
        "LEFT JOIN moz_historyvisits_temp v1 on b.fk = v1.place_id "
        "LEFT JOIN moz_historyvisits_temp v2 on v2.from_visit = v1.id "
        "WHERE b.fk IS NOT NULL AND b.type = ?1 "
        "AND v2.visit_type IN (") +
        nsPrintfCString("%d,%d",
                        nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                        nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(") GROUP BY v2.place_id "),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(0, TYPE_BOOKMARK);
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
nsNavBookmarks::AddBookmarkToHash(PRInt64 aPlaceId, PRTime aMinTime)
{
  if (!GetBookmarksHash()->Put(aPlaceId, aPlaceId))
    return NS_ERROR_OUT_OF_MEMORY;
  return RecursiveAddBookmarkHash(aPlaceId, aPlaceId, aMinTime);
}














nsresult
nsNavBookmarks::RecursiveAddBookmarkHash(PRInt64 aPlaceID,
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
      if (GetBookmarksHash()->Get(curID, &alreadyExistingOne))
        continue;

      if (!GetBookmarksHash()->Put(curID, aPlaceID))
        return NS_ERROR_OUT_OF_MEMORY;

      
      found.AppendElement(curID);
    }
  }

  
  for (PRUint32 i = 0; i < found.Length(); i ++) {
    rv = RecursiveAddBookmarkHash(aPlaceID, found[i], aMinTime);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}












static PLDHashOperator
RemoveBookmarkHashCallback(nsTrimInt64HashKey::KeyType aKey,
                           PRInt64& aPlaceId, void* aUserArg)
{
  const PRInt64* removeThisOne = reinterpret_cast<const PRInt64*>(aUserArg);
  if (aPlaceId == *removeThisOne)
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

  
  GetBookmarksHash()->Enumerate(RemoveBookmarkHashCallback,
                                reinterpret_cast<void*>(&aPlaceId));
  return NS_OK;
}


PRBool
nsNavBookmarks::IsRealBookmark(PRInt64 aPlaceId)
{
  
  
  PRInt64 bookmarkId;
  PRBool isBookmark = GetBookmarksHash()->Get(aPlaceId, &bookmarkId);
  if (!isBookmark)
    return PR_FALSE;

  {
    mozStorageStatementScoper scope(mDBIsRealBookmark);

    (void)mDBIsRealBookmark->BindInt64Parameter(0, aPlaceId);
    (void)mDBIsRealBookmark->BindInt32Parameter(1, TYPE_BOOKMARK);
    (void)mDBIsRealBookmark->BindUTF8StringParameter(
      2, NS_LITERAL_CSTRING(LMANNO_FEEDURI)
    );

    
    
    if (NS_SUCCEEDED(mDBIsRealBookmark->ExecuteStep(&isBookmark)))
      return isBookmark;
  }

  return PR_FALSE;
}






nsresult
nsNavBookmarks::IsBookmarkedInDatabase(PRInt64 aPlaceId,
                                       PRBool *aIsBookmarked)
{
  mozStorageStatementScoper scope(mDBIsBookmarkedInDatabase);
  nsresult rv = mDBIsBookmarkedInDatabase->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBIsBookmarkedInDatabase->BindInt32Parameter(1, TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);

  return mDBIsBookmarkedInDatabase->ExecuteStep(aIsBookmarked);
}


nsresult
nsNavBookmarks::AdjustIndices(PRInt64 aFolder,
                              PRInt32 aStartIndex, PRInt32 aEndIndex,
                              PRInt32 aDelta)
{
  NS_ASSERTION(aStartIndex <= aEndIndex, "start index must be <= end index");

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

  nsresult rv = mDBConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);
 
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetPlacesRoot(PRInt64 *aRoot)
{
  *aRoot = mRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarksMenuFolder(PRInt64 *aRoot)
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
nsNavBookmarks::GetTagsFolder(PRInt64 *aRoot)
{
  *aRoot = mTagRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetUnfiledBookmarksFolder(PRInt64 *aRoot)
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
                                   const nsACString &aTitle,
                                   PRTime aDateAdded,
                                   PRTime aLastModified,
                                   const nsAString &aServiceContractId,
                                   PRInt64 *_newItemId)
{
  NS_ASSERTION(_newItemId, "Null pointer passed to InsertBookmarkInDB!");

  mozStorageStatementScoper scope(mDBInsertBookmark);

  nsresult rv;
  if (aItemId && aItemId != -1)
    rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_Id, aItemId);
  else
    rv = mDBInsertBookmark->BindNullParameter(kInsertBookmarkIndex_Id);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aPlaceId && aPlaceId != -1)
    rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_PlaceId, aPlaceId);
  else
    rv = mDBInsertBookmark->BindNullParameter(kInsertBookmarkIndex_PlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBInsertBookmark->BindInt32Parameter(kInsertBookmarkIndex_Type, aItemType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_Parent, aParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBInsertBookmark->BindInt32Parameter(kInsertBookmarkIndex_Position, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTitle.IsVoid())
    rv = mDBInsertBookmark->BindNullParameter(kInsertBookmarkIndex_Title);
  else
    rv = mDBInsertBookmark->BindUTF8StringParameter(kInsertBookmarkIndex_Title, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aServiceContractId.IsEmpty())
    rv = mDBInsertBookmark->BindNullParameter(kInsertBookmarkIndex_ServiceContractId);
  else
    rv = mDBInsertBookmark->BindStringParameter(kInsertBookmarkIndex_ServiceContractId, aServiceContractId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_DateAdded, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aLastModified)
    rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_LastModified, aLastModified);
  else
    rv = mDBInsertBookmark->BindInt64Parameter(kInsertBookmarkIndex_LastModified, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBInsertBookmark->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aItemId || aItemId == -1) {
    
    mozStorageStatementScoper scoper(mDBGetLastBookmarkID);
    PRBool hasResult;
    rv = mDBGetLastBookmarkID->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);
    *_newItemId = mDBGetLastBookmarkID->AsInt64(0);
  }
  else
    *_newItemId = aItemId;

  
  
  
  rv = SetItemDateInternal(mDBSetItemLastModified, aParentId, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::InsertBookmark(PRInt64 aFolder,
                               nsIURI *aURI,
                               PRInt32 aIndex,
                               const nsACString& aTitle,
                               PRInt64 *aNewBookmarkId)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aNewBookmarkId);

  
  if (aIndex < nsINavBookmarksService::DEFAULT_INDEX)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsNavHistory *history = nsNavHistory::GetHistoryService();
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

  nsString voidString;
  voidString.SetIsVoid(PR_TRUE);
  rv = InsertBookmarkInDB(-1, childID, BOOKMARK, aFolder, index,
                          aTitle, PR_Now(), nsnull, EmptyString(),
                          aNewBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  

  nsCAutoString url;
  rv = aURI->GetSpec(url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool isBookmark = !IsQueryURI(url);

  if (isBookmark) {
    
    
    
    PRBool parentIsLivemark;
    nsCOMPtr<nsILivemarkService> lms = 
      do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lms->IsLivemark(aFolder, &parentIsLivemark);
    NS_ENSURE_SUCCESS(rv, rv);
 
    isBookmark = !parentIsLivemark;
  }
  
  
  
  
  rv = history->UpdateFrecency(childID, isBookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  AddBookmarkToHash(childID, 0);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemAdded(*aNewBookmarkId, aFolder, index, TYPE_BOOKMARK));

  
  
  
  PRInt64 grandParentId;
  rv = GetFolderIdForItem(aFolder, &grandParentId);
  NS_ENSURE_SUCCESS(rv, rv);
  if (grandParentId == mTagRoot) {
    
    nsTArray<PRInt64> bookmarks;

    rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
        ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                            OnItemChanged(bookmarks[i],
                                          NS_LITERAL_CSTRING("tags"),
                                          PR_FALSE,
                                          EmptyCString(),
                                          0,
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
  PRUint16 itemType;
  nsCAutoString buffer;
  nsCAutoString spec;

  { 
    mozStorageStatementScoper scope(mDBGetItemProperties);
    mDBGetItemProperties->BindInt64Parameter(0, aItemId);

    PRBool results;
    rv = mDBGetItemProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!results)
      return NS_ERROR_INVALID_ARG; 

    childIndex = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Position);
    placeId = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_PlaceID);
    folderId = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent);
    itemType = (PRUint16) mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
    if (itemType == TYPE_BOOKMARK) {
      rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_URI, spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (itemType == TYPE_FOLDER) {
    rv = RemoveFolder(aItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnBeforeItemRemoved(aItemId, itemType));

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  rv = annosvc->RemoveItemAnnotations(aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  buffer.AssignLiteral("DELETE FROM moz_bookmarks WHERE id = ");
  buffer.AppendInt(aItemId);

  rv = mDBConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  if (childIndex != -1) {
    rv = AdjustIndices(folderId, childIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = SetItemDateInternal(mDBSetItemLastModified, folderId, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateBookmarkHashOnRemove(placeId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (itemType == TYPE_BOOKMARK) {
    
    
    
    nsNavHistory *history = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
    rv = history->UpdateFrecency(placeId, IsRealBookmark(placeId));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemRemoved(aItemId, folderId, childIndex, itemType));

  if (itemType == TYPE_BOOKMARK) {
    
    
    
    PRInt64 grandParentId;
    rv = GetFolderIdForItem(folderId, &grandParentId);
    NS_ENSURE_SUCCESS(rv, rv);
    if (grandParentId == mTagRoot) {
      nsCOMPtr<nsIURI> uri;
      rv = NS_NewURI(getter_AddRefs(uri), spec);
      NS_ENSURE_SUCCESS(rv, rv);
      nsTArray<PRInt64> bookmarks;

      rv = GetBookmarkIdsForURITArray(uri, bookmarks);
      NS_ENSURE_SUCCESS(rv, rv);

      if (bookmarks.Length()) {
        for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
          ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                              OnItemChanged(bookmarks[i],
                                            NS_LITERAL_CSTRING("tags"),
                                            PR_FALSE,
                                            EmptyCString(),
                                            0,
                                            TYPE_BOOKMARK));
        }
      }
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsNavBookmarks::CreateFolder(PRInt64 aParent, const nsACString &aName,
                             PRInt32 aIndex, PRInt64 *aNewFolder)
{
  
  NS_ENSURE_ARG_POINTER(aNewFolder);

  
  
  
  
  PRInt32 localIndex = aIndex;
  nsString voidString;
  voidString.SetIsVoid(PR_TRUE);
  return CreateContainerWithID(-1, aParent, aName, voidString, PR_TRUE,
                               &localIndex, aNewFolder);
}

NS_IMETHODIMP
nsNavBookmarks::CreateDynamicContainer(PRInt64 aParent, const nsACString &aName,
                                       const nsAString &aContractId,
                                       PRInt32 aIndex,
                                       PRInt64 *aNewFolder)
{
  if (aContractId.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  return CreateContainerWithID(-1, aParent, aName, aContractId, PR_FALSE,
                               &aIndex, aNewFolder);
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderReadonly(PRInt64 aFolder, PRBool *aResult)
{
  NS_ENSURE_ARG_MIN(aFolder, 1);
  NS_ENSURE_ARG_POINTER(aResult);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  return annosvc->ItemHasAnnotation(aFolder, READ_ONLY_ANNO, aResult);
}

NS_IMETHODIMP
nsNavBookmarks::SetFolderReadonly(PRInt64 aFolder, PRBool aReadOnly)
{
  NS_ENSURE_ARG_MIN(aFolder, 1);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  if (aReadOnly) {
    return annosvc->SetItemAnnotationInt32(aFolder, READ_ONLY_ANNO, 1,
                                           0,
                                           nsAnnotationService::EXPIRE_NEVER);
  }
  else {
    PRBool hasAnno;
    nsresult rv = annosvc->ItemHasAnnotation(aFolder, READ_ONLY_ANNO, &hasAnno);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasAnno)
      return annosvc->RemoveItemAnnotation(aFolder, READ_ONLY_ANNO);
  }
  return NS_OK;
}

nsresult
nsNavBookmarks::CreateContainerWithID(PRInt64 aItemId, PRInt64 aParent,
                                      const nsACString& aName,
                                      const nsAString& aContractId,
                                      PRBool aIsBookmarkFolder,
                                      PRInt32* aIndex, PRInt64* aNewFolder)
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

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemAdded(*aNewFolder, aParent, index, containerType));

  *aIndex = index;
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::InsertSeparator(PRInt64 aParent, PRInt32 aIndex,
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

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemAdded(*aNewItemId, aParent, index, TYPE_SEPARATOR));

  return NS_OK;
}

nsresult
nsNavBookmarks::GetLastChildId(PRInt64 aFolder, PRInt64* aItemId)
{
  *aItemId = -1;

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_bookmarks WHERE parent = ?1 "
      "ORDER BY position DESC LIMIT 1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool found;
  rv = statement->ExecuteStep(&found);
  NS_ENSURE_SUCCESS(rv, rv);
  if (found)
    *aItemId = statement->AsInt64(0);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetIdForItemAt(PRInt64 aFolder, PRInt32 aIndex, PRInt64* aItemId)
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
    
    mozStorageStatementScoper scope(mDBGetChildAt);

    rv = mDBGetChildAt->BindInt64Parameter(0, aFolder);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetChildAt->BindInt32Parameter(1, aIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool found;
    rv = mDBGetChildAt->ExecuteStep(&found);
    NS_ENSURE_SUCCESS(rv, rv);
    if (found)
      *aItemId = mDBGetChildAt->AsInt64(0);
  }
  return NS_OK;
}

nsresult 
nsNavBookmarks::GetParentAndIndexOfFolder(PRInt64 aFolder, PRInt64* aParent, 
                                          PRInt32* aIndex)
{
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT parent, position FROM moz_bookmarks WHERE id = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool found;
  rv = statement->ExecuteStep(&found);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(found, NS_ERROR_INVALID_ARG); 

  *aParent = statement->AsInt64(0);
  *aIndex = statement->AsInt32(1);

  return NS_OK;
}

NS_HIDDEN_(nsresult)
nsNavBookmarks::RemoveFolder(PRInt64 aFolderId)
{
  NS_ENSURE_TRUE(aFolderId != mRoot, NS_ERROR_INVALID_ARG);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnBeforeItemRemoved(aFolderId, TYPE_FOLDER));

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsresult rv;
  PRInt64 parent;
  PRInt32 index, type;
  nsCAutoString folderType;
  {
    mozStorageStatementScoper scope(mDBGetItemProperties);
    rv = mDBGetItemProperties->BindInt64Parameter(0, aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    rv = mDBGetItemProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!results) {
      return NS_ERROR_INVALID_ARG; 
    }

    type = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
    parent = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent);
    index = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Position);
    rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId, folderType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (type != TYPE_FOLDER) {
    NS_WARNING("RemoveFolder(): aFolderId is not a folder!");
    return NS_ERROR_INVALID_ARG; 
  }

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  rv = annosvc->RemoveItemAnnotations(aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (folderType.Length() > 0) {
    
    nsCOMPtr<nsIDynamicContainer> bmcServ = do_GetService(folderType.get());
    if (bmcServ) {
      rv = bmcServ->OnContainerRemoving(aFolderId);
      if (NS_FAILED(rv))
        NS_WARNING("Remove folder container notification failed.");
    }
  }

  
  rv = RemoveFolderChildren(aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString buffer;
  buffer.AssignLiteral("DELETE FROM moz_bookmarks WHERE id = ");
  buffer.AppendInt(aFolderId);
  rv = mDBConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AdjustIndices(parent, index + 1, PR_INT32_MAX, -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemDateInternal(mDBSetItemLastModified, parent, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aFolderId == mToolbarFolder) {
    mToolbarFolder = 0;
  }

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
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
    
    mozStorageStatementScoper scope(mDBGetChildren);
    rv = mDBGetChildren->BindInt64Parameter(0, aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    while (NS_SUCCEEDED(mDBGetChildren->ExecuteStep(&hasMore)) && hasMore) {
      folderChildrenInfo child;
      child.itemId = mDBGetChildren->AsInt64(nsNavHistory::kGetInfoIndex_ItemId);
      child.parentId = aFolderId;
      child.grandParentId = aGrandParentId;
      child.itemType = (PRUint16) mDBGetChildren->AsInt32(kGetChildrenIndex_Type);
      child.placeId = mDBGetChildren->AsInt64(kGetChildrenIndex_PlaceID);
      child.index = mDBGetChildren->AsInt32(kGetChildrenIndex_Position);

      if (child.itemType == TYPE_BOOKMARK) {
        nsCAutoString URIString;
        rv = mDBGetChildren->GetUTF8String(nsNavHistory::kGetInfoIndex_URL,
                                           URIString);
        NS_ENSURE_SUCCESS(rv, rv);
        child.url = URIString;
      }
      else if (child.itemType == TYPE_FOLDER) {
        nsCAutoString folderType;
        rv = mDBGetChildren->GetUTF8String(kGetChildrenIndex_ServiceContractId,
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
  nsresult rv;
  PRUint16 itemType;
  PRInt64 grandParentId;
  {
    mozStorageStatementScoper scope(mDBGetItemProperties);
    rv = mDBGetItemProperties->BindInt64Parameter(0, aFolderId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRBool folderExists;
    if (NS_FAILED(mDBGetItemProperties->ExecuteStep(&folderExists)) || !folderExists)
      return NS_ERROR_INVALID_ARG;

    
    itemType = (PRUint16) mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
    if (itemType != TYPE_FOLDER)
      return NS_ERROR_INVALID_ARG;

    
    
    
    grandParentId = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent);
  }

  
  nsTArray<folderChildrenInfo> folderChildrenArray;
  rv = GetDescendantChildren(aFolderId, grandParentId, folderChildrenArray);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCString foldersToRemove;
  for (PRUint32 i = 0; i < folderChildrenArray.Length(); i++) {
    folderChildrenInfo child = folderChildrenArray[i];

    
    ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
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
      "WHERE parent IN (?1") +
        foldersToRemove +
      NS_LITERAL_CSTRING(")"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteStatement->BindInt64Parameter(0, aFolderId);
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

  
  rv = SetItemDateInternal(mDBSetItemLastModified, aFolderId, PR_Now());
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < folderChildrenArray.Length(); i++) {
    folderChildrenInfo child = folderChildrenArray[i];
    if (child.itemType == TYPE_BOOKMARK) {
      PRInt64 placeId = child.placeId;
      UpdateBookmarkHashOnRemove(placeId);

      
      
      
      nsNavHistory *history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      rv = history->UpdateFrecency(placeId, IsRealBookmark(placeId));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (PRInt32 i = folderChildrenArray.Length() - 1; i >= 0 ; i--) {
    folderChildrenInfo child = folderChildrenArray[i];

    ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                        OnItemRemoved(child.itemId,
                                      child.parentId,
                                      child.index,
                                      child.itemType));

    if (child.itemType == TYPE_BOOKMARK) {
      
      
      

      if (child.grandParentId == mTagRoot) {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), child.url);
        NS_ENSURE_SUCCESS(rv, rv);

        nsTArray<PRInt64> bookmarks;
        rv = GetBookmarkIdsForURITArray(uri, bookmarks);
        NS_ENSURE_SUCCESS(rv, rv);

        if (bookmarks.Length()) {
          for (PRUint32 i = 0; i < bookmarks.Length(); i++) {
            ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                                OnItemChanged(bookmarks[i],
                                              NS_LITERAL_CSTRING("tags"),
                                              PR_FALSE,
                                              EmptyCString(),
                                              0,
                                              TYPE_BOOKMARK));
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

  
  if (aIndex < -1)
    return NS_ERROR_INVALID_ARG;

  
  if (aItemId == aNewParent)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsresult rv;
  PRInt64 oldParent;
  PRInt32 oldIndex;
  PRUint16 itemType;
  nsCAutoString folderType;
  {
    mozStorageStatementScoper scope(mDBGetItemProperties);
    rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    rv = mDBGetItemProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!results) {
      return NS_ERROR_INVALID_ARG; 
    }

    oldParent = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent);
    oldIndex = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Position);
    itemType = (PRUint16) mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
    if (itemType == TYPE_FOLDER) {
      rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId,
                                               folderType);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (oldParent == aNewParent && oldIndex == aIndex)
    return NS_OK;

  
  if (itemType == TYPE_FOLDER) {
    PRInt64 p = aNewParent;

    while (p) {
      mozStorageStatementScoper scope(mDBGetItemProperties);
      if (p == aItemId) {
        return NS_ERROR_INVALID_ARG;
      }

      rv = mDBGetItemProperties->BindInt64Parameter(0, p);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool results;
      rv = mDBGetItemProperties->ExecuteStep(&results);
      NS_ENSURE_SUCCESS(rv, rv);
      p = results ? mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent) : 0;
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
    } else {
      rv = AdjustIndices(oldParent, oldIndex + 1, newIndex, -1);
    }
  } else {
    
    
    rv = AdjustIndices(oldParent, oldIndex + 1, PR_INT32_MAX, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = AdjustIndices(aNewParent, newIndex, PR_INT32_MAX, 1);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString buffer;
  buffer.AssignLiteral("UPDATE moz_bookmarks SET ");
  if (aNewParent != oldParent) {
    buffer.AppendLiteral(" parent = ");
    buffer.AppendInt(aNewParent);
  }
  if (newIndex != oldIndex) {
    if (aNewParent != oldParent)
      buffer.AppendLiteral(", ");
    buffer.AppendLiteral(" position = ");
    buffer.AppendInt(newIndex);
  }
  buffer.AppendLiteral(" WHERE id = ");
  buffer.AppendInt(aItemId);
  rv = mDBConn->ExecuteSimpleSQL(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime now = PR_Now();
  rv = SetItemDateInternal(mDBSetItemLastModified, oldParent, now);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemDateInternal(mDBSetItemLastModified, aNewParent, now);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
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
nsNavBookmarks::SetItemDateInternal(mozIStorageStatement* aStatement, PRInt64 aItemId, PRTime aValue)
{
  mozStorageStatementScoper scope(aStatement);
  nsresult rv = aStatement->BindInt64Parameter(0, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStatement->BindInt64Parameter(1, aItemId);
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

  rv = SetItemDateInternal(mDBSetItemDateAdded, aItemId, aDateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemChanged(aItemId, NS_LITERAL_CSTRING("dateAdded"),
                                    PR_FALSE,
                                    nsPrintfCString(16, "%lld", aDateAdded),
                                    aDateAdded,
                                    itemType));
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemDateAdded(PRInt64 aItemId, PRTime *aDateAdded)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aDateAdded);

  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  return mDBGetItemProperties->GetInt64(kGetItemPropertiesIndex_DateAdded, aDateAdded);
}

NS_IMETHODIMP
nsNavBookmarks::SetItemLastModified(PRInt64 aItemId, PRTime aLastModified)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemDateInternal(mDBSetItemLastModified, aItemId, aLastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemChanged(aItemId,
                                    NS_LITERAL_CSTRING("lastModified"),
                                    PR_FALSE,
                                    nsPrintfCString(16, "%lld", aLastModified),
                                    aLastModified,
                                    itemType));
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemLastModified(PRInt64 aItemId, PRTime *aLastModified)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aLastModified);

  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  return mDBGetItemProperties->GetInt64(kGetItemPropertiesIndex_LastModified, aLastModified);
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
nsNavBookmarks::GetItemGUID(PRInt64 aItemId, nsAString &aGUID)
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

  return SetItemGUID(aItemId, aGUID);
}

NS_IMETHODIMP
nsNavBookmarks::SetItemGUID(PRInt64 aItemId, const nsAString &aGUID)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  PRInt64 checkId;
  GetItemIdForGUID(aGUID, &checkId);
  if (checkId != -1)
    return NS_ERROR_INVALID_ARG; 

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  return annosvc->SetItemAnnotationString(aItemId, GUID_ANNO, aGUID, 0,
                                          nsIAnnotationService::EXPIRE_NEVER);
}

NS_IMETHODIMP
nsNavBookmarks::GetItemIdForGUID(const nsAString &aGUID, PRInt64 *aItemId)
{
  NS_ENSURE_ARG_POINTER(aItemId);

  mozStorageStatementScoper scoper(mDBGetItemIdForGUID);
  nsresult rv = mDBGetItemIdForGUID->BindStringParameter(0, aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetItemIdForGUID->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || ! hasMore) {
    *aItemId = -1;
    return NS_OK; 
  }

  
  return mDBGetItemIdForGUID->GetInt64(0, aItemId);
}

NS_IMETHODIMP
nsNavBookmarks::SetItemTitle(PRInt64 aItemId, const nsACString &aTitle)
{
  
  PRUint16 itemType;
  nsresult rv = GetItemType(aItemId, &itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET title = ?1, lastModified = ?2 WHERE id = ?3"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aTitle.IsVoid())
    rv = statement->BindNullParameter(0);
  else
    rv = statement->BindUTF8StringParameter(0, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = statement->BindInt64Parameter(1, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(2, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemChanged(aItemId,
                                    NS_LITERAL_CSTRING("title"),
                                    PR_FALSE,
                                    aTitle,
                                    lastModified,
                                    itemType));
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemTitle(PRInt64 aItemId, nsACString &aTitle)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);

  mozStorageStatementScoper scope(mDBGetItemProperties);

  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  return mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_Title, aTitle);
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkURI(PRInt64 aItemId, nsIURI **aURI)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aURI);

  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  PRInt32 type = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
  if (type != TYPE_BOOKMARK)
    return NS_ERROR_INVALID_ARG; 

  nsCAutoString spec;
  rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_URI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aURI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemType(PRInt64 aItemId, PRUint16 *aType)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aType);

  mozStorageStatementScoper scope(mDBGetItemProperties);

  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results) {
    return NS_ERROR_INVALID_ARG; 
  }

  *aType = (PRUint16)mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
  return NS_OK;
}

nsresult
nsNavBookmarks::GetFolderType(PRInt64 aFolder, nsACString &aType)
{
  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results) {
    return NS_ERROR_INVALID_ARG;
  }

  return mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId, aType);
}

nsresult
nsNavBookmarks::ResultNodeForContainer(PRInt64 aID,
                                       nsNavHistoryQueryOptions *aOptions,
                                       nsNavHistoryResultNode **aNode)
{
  mozStorageStatementScoper scope(mDBGetItemProperties);
  mDBGetItemProperties->BindInt64Parameter(0, aID);

  PRBool results;
  nsresult rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(results, "ResultNodeForContainer expects a valid item id");

  
  nsCAutoString title;
  rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_Title, title);

  PRUint16 itemType = (PRUint16) mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
  if (itemType == TYPE_DYNAMIC_CONTAINER) {
    
    nsCAutoString contractId;
    rv = mDBGetItemProperties->GetUTF8String(kGetItemPropertiesIndex_ServiceContractId,
                                             contractId);
    NS_ENSURE_SUCCESS(rv, rv);
    *aNode = new nsNavHistoryContainerResultNode(EmptyCString(), title, EmptyCString(),
                                                 nsINavHistoryResultNode::RESULT_TYPE_DYNAMIC_CONTAINER,
                                                 PR_TRUE,
                                                 contractId,
                                                 aOptions);
    (*aNode)->mItemId = aID;
  } else { 
    *aNode = new nsNavHistoryFolderResultNode(title, aOptions, aID, EmptyCString());
  }
  if (!*aNode)
    return NS_ERROR_OUT_OF_MEMORY;

  (*aNode)->mDateAdded =
    mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_DateAdded);
  (*aNode)->mLastModified =
    mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_LastModified);

  NS_ADDREF(*aNode);
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

  PRBool results;

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index = -1;
  while (NS_SUCCEEDED(mDBGetChildren->ExecuteStep(&results)) && results) {

    
    
    
    
    index ++;

    PRUint16 itemType = (PRUint16) mDBGetChildren->AsInt32(kGetChildrenIndex_Type);
    PRInt64 id = mDBGetChildren->AsInt64(nsNavHistory::kGetInfoIndex_ItemId);
    nsRefPtr<nsNavHistoryResultNode> node;
    if (itemType == TYPE_BOOKMARK) {
      nsNavHistory *history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
      rv = history->RowToResult(mDBGetChildren, options, getter_AddRefs(node));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 nodeType;
      node->GetType(&nodeType);
      if ((nodeType == nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
           aOptions->ExcludeQueries()) ||
          (nodeType != nsINavHistoryResultNode::RESULT_TYPE_QUERY &&
           nodeType != nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT &&
           aOptions->ExcludeItems())) {
        continue;
      }
    } else if (itemType == TYPE_FOLDER || itemType == TYPE_DYNAMIC_CONTAINER) {
      if (options->ExcludeReadOnlyFolders()) {
        
        PRBool readOnly = PR_FALSE;
        GetFolderReadonly(id, &readOnly);
        if (readOnly)
          continue; 
      }

      rv = ResultNodeForContainer(id, aOptions, getter_AddRefs(node));
      if (NS_FAILED(rv))
        continue;
    } else {
      
      if (aOptions->ExcludeItems()) {
        continue;
      }
      node = new nsNavHistorySeparatorResultNode();
      NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);

      
      
      node->mItemId =
        mDBGetChildren->AsInt64(nsNavHistory::kGetInfoIndex_ItemId);

      
      node->mDateAdded =
        mDBGetChildren->AsInt64(nsNavHistory::kGetInfoIndex_ItemDateAdded);
      node->mLastModified =
        mDBGetChildren->AsInt64(nsNavHistory::kGetInfoIndex_ItemLastModified);
    }

    
    
    node->mBookmarkIndex = index;

    NS_ENSURE_TRUE(aChildren->AppendObject(node), NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}

nsresult
nsNavBookmarks::FolderCount(PRInt64 aFolderId, PRInt32 *aFolderCount)
{
  *aFolderCount = 0;
  mozStorageStatementScoper scope(mDBFolderCount);

  nsresult rv = mDBFolderCount->BindInt64Parameter(0, aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBFolderCount->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(mDBFolderCount->AsInt64(1) == aFolderId, NS_ERROR_INVALID_ARG);

  *aFolderCount = mDBFolderCount->AsInt32(0);

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::IsBookmarked(nsIURI *aURI, PRBool *aBookmarked)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aBookmarked);

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 urlID;
  nsresult rv = history->GetUrlIdFor(aURI, &urlID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! urlID) {
    
    *aBookmarked = PR_FALSE;
    return NS_OK;
  }

  PRInt64 bookmarkedID;
  PRBool foundOne = GetBookmarksHash()->Get(urlID, &bookmarkedID);

  
  
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
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = nsnull;

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt64 urlID;
  nsresult rv = history->GetUrlIdFor(aURI, &urlID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! urlID) {
    
    return NS_OK;
  }

  PRInt64 bookmarkID;
  if (GetBookmarksHash()->Get(urlID, &bookmarkID)) {
    
    mozIStorageStatement* statement = history->DBGetIdPageInfo();
    NS_ENSURE_STATE(statement);
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
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
  NS_ENSURE_ARG(aNewURI);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsNavHistory *history = nsNavHistory::GetHistoryService();
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

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET fk = ?1, lastModified = ?2 WHERE id = ?3"),
    getter_AddRefs(statement));
  rv = statement->BindInt64Parameter(0, placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = statement->BindInt64Parameter(1, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(2, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = AddBookmarkToHash(placeId, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = UpdateBookmarkHashOnRemove(oldPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = history->UpdateFrecency(placeId, PR_TRUE );
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  

  rv = history->UpdateFrecency(oldPlaceId, IsRealBookmark(oldPlaceId));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = aNewURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
    OnItemChanged(aBookmarkId, NS_LITERAL_CSTRING("uri"), PR_FALSE, spec,
                  lastModified, TYPE_BOOKMARK));

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetFolderIdForItem(PRInt64 aItemId, PRInt64 *aFolderId)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aFolderId);

  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results)
    return NS_ERROR_INVALID_ARG; 

  rv = mDBGetItemProperties->GetInt64(kGetItemPropertiesIndex_Parent, aFolderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(aItemId != *aFolderId, NS_ERROR_UNEXPECTED);
  return NS_OK;
}

nsresult
nsNavBookmarks::GetBookmarkIdsForURITArray(nsIURI *aURI,
                                           nsTArray<PRInt64> &aResult)
{
  NS_PRECONDITION(aURI, "Should not be null");
  NS_ENSURE_ARG(aURI);

  mozStorageStatementScoper scope(mDBFindURIBookmarks);

  nsresult rv = BindStatementURI(mDBFindURIBookmarks, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFindURIBookmarks->BindInt32Parameter(1, TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more;
  while (NS_SUCCEEDED((rv = mDBFindURIBookmarks->ExecuteStep(&more))) && more) {
    if (!aResult.AppendElement(
        mDBFindURIBookmarks->AsInt64(kFindBookmarksIndex_ID)))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetBookmarkIdsForURI(nsIURI *aURI, PRUint32 *aCount,
                                   PRInt64 **aBookmarks)
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
    *aBookmarks = static_cast<PRInt64*>
                             (nsMemory::Alloc(sizeof(PRInt64) * bookmarks.Length()));
    if (! *aBookmarks)
      return NS_ERROR_OUT_OF_MEMORY;
    for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
      (*aBookmarks)[i] = bookmarks[i];
  }
  *aCount = bookmarks.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetItemIndex(PRInt64 aItemId, PRInt32 *aIndex)
{
  NS_ENSURE_ARG_MIN(aItemId, 1);
  NS_ENSURE_ARG_POINTER(aIndex);

  mozStorageStatementScoper scope(mDBGetItemIndex);
  mDBGetItemIndex->BindInt64Parameter(0, aItemId);
  PRBool results;
  nsresult rv = mDBGetItemIndex->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!results) {
    *aIndex = -1;
    return NS_OK;
  }

  *aIndex = mDBGetItemIndex->AsInt32(0);
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
  PRUint16 itemType;

  {
    mozStorageStatementScoper scopeGet(mDBGetItemProperties);
    rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool results;
    rv = mDBGetItemProperties->ExecuteStep(&results);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!results)
      return NS_OK;

    oldIndex = mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Position);
    itemType = (PRUint16)mDBGetItemProperties->AsInt32(kGetItemPropertiesIndex_Type);
    parent = mDBGetItemProperties->AsInt64(kGetItemPropertiesIndex_Parent);
  }

  
  PRInt32 folderCount;
  rv = FolderCount(parent, &folderCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aNewIndex < folderCount, NS_ERROR_INVALID_ARG);

  mozStorageStatementScoper scoper(mDBSetItemIndex);
  rv = mDBSetItemIndex->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBSetItemIndex->BindInt32Parameter(1, aNewIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBSetItemIndex->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemMoved(aItemId, parent, oldIndex, parent,
                                  aNewIndex, itemType));

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::SetKeywordForBookmark(PRInt64 aBookmarkId, const nsAString& aKeyword)
{
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);

  
  nsAutoString kwd(aKeyword);
  ToLowerCase(kwd);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  nsresult rv;
  PRBool results;
  PRInt64 keywordId = 0;

  if (!kwd.IsEmpty()) {
    
    nsCOMPtr<mozIStorageStatement> getKeywordStmnt;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
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
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "INSERT INTO moz_keywords (keyword) VALUES (?1)"),
        getter_AddRefs(addKeywordStmnt));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = addKeywordStmnt->BindStringParameter(0, kwd);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = addKeywordStmnt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<mozIStorageStatement> idStmt;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT id "
          "FROM moz_keywords "
          "ORDER BY ROWID DESC "
          "LIMIT 1"),
        getter_AddRefs(idStmt));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasResult;
      rv = idStmt->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");
      keywordId = idStmt->AsInt64(0);
    }
  }

  
  nsCOMPtr<mozIStorageStatement> updateKeywordStmnt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET keyword_id = ?1, lastModified = ?2 "
      "WHERE id = ?3"),
    getter_AddRefs(updateKeywordStmnt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->BindInt64Parameter(0, keywordId);
  NS_ENSURE_SUCCESS(rv, rv);
  PRTime lastModified = PR_Now();
  rv = updateKeywordStmnt->BindInt64Parameter(1, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->BindInt64Parameter(2, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = updateKeywordStmnt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                      OnItemChanged(aBookmarkId, NS_LITERAL_CSTRING("keyword"),
                                    PR_FALSE, NS_ConvertUTF16toUTF8(aKeyword),
                                    lastModified, TYPE_BOOKMARK));

  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::GetKeywordForURI(nsIURI* aURI, nsAString& aKeyword)
{
  NS_ENSURE_ARG(aURI);
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
  NS_ENSURE_ARG_MIN(aBookmarkId, 1);
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
  NS_ENSURE_ARG_POINTER(aURI);
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


nsresult
nsNavBookmarks::BeginUpdateBatch()
{
  if (mBatchLevel++ == 0) {
    mozIStorageConnection* conn = mDBConn;
    PRBool transactionInProgress = PR_TRUE; 
    conn->GetTransactionInProgress(&transactionInProgress);
    mBatchHasTransaction = ! transactionInProgress;
    if (mBatchHasTransaction)
      conn->BeginTransaction();

    ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                        OnBeginUpdateBatch());
  }
  return NS_OK;
}

nsresult
nsNavBookmarks::EndUpdateBatch()
{
  if (--mBatchLevel == 0) {
    if (mBatchHasTransaction)
      mDBConn->CommitTransaction();
    mBatchHasTransaction = PR_FALSE;
    ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                        OnEndUpdateBatch());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                               nsISupports* aUserData) {
  NS_ENSURE_ARG(aCallback);

  BeginUpdateBatch();
  nsresult rv = aCallback->RunBatched(aUserData);
  EndUpdateBatch();

  return rv;
}

NS_IMETHODIMP
nsNavBookmarks::AddObserver(nsINavBookmarkObserver *aObserver,
                            PRBool aOwnsWeak)
{
  NS_ENSURE_ARG(aObserver);
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
                        PRUint32 aTransitionType, PRUint32* aAdded)
{
  
  PRBool bookmarked = PR_FALSE;
  IsBookmarked(aURI, &bookmarked);
  if (bookmarked) {
    
    nsTArray<PRInt64> bookmarks;

    nsresult rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i++)
        ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                            OnItemVisited(bookmarks[i], aVisitID, aTime));
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnBeforeDeleteURI(nsIURI *aURI)
{
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnDeleteURI(nsIURI *aURI)
{
  
  PRBool bookmarked = PR_FALSE;
  IsBookmarked(aURI, &bookmarked);
  if (bookmarked) {
    
    nsTArray<PRInt64> bookmarks;

    nsresult rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
    NS_ENSURE_SUCCESS(rv, rv);

    if (bookmarks.Length()) {
      for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
        ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                            OnItemChanged(bookmarks[i],
                                          NS_LITERAL_CSTRING("cleartime"),
                                          PR_FALSE,
                                          EmptyCString(),
                                          0,
                                          TYPE_BOOKMARK));
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
nsNavBookmarks::OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsNavBookmarks::OnPageChanged(nsIURI *aURI, PRUint32 aWhat,
                              const nsAString &aValue)
{
  nsresult rv;
  if (aWhat == nsINavHistoryObserver::ATTRIBUTE_FAVICON) {
    
    PRBool isPlaceURI;
    rv = aURI->SchemeIs("place", &isPlaceURI);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isPlaceURI) {
      nsCAutoString spec;
      rv = aURI->GetSpec(spec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsNavHistory *history = nsNavHistory::GetHistoryService();
      NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  
      nsCOMArray<nsNavHistoryQuery> queries;
      nsCOMPtr<nsNavHistoryQueryOptions> options;
      rv = history->QueryStringToQueryArray(spec, &queries, getter_AddRefs(options));
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ENSURE_STATE(queries.Count() == 1);
      NS_ENSURE_STATE(queries[0]->Folders().Length() == 1);

      ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                          OnItemChanged(queries[0]->Folders()[0],
                                        NS_LITERAL_CSTRING("favicon"),
                                        PR_FALSE,
                                        NS_ConvertUTF16toUTF8(aValue),
                                        0,
                                        TYPE_BOOKMARK));
    }
    else {
      
      nsTArray<PRInt64> bookmarks;
      rv = GetBookmarkIdsForURITArray(aURI, bookmarks);
      NS_ENSURE_SUCCESS(rv, rv);

      if (bookmarks.Length()) {
        for (PRUint32 i = 0; i < bookmarks.Length(); i ++)
          ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers, nsINavBookmarkObserver,
                              OnItemChanged(bookmarks[i],
                                            NS_LITERAL_CSTRING("favicon"),
                                            PR_FALSE,
                                            NS_ConvertUTF16toUTF8(aValue),
                                            0,
                                            TYPE_BOOKMARK));
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
  rv = SetItemDateInternal(mDBSetItemLastModified, aItemId, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
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
  rv = SetItemDateInternal(mDBSetItemLastModified, aItemId, lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                      nsINavBookmarkObserver,
                      OnItemChanged(aItemId, aName, PR_TRUE, EmptyCString(),
                                    lastModified, itemType));

  return NS_OK;
}

PRBool
nsNavBookmarks::ItemExists(PRInt64 aItemId) {
  mozStorageStatementScoper scope(mDBGetItemProperties);
  nsresult rv = mDBGetItemProperties->BindInt64Parameter(0, aItemId);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool results;
  rv = mDBGetItemProperties->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if (!results)
    return PR_FALSE;

  return PR_TRUE;
}
