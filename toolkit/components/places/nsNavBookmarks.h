








































#ifndef nsNavBookmarks_h_
#define nsNavBookmarks_h_

#include "nsINavBookmarksService.h"
#include "nsIAnnotationService.h"
#include "nsITransaction.h"
#include "nsNavHistory.h"
#include "nsToolkitCompsCID.h"
#include "nsCategoryCache.h"
#include "nsTHashtable.h"
#include "nsWeakReference.h"

class nsNavBookmarks;
class nsIOutputStream;

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
    bool isAnnotation;
    nsCString newValue;
  };

  typedef void (nsNavBookmarks::*ItemVisitMethod)(const ItemVisitData&);
  typedef void (nsNavBookmarks::*ItemChangeMethod)(const ItemChangeData&);

  class BookmarkKeyClass : public nsTrimInt64HashKey
  {
    public:
    BookmarkKeyClass(const PRInt64* aItemId)
    : nsTrimInt64HashKey(aItemId)
    , creationTime(PR_Now())
    {
    }
    BookmarkKeyClass(const BookmarkKeyClass& aOther)
    : nsTrimInt64HashKey(aOther)
    , creationTime(PR_Now())
    {
      NS_NOTREACHED("Do not call me!");
    }
    BookmarkData bookmark;
    PRTime creationTime;
  };

  enum BookmarkDate {
    DATE_ADDED = 0
  , LAST_MODIFIED
  };

} 
} 

class nsNavBookmarks : public nsINavBookmarksService
                     , public nsINavHistoryObserver
                     , public nsIAnnotationObserver
                     , public nsIObserver
                     , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVBOOKMARKSSERVICE
  NS_DECL_NSINAVHISTORYOBSERVER
  NS_DECL_NSIANNOTATIONOBSERVER
  NS_DECL_NSIOBSERVER

  nsNavBookmarks();

  


  static nsNavBookmarks* GetSingleton();

  


  nsresult Init();

  static nsNavBookmarks* GetBookmarksServiceIfAvailable() {
    return gBookmarksService;
  }

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

  typedef mozilla::places::BookmarkData BookmarkData;
  typedef mozilla::places::BookmarkKeyClass BookmarkKeyClass;
  typedef mozilla::places::ItemVisitData ItemVisitData;
  typedef mozilla::places::ItemChangeData ItemChangeData;
  typedef mozilla::places::BookmarkStatementId BookmarkStatementId;

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
                                 bool aIsBookmarkFolder,
                                 PRInt32* aIndex,
                                 PRInt64* aNewFolder);

  






  bool IsRealBookmark(PRInt64 aPlaceId);

  







  nsresult FetchItemInfo(PRInt64 aItemId,
                         BookmarkData& _bookmark);

  







  void NotifyItemVisited(const ItemVisitData& aData);

  







  void NotifyItemChanged(const ItemChangeData& aData);

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

  


  nsRefPtr<mozilla::places::Database> mDB;

  nsString mGUIDBase;
  nsresult GetGUIDBase(nsAString& aGUIDBase);

  PRInt32 mItemCount;

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;

  PRInt64 mRoot;
  PRInt64 mMenuRoot;
  PRInt64 mTagsRoot;
  PRInt64 mUnfiledRoot;
  PRInt64 mToolbarRoot;

  nsresult IsBookmarkedInDatabase(PRInt64 aBookmarkID, bool* aIsBookmarked);

  nsresult SetItemDateInternal(enum mozilla::places::BookmarkDate aDateType,
                               PRInt64 aItemId,
                               PRTime aValue);

  
  nsresult GetDescendantChildren(PRInt64 aFolderId,
                                 const nsACString& aFolderGuid,
                                 PRInt64 aGrandParentId,
                                 nsTArray<BookmarkData>& aFolderChildrenArray);

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
                              nsTArray<BookmarkData>& _bookmarks);

  PRInt64 RecursiveFindRedirectedBookmark(PRInt64 aPlaceId);

  static const PRInt32 kGetChildrenIndex_Position;
  static const PRInt32 kGetChildrenIndex_Type;
  static const PRInt32 kGetChildrenIndex_PlaceID;
  static const PRInt32 kGetChildrenIndex_FolderTitle;
  static const PRInt32 kGetChildrenIndex_ServiceContractId;
  static const PRInt32 kGetChildrenIndex_Guid;

  class RemoveFolderTransaction : public nsITransaction {
  public:
    RemoveFolderTransaction(PRInt64 aID) : mID(aID) {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      BookmarkData folder;
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
      return bookmarks->CreateContainerWithID(mID, mParent, mTitle, mType, true,
                                              &mIndex, &newFolder); 
    }

    NS_IMETHOD RedoTransaction() {
      return DoTransaction();
    }

    NS_IMETHOD GetIsTransient(bool* aResult) {
      *aResult = false;
      return NS_OK;
    }
    
    NS_IMETHOD Merge(nsITransaction* aTransaction, bool* aResult) {
      *aResult = false;
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

  
  
  bool mBatching;

  



  nsresult EnsureKeywordsHash();
  nsDataHashtable<nsTrimInt64HashKey, nsString> mBookmarkToKeywordHash;

  





  nsresult UpdateKeywordsHashForRemovedBookmark(PRInt64 aItemId);

  



  nsTHashtable<BookmarkKeyClass> mRecentBookmarksCache;
};

#endif 
