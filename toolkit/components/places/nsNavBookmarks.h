








































#ifndef nsNavBookmarks_h_
#define nsNavBookmarks_h_

#include "nsINavBookmarksService.h"
#include "nsIAnnotationService.h"
#include "nsITransaction.h"
#include "nsNavHistory.h"
#include "nsToolkitCompsCID.h"
#include "nsCategoryCache.h"

namespace mozilla {
namespace places {

  enum BookmarkStatementId {
    DB_FIND_REDIRECTED_BOOKMARK = 0
  , DB_GET_BOOKMARKS_FOR_URI
  };

  struct BookmarkData {
    PRInt64 id;
    nsCString url;
    nsCString title;
    PRInt32 position;
    PRInt64 placeId;
    PRInt64 parentId;
    PRInt64 grandParentId;
    PRInt32 type;
    nsCString serviceCID;
    PRTime dateAdded;
    PRTime lastModified;
    nsCString guid;
    nsCString parentGuid;
  };

  struct ItemVisitData {
    BookmarkData bookmark;
    PRInt64 visitId;
    PRUint32 transitionType;
    PRTime time;
  };

  struct ItemChangeData {
    BookmarkData bookmark;
    nsCString property;
    PRBool isAnnotation;
    nsCString newValue;
  };

  typedef void (nsNavBookmarks::*ItemVisitMethod)(const ItemVisitData&);
  typedef void (nsNavBookmarks::*ItemChangeMethod)(const ItemChangeData&);

} 
} 

class nsIOutputStream;

class nsNavBookmarks : public nsINavBookmarksService,
                       public nsINavHistoryObserver,
                       public nsIAnnotationObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVBOOKMARKSSERVICE
  NS_DECL_NSINAVHISTORYOBSERVER
  NS_DECL_NSIANNOTATIONOBSERVER

  nsNavBookmarks();

  


  static nsNavBookmarks* GetSingleton();

  


  nsresult Init();

  
  static nsresult InitTables(mozIStorageConnection* aDBConn);

  static nsNavBookmarks* GetBookmarksService() {
    if (!gBookmarksService) {
      nsCOMPtr<nsINavBookmarksService> serv =
        do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nsnull);
      NS_ASSERTION(gBookmarksService,
                   "Should have static instance pointer now");
    }
    return gBookmarksService;
  }

  nsresult ResultNodeForContainer(PRInt64 aID,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aNode);

  
  
  
  nsresult QueryFolderChildren(PRInt64 aFolderId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsCOMArray<nsNavHistoryResultNode>* children);

  














  nsresult ProcessFolderNodeRow(mozIStorageValueArray* aRow,
                                nsNavHistoryQueryOptions* aOptions,
                                nsCOMArray<nsNavHistoryResultNode>* aChildren,
                                PRInt32& aCurrentIndex);

  








  nsresult QueryFolderChildrenAsync(nsNavHistoryFolderResultNode* aNode,
                                    PRInt64 aFolderId,
                                    mozIStoragePendingStatement** _pendingStmt);

  
  
  
  nsresult CreateContainerWithID(PRInt64 aId, PRInt64 aParent,
                                 const nsACString& aTitle,
                                 const nsAString& aContractId,
                                 PRBool aIsBookmarkFolder,
                                 PRInt32* aIndex,
                                 PRInt64* aNewFolder);

  






  PRBool IsRealBookmark(PRInt64 aPlaceId);

  







  nsresult FetchItemInfo(PRInt64 aItemId,
                         mozilla::places::BookmarkData& _bookmark);

  


  nsresult FinalizeStatements();

  mozIStorageStatement* GetStatementById(
    enum mozilla::places::BookmarkStatementId aStatementId
  )
  {
    using namespace mozilla::places;
    switch(aStatementId) {
      case DB_FIND_REDIRECTED_BOOKMARK:
        return GetStatement(mDBFindRedirectedBookmark);
      case DB_GET_BOOKMARKS_FOR_URI:
        return GetStatement(mDBFindURIBookmarks);
    }
    return nsnull;
  }

  







  void NotifyItemVisited(const mozilla::places::ItemVisitData& aData);

  







  void NotifyItemChanged(const mozilla::places::ItemChangeData& aData);

