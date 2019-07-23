





































#ifndef nsNavBookmarks_h_
#define nsNavBookmarks_h_

#include "nsINavBookmarksService.h"
#include "nsIAnnotationService.h"
#include "nsITransaction.h"
#include "nsNavHistory.h"
#include "nsNavHistoryResult.h" 
#include "nsToolkitCompsCID.h"
#include "nsCategoryCache.h"

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
  nsresult Init();

  
  static nsresult InitTables(mozIStorageConnection* aDBConn);

  static nsNavBookmarks* GetBookmarksService() {
    if (!sInstance) {
      nsresult rv;
      nsCOMPtr<nsINavBookmarksService> serv(do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);
      NS_ASSERTION(sInstance, "Should have static instance pointer now");
    }
    return sInstance;
  }

  nsresult AddBookmarkToHash(PRInt64 aBookmarkId, PRTime aMinTime);

  nsresult ResultNodeForContainer(PRInt64 aID, nsNavHistoryQueryOptions *aOptions,
                                  nsNavHistoryResultNode **aNode);

  
  
  
  nsresult QueryFolderChildren(PRInt64 aFolderId,
                               nsNavHistoryQueryOptions *aOptions,
                               nsCOMArray<nsNavHistoryResultNode> *children);

  
  
  
  nsresult CreateContainerWithID(PRInt64 aId, PRInt64 aParent,
                                 const nsACString& aName,
                                 const nsAString& aContractId,
                                 PRBool aIsBookmarkFolder,
                                 PRInt32* aIndex,
                                 PRInt64* aNewFolder);

  






  PRBool IsRealBookmark(PRInt64 aPlaceId);

  
  nsresult OnQuit();

  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();

  PRBool ItemExists(PRInt64 aItemId);

  


  nsresult FinalizeStatements();

