





































#ifndef nsNavBookmarks_h_
#define nsNavBookmarks_h_

#include "nsINavBookmarksService.h"
#include "nsIStringBundle.h"
#include "nsITransaction.h"
#include "nsNavHistory.h"
#include "nsNavHistoryResult.h" 
#include "nsToolkitCompsCID.h"

class nsIOutputStream;

class nsNavBookmarks : public nsINavBookmarksService,
                       public nsINavHistoryObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVBOOKMARKSSERVICE
  NS_DECL_NSINAVHISTORYOBSERVER

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

  nsresult ResultNodeForFolder(PRInt64 aID, nsNavHistoryQueryOptions *aOptions,
                               nsNavHistoryResultNode **aNode);

  
  
  
  nsresult QueryFolderChildren(PRInt64 aFolderId,
                               nsNavHistoryQueryOptions *aOptions,
                               nsCOMArray<nsNavHistoryResultNode> *children);

  
  
  
  
  nsresult CreateFolderWithID(PRInt64 aFolder, PRInt64 aParent, 
                              const nsAString& title,
                              PRBool aSendNotifications,
                              PRInt32 *aIndex, PRInt64* aNewFolder);

  
  
  
  
  nsresult CreateContainerWithID(PRInt64 aFolder, PRInt64 aParent,
                                 const nsAString& title, const nsAString& type,
                                 PRInt32 aIndex, PRInt64* aNewFolder);

  
  nsresult OnQuit();

private:
  static nsNavBookmarks *sInstance;

  ~nsNavBookmarks();

  nsresult InitRoots();
  nsresult InitDefaults();
  nsresult InitToolbarFolder();
  nsresult CreateRoot(mozIStorageStatement* aGetRootStatement,
                      const nsCString& name, PRInt64* aID,
                      PRBool* aWasCreated);

  nsresult AdjustIndices(PRInt64 aFolder,
                         PRInt32 aStartIndex, PRInt32 aEndIndex,
                         PRInt32 aDelta);
  PRInt32 FolderCount(PRInt64 aFolder);
  nsresult GetFolderType(PRInt64 aFolder, nsACString &aType);

  
  nsNavHistory* History() { return nsNavHistory::GetHistoryService(); }

  mozIStorageStatement* DBGetURLPageInfo()
  { return History()->DBGetURLPageInfo(); }

  mozIStorageConnection* DBConn() { return History()->GetStorageConnection(); }

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;
  PRInt64 mRoot;
  PRInt64 mBookmarksRoot;
  PRInt64 mTagRoot;

  
  PRInt64 mToolbarFolder;

  
  PRInt32 mBatchLevel;

  
  
  PRBool mBatchHasTransaction;

  
  
  nsDataHashtable<nsTrimInt64HashKey, PRInt64> mBookmarksHash;
  nsresult FillBookmarksHash();
  nsresult RecursiveAddBookmarkHash(PRInt64 aBookmarkId, PRInt64 aCurrentSource,
                                    PRTime aMinTime);
  nsresult UpdateBookmarkHashOnRemove(PRInt64 aPlaceId);

  nsresult GetParentAndIndexOfFolder(PRInt64 aFolder, PRInt64* aParent, 
                                     PRInt32* aIndex);

  nsresult IsBookmarkedInDatabase(PRInt64 aBookmarkID, PRBool* aIsBookmarked);

  nsCOMPtr<mozIStorageStatement> mDBGetChildren;       
  static const PRInt32 kGetChildrenIndex_Position;
  static const PRInt32 kGetChildrenIndex_Type;
  static const PRInt32 kGetChildrenIndex_ForeignKey;
  static const PRInt32 kGetChildrenIndex_FolderTitle;
  static const PRInt32 kGetChildrenIndex_ID;

  nsCOMPtr<mozIStorageStatement> mDBFindURIBookmarks;  
  static const PRInt32 kFindBookmarksIndex_ID;
  static const PRInt32 kFindBookmarksIndex_Type;
  static const PRInt32 kFindBookmarksIndex_ForeignKey;
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
  static const PRInt32 kGetItemPropertiesIndex_FolderType;

  nsCOMPtr<mozIStorageStatement> mDBGetRedirectDestinations;

  
  nsCOMPtr<mozIStorageStatement> mDBGetKeywordForURI;
  nsCOMPtr<mozIStorageStatement> mDBGetKeywordForBookmark;
  nsCOMPtr<mozIStorageStatement> mDBGetURIForKeyword;

  nsCOMPtr<nsIStringBundle> mBundle;

  class RemoveFolderTransaction : public nsITransaction {
  public:
    RemoveFolderTransaction(PRInt64 aID, PRInt64 aParent, 
                            const nsAString& aTitle, PRInt32 aIndex,
                            const nsACString& aType) 
                            : mID(aID),
                              mParent(aParent),
                              mIndex(aIndex){
      mTitle = aTitle;
      mType = aType;
    }
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      return bookmarks->RemoveFolder(mID);
    }

    NS_IMETHOD UndoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      PRInt64 newFolder;
      
      
      if (mType.IsEmpty())
        return bookmarks->CreateFolderWithID(mID, mParent, mTitle, PR_TRUE, &mIndex, &newFolder);
      nsAutoString type; type.AssignWithConversion(mType);
      return bookmarks->CreateContainerWithID(mID, mParent, mTitle, type, mIndex, &newFolder); 
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
    nsString mTitle;
    nsCString mType;
    PRInt32 mIndex;
  };
};

struct nsBookmarksUpdateBatcher
{
  nsBookmarksUpdateBatcher() { nsNavBookmarks::GetBookmarksService()->BeginUpdateBatch(); }
  ~nsBookmarksUpdateBatcher() { nsNavBookmarks::GetBookmarksService()->EndUpdateBatch(); }
};


#endif 