private:
  static nsNavBookmarks* gBookmarksService;

  ~nsNavBookmarks();

  









  nsresult InitRoots(bool aForceCreate);

  














  nsresult CreateRoot(const nsCString& name,
                      PRInt64* _itemId,
                      PRInt64 aParentId,
                      nsIStringBundle* aBundle,
                      const PRUnichar* aTitleStringId);

  nsresult AdjustIndices(PRInt64 aFolder,
                         PRInt32 aStartIndex,
                         PRInt32 aEndIndex,
                         PRInt32 aDelta);

  













  nsresult FetchFolderInfo(PRInt64 aFolderId,
                           PRInt32* _folderCount,
                           nsACString& _guid,
                           PRInt64* _parentId);

  nsresult GetFolderType(PRInt64 aFolder, nsACString& aType);

  nsresult GetLastChildId(PRInt64 aFolder, PRInt64* aItemId);

  


  nsCOMPtr<mozIStorageConnection> mDBConn;
  



  nsCOMPtr<mozIStorageConnection> mDBReadOnlyConn;

  nsString mGUIDBase;
  nsresult GetGUIDBase(nsAString& aGUIDBase);

  PRInt32 mItemCount;

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;

  PRInt64 mRoot;
  PRInt64 mMenuRoot;
  PRInt64 mTagsRoot;
  PRInt64 mUnfiledRoot;
  PRInt64 mToolbarRoot;

  nsresult IsBookmarkedInDatabase(PRInt64 aBookmarkID, PRBool* aIsBookmarked);

  nsresult SetItemDateInternal(mozIStorageStatement* aStatement,
                               PRInt64 aItemId,
                               PRTime aValue);

  
  nsresult GetDescendantChildren(PRInt64 aFolderId,
                                 const nsACString& aFolderGuid,
                                 PRInt64 aGrandParentId,
                                 nsTArray<mozilla::places::BookmarkData>& aFolderChildrenArray);

  enum ItemType {
    BOOKMARK = TYPE_BOOKMARK,
    FOLDER = TYPE_FOLDER,
    SEPARATOR = TYPE_SEPARATOR,
    DYNAMIC_CONTAINER = TYPE_DYNAMIC_CONTAINER
  };

  





























  nsresult InsertBookmarkInDB(PRInt64 aPlaceId,
                              enum ItemType aItemType,
                              PRInt64 aParentId,
                              PRInt32 aIndex,
                              const nsACString& aTitle,
                              PRTime aDateAdded,
                              PRTime aLastModified,
                              const nsAString& aServiceContractId,
                              PRInt64* _itemId,
                              nsACString& _guid);

  











  nsresult GetBookmarkIdsForURITArray(nsIURI* aURI,
                                      nsTArray<PRInt64>& aResult,
                                      bool aSkipTags);

  nsresult GetBookmarksForURI(nsIURI* aURI,
                              nsTArray<mozilla::places::BookmarkData>& _bookmarks);

  PRInt64 RecursiveFindRedirectedBookmark(PRInt64 aPlaceId);

  


  mozIStorageStatement* GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt);

  nsCOMPtr<mozIStorageStatement> mDBGetChildren;
  
  static const PRInt32 kGetChildrenIndex_Position;
  static const PRInt32 kGetChildrenIndex_Type;
  static const PRInt32 kGetChildrenIndex_PlaceID;
  static const PRInt32 kGetChildrenIndex_FolderTitle;
  static const PRInt32 kGetChildrenIndex_ServiceContractId;
  static const PRInt32 kGetChildrenIndex_Guid;

  nsCOMPtr<mozIStorageStatement> mDBFindURIBookmarks;
  static const PRInt32 kFindURIBookmarksIndex_Id;
  static const PRInt32 kFindURIBookmarksIndex_Guid;
  static const PRInt32 kFindURIBookmarksIndex_ParentId;
  static const PRInt32 kFindURIBookmarksIndex_LastModified;
  static const PRInt32 kFindURIBookmarksIndex_ParentGuid;
  static const PRInt32 kFindURIBookmarksIndex_GrandParentId;

  nsCOMPtr<mozIStorageStatement> mDBGetItemProperties;
  static const PRInt32 kGetItemPropertiesIndex_Id;
  static const PRInt32 kGetItemPropertiesIndex_Url;
  static const PRInt32 kGetItemPropertiesIndex_Title;
  static const PRInt32 kGetItemPropertiesIndex_Position;
  static const PRInt32 kGetItemPropertiesIndex_PlaceId;
  static const PRInt32 kGetItemPropertiesIndex_ParentId;
  static const PRInt32 kGetItemPropertiesIndex_Type;
  static const PRInt32 kGetItemPropertiesIndex_ServiceContractId;
  static const PRInt32 kGetItemPropertiesIndex_DateAdded;
  static const PRInt32 kGetItemPropertiesIndex_LastModified;
  static const PRInt32 kGetItemPropertiesIndex_Guid;
  static const PRInt32 kGetItemPropertiesIndex_ParentGuid;
  static const PRInt32 kGetItemPropertiesIndex_GrandParentId;

  nsCOMPtr<mozIStorageStatement> mDBInsertBookmark;
  static const PRInt32 kInsertBookmarkIndex_Id;
  static const PRInt32 kInsertBookmarkIndex_PlaceId;
  static const PRInt32 kInsertBookmarkIndex_Type;
  static const PRInt32 kInsertBookmarkIndex_Parent;
  static const PRInt32 kInsertBookmarkIndex_Position;
  static const PRInt32 kInsertBookmarkIndex_Title;
  static const PRInt32 kInsertBookmarkIndex_ServiceContractId;
  static const PRInt32 kInsertBookmarkIndex_DateAdded;
  static const PRInt32 kInsertBookmarkIndex_LastModified;

  nsCOMPtr<mozIStorageStatement> mDBFolderInfo;
  nsCOMPtr<mozIStorageStatement> mDBGetItemIndex;
  nsCOMPtr<mozIStorageStatement> mDBGetChildAt;
  nsCOMPtr<mozIStorageStatement> mDBGetItemIdForGUID;
  nsCOMPtr<mozIStorageStatement> mDBIsBookmarkedInDatabase;
  nsCOMPtr<mozIStorageStatement> mDBIsURIBookmarkedInDatabase;
  nsCOMPtr<mozIStorageStatement> mDBIsRealBookmark;
  nsCOMPtr<mozIStorageStatement> mDBGetLastBookmarkID;
  nsCOMPtr<mozIStorageStatement> mDBSetItemDateAdded;
  nsCOMPtr<mozIStorageStatement> mDBSetItemLastModified;
  nsCOMPtr<mozIStorageStatement> mDBSetItemIndex;
  nsCOMPtr<mozIStorageStatement> mDBGetKeywordForURI;
  nsCOMPtr<mozIStorageStatement> mDBGetBookmarksToKeywords;
  nsCOMPtr<mozIStorageStatement> mDBAdjustPosition;
  nsCOMPtr<mozIStorageStatement> mDBRemoveItem;
  nsCOMPtr<mozIStorageStatement> mDBGetLastChildId;
  nsCOMPtr<mozIStorageStatement> mDBMoveItem;
  nsCOMPtr<mozIStorageStatement> mDBSetItemTitle;
  nsCOMPtr<mozIStorageStatement> mDBChangeBookmarkURI;
  nsCOMPtr<mozIStorageStatement> mDBFindRedirectedBookmark;

  class RemoveFolderTransaction : public nsITransaction {
  public:
    RemoveFolderTransaction(PRInt64 aID) : mID(aID) {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      mozilla::places::BookmarkData folder;
      nsresult rv = bookmarks->FetchItemInfo(mID, folder);
      
      mParent = folder.parentId;
      mIndex = folder.position;

      rv = bookmarks->GetItemTitle(mID, mTitle);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString type;
      rv = bookmarks->GetFolderType(mID, type);
      NS_ENSURE_SUCCESS(rv, rv);
      CopyUTF8toUTF16(type, mType);

      return bookmarks->RemoveItem(mID);
    }

    NS_IMETHOD UndoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      PRInt64 newFolder;
      return bookmarks->CreateContainerWithID(mID, mParent, mTitle, mType, PR_TRUE,
                                              &mIndex, &newFolder); 
    }

    NS_IMETHOD RedoTransaction() {
      return DoTransaction();
    }

    NS_IMETHOD GetIsTransient(PRBool* aResult) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
    
    NS_IMETHOD Merge(nsITransaction* aTransaction, PRBool* aResult) {
      *aResult = PR_FALSE;
      return NS_OK;
    }

  private:
    PRInt64 mID;
    PRInt64 mParent;
    nsCString mTitle;
    nsString mType;
    PRInt32 mIndex;
  };

  
  bool mCanNotify;
  nsCategoryCache<nsINavBookmarkObserver> mCacheObservers;

  bool mShuttingDown;

  
  
  bool mBatching;

  



  nsresult EnsureKeywordsHash();
  nsDataHashtable<nsTrimInt64HashKey, nsString> mBookmarkToKeywordHash;

  





  nsresult UpdateKeywordsHashForRemovedBookmark(PRInt64 aItemId);
};

#endif 