private:
  static nsNavBookmarks *sInstance;

  ~nsNavBookmarks();

  nsresult InitRoots();
  nsresult InitDefaults();
  nsresult InitStatements();
  nsresult CreateRoot(mozIStorageStatement* aGetRootStatement,
                      const nsCString& name, PRInt64* aID,
                      PRInt64 aParentID, PRBool* aWasCreated);

  nsresult AdjustIndices(PRInt64 aFolder,
                         PRInt32 aStartIndex, PRInt32 aEndIndex,
                         PRInt32 aDelta);

  








  nsresult FolderCount(PRInt64 aFolderId, PRInt32 *aFolderCount);

  nsresult GetFolderType(PRInt64 aFolder, nsACString &aType);

  nsresult GetLastChildId(PRInt64 aFolder, PRInt64* aItemId);

  
  nsNavHistory* History() { return nsNavHistory::GetHistoryService(); }

  nsCOMPtr<mozIStorageConnection> mDBConn;

  nsString mGUIDBase;
  nsresult GetGUIDBase(nsAString& aGUIDBase);

  PRInt32 mItemCount;

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;
  PRInt64 mRoot;
  PRInt64 mBookmarksRoot;
  PRInt64 mTagRoot;
  PRInt64 mUnfiledRoot;

  
  PRInt64 mToolbarFolder;

  
  PRInt32 mBatchLevel;

  
  
  PRBool mBatchHasTransaction;

  
  
  nsDataHashtable<nsTrimInt64HashKey, PRInt64> mBookmarksHash;
  nsDataHashtable<nsTrimInt64HashKey, PRInt64>* GetBookmarksHash();
  nsresult FillBookmarksHash();
  nsresult RecursiveAddBookmarkHash(PRInt64 aBookmarkId, PRInt64 aCurrentSource,
                                    PRTime aMinTime);
  nsresult UpdateBookmarkHashOnRemove(PRInt64 aPlaceId);

  nsresult GetParentAndIndexOfFolder(PRInt64 aFolder, PRInt64* aParent, 
                                     PRInt32* aIndex);

  nsresult IsBookmarkedInDatabase(PRInt64 aBookmarkID, PRBool* aIsBookmarked);

  nsresult SetItemDateInternal(mozIStorageStatement* aStatement, PRInt64 aItemId, PRTime aValue);

  
  struct folderChildrenInfo
  {
    PRInt64 itemId;
    PRUint16 itemType;
    PRInt64 placeId;
    PRInt64 parentId;
    PRInt64 grandParentId;
    PRInt32 index;
    nsCString url;
    nsCString folderType;
  };

  
  nsresult GetDescendantChildren(PRInt64 aFolderId,
                                 PRInt64 aGrandParentId,
                                 nsTArray<folderChildrenInfo>& aFolderChildrenArray);

  enum ItemType {
    BOOKMARK = TYPE_BOOKMARK,
    FOLDER = TYPE_FOLDER,
    SEPARATOR = TYPE_SEPARATOR,
    DYNAMIC_CONTAINER = TYPE_DYNAMIC_CONTAINER
  };

  





























  nsresult InsertBookmarkInDB(PRInt64 aItemId,
                              PRInt64 aPlaceId,
                              enum ItemType aItemType,
                              PRInt64 aParentId,
                              PRInt32 aIndex,
                              const nsACString &aTitle,
                              PRTime aDateAdded,
                              PRTime aLastModified,
                              const nsAString &aServiceContractId,
                              PRInt64 *_retval);

  
  nsCOMPtr<mozIStorageStatement> mDBGetChildren;
  static const PRInt32 kGetChildrenIndex_Position;
  static const PRInt32 kGetChildrenIndex_Type;
  static const PRInt32 kGetChildrenIndex_PlaceID;
  static const PRInt32 kGetChildrenIndex_FolderTitle;
  static const PRInt32 kGetChildrenIndex_ServiceContractId;

  nsCOMPtr<mozIStorageStatement> mDBFindURIBookmarks;  
  static const PRInt32 kFindBookmarksIndex_ID;
  static const PRInt32 kFindBookmarksIndex_Type;
  static const PRInt32 kFindBookmarksIndex_PlaceID;
  static const PRInt32 kFindBookmarksIndex_Parent;
  static const PRInt32 kFindBookmarksIndex_Position;
  static const PRInt32 kFindBookmarksIndex_Title;

  nsCOMPtr<mozIStorageStatement> mDBFolderCount;

  nsCOMPtr<mozIStorageStatement> mDBGetItemIndex;
  nsCOMPtr<mozIStorageStatement> mDBGetChildAt;

  nsCOMPtr<mozIStorageStatement> mDBGetItemProperties; 
  static const PRInt32 kGetItemPropertiesIndex_ID;
  static const PRInt32 kGetItemPropertiesIndex_URI; 
  static const PRInt32 kGetItemPropertiesIndex_Title;
  static const PRInt32 kGetItemPropertiesIndex_Position;
  static const PRInt32 kGetItemPropertiesIndex_PlaceID;
  static const PRInt32 kGetItemPropertiesIndex_Parent;
  static const PRInt32 kGetItemPropertiesIndex_Type;
  static const PRInt32 kGetItemPropertiesIndex_ServiceContractId;
  static const PRInt32 kGetItemPropertiesIndex_DateAdded;
  static const PRInt32 kGetItemPropertiesIndex_LastModified;

  nsCOMPtr<mozIStorageStatement> mDBGetItemIdForGUID;
  nsCOMPtr<mozIStorageStatement> mDBGetRedirectDestinations;

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

  nsCOMPtr<mozIStorageStatement> mDBIsBookmarkedInDatabase;
  nsCOMPtr<mozIStorageStatement> mDBIsRealBookmark;
  nsCOMPtr<mozIStorageStatement> mDBGetLastBookmarkID;
  nsCOMPtr<mozIStorageStatement> mDBSetItemDateAdded;
  nsCOMPtr<mozIStorageStatement> mDBSetItemLastModified;
  nsCOMPtr<mozIStorageStatement> mDBSetItemIndex;

  
  nsCOMPtr<mozIStorageStatement> mDBGetKeywordForURI;
  nsCOMPtr<mozIStorageStatement> mDBGetKeywordForBookmark;
  nsCOMPtr<mozIStorageStatement> mDBGetURIForKeyword;

  class RemoveFolderTransaction : public nsITransaction {
  public:
    RemoveFolderTransaction(PRInt64 aID) : mID(aID) {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();

      nsresult rv = bookmarks->GetParentAndIndexOfFolder(mID, &mParent, &mIndex);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = bookmarks->GetItemTitle(mID, mTitle);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString type;
      rv = bookmarks->GetFolderType(mID, type);
      NS_ENSURE_SUCCESS(rv, rv);
      CopyUTF8toUTF16(type, mType);

      return bookmarks->RemoveFolder(mID);
    }

    NS_IMETHOD UndoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
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
};

struct nsBookmarksUpdateBatcher
{
  nsBookmarksUpdateBatcher() { nsNavBookmarks::GetBookmarksService()->BeginUpdateBatch(); }
  ~nsBookmarksUpdateBatcher() { nsNavBookmarks::GetBookmarksService()->EndUpdateBatch(); }
};


#endif 
