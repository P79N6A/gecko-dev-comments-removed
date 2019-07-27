




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
#include "mozilla/Attributes.h"
#include "prtime.h"

class nsNavBookmarks;

namespace mozilla {
namespace places {

  enum BookmarkStatementId {
    DB_FIND_REDIRECTED_BOOKMARK = 0
  , DB_GET_BOOKMARKS_FOR_URI
  };

  struct BookmarkData {
    int64_t id;
    nsCString url;
    nsCString title;
    int32_t position;
    int64_t placeId;
    int64_t parentId;
    int64_t grandParentId;
    int32_t type;
    nsCString serviceCID;
    PRTime dateAdded;
    PRTime lastModified;
    nsCString guid;
    nsCString parentGuid;
  };

  struct ItemVisitData {
    BookmarkData bookmark;
    int64_t visitId;
    uint32_t transitionType;
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

  enum BookmarkDate {
    DATE_ADDED = 0
  , LAST_MODIFIED
  };

} 
} 

class nsNavBookmarks final : public nsINavBookmarksService
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

  


  static already_AddRefed<nsNavBookmarks> GetSingleton();

  


  nsresult Init();

  static nsNavBookmarks* GetBookmarksService() {
    if (!gBookmarksService) {
      nsCOMPtr<nsINavBookmarksService> serv =
        do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nullptr);
      NS_ASSERTION(gBookmarksService,
                   "Should have static instance pointer now");
    }
    return gBookmarksService;
  }

  typedef mozilla::places::BookmarkData BookmarkData;
  typedef mozilla::places::ItemVisitData ItemVisitData;
  typedef mozilla::places::ItemChangeData ItemChangeData;
  typedef mozilla::places::BookmarkStatementId BookmarkStatementId;

  nsresult ResultNodeForContainer(int64_t aID,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aNode);

  
  
  
  nsresult QueryFolderChildren(int64_t aFolderId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsCOMArray<nsNavHistoryResultNode>* children);

  














  nsresult ProcessFolderNodeRow(mozIStorageValueArray* aRow,
                                nsNavHistoryQueryOptions* aOptions,
                                nsCOMArray<nsNavHistoryResultNode>* aChildren,
                                int32_t& aCurrentIndex);

  








  nsresult QueryFolderChildrenAsync(nsNavHistoryFolderResultNode* aNode,
                                    int64_t aFolderId,
                                    mozIStoragePendingStatement** _pendingStmt);

  






  nsresult CreateContainerWithID(int64_t aId, int64_t aParent,
                                 const nsACString& aTitle,
                                 bool aIsBookmarkFolder,
                                 int32_t* aIndex,
                                 const nsACString& aGUID,
                                 int64_t* aNewFolder);

  







  nsresult FetchItemInfo(int64_t aItemId,
                         BookmarkData& _bookmark);

  







  void NotifyItemVisited(const ItemVisitData& aData);

  







  void NotifyItemChanged(const ItemChangeData& aData);

  







  nsresult GetDescendantFolders(int64_t aFolderId,
                                nsTArray<int64_t>& aDescendantFoldersArray);

  static const int32_t kGetChildrenIndex_Guid;
  static const int32_t kGetChildrenIndex_Position;
  static const int32_t kGetChildrenIndex_Type;
  static const int32_t kGetChildrenIndex_PlaceID;

private:
  static nsNavBookmarks* gBookmarksService;

  ~nsNavBookmarks();

  






  bool IsLivemark(int64_t aFolderId);

  



  nsresult ReadRoots();

  nsresult AdjustIndices(int64_t aFolder,
                         int32_t aStartIndex,
                         int32_t aEndIndex,
                         int32_t aDelta);

  













  nsresult FetchFolderInfo(int64_t aFolderId,
                           int32_t* _folderCount,
                           nsACString& _guid,
                           int64_t* _parentId);

  nsresult GetLastChildId(int64_t aFolder, int64_t* aItemId);

  


  nsRefPtr<mozilla::places::Database> mDB;

  int32_t mItemCount;

  nsMaybeWeakPtrArray<nsINavBookmarkObserver> mObservers;

  int64_t mRoot;
  int64_t mMenuRoot;
  int64_t mTagsRoot;
  int64_t mUnfiledRoot;
  int64_t mToolbarRoot;

  inline bool IsRoot(int64_t aFolderId) {
    return aFolderId == mRoot || aFolderId == mMenuRoot ||
           aFolderId == mTagsRoot || aFolderId == mUnfiledRoot ||
           aFolderId == mToolbarRoot;
  }

  nsresult IsBookmarkedInDatabase(int64_t aBookmarkID, bool* aIsBookmarked);

  nsresult SetItemDateInternal(enum mozilla::places::BookmarkDate aDateType,
                               int64_t aItemId,
                               PRTime aValue);

  
  nsresult GetDescendantChildren(int64_t aFolderId,
                                 const nsACString& aFolderGuid,
                                 int64_t aGrandParentId,
                                 nsTArray<BookmarkData>& aFolderChildrenArray);

  enum ItemType {
    BOOKMARK = TYPE_BOOKMARK,
    FOLDER = TYPE_FOLDER,
    SEPARATOR = TYPE_SEPARATOR,
  };

  


























  nsresult InsertBookmarkInDB(int64_t aPlaceId,
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
                              nsACString& _guid);

  











  nsresult GetBookmarkIdsForURITArray(nsIURI* aURI,
                                      nsTArray<int64_t>& aResult,
                                      bool aSkipTags);

  nsresult GetBookmarksForURI(nsIURI* aURI,
                              nsTArray<BookmarkData>& _bookmarks);

  int64_t RecursiveFindRedirectedBookmark(int64_t aPlaceId);

  class RemoveFolderTransaction final : public nsITransaction {
  public:
    explicit RemoveFolderTransaction(int64_t aID) : mID(aID) {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD DoTransaction() override {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      BookmarkData folder;
      nsresult rv = bookmarks->FetchItemInfo(mID, folder);
      
      mParent = folder.parentId;
      mIndex = folder.position;

      rv = bookmarks->GetItemTitle(mID, mTitle);
      NS_ENSURE_SUCCESS(rv, rv);

      return bookmarks->RemoveItem(mID);
    }

    NS_IMETHOD UndoTransaction() override {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
      int64_t newFolder;
      return bookmarks->CreateContainerWithID(mID, mParent, mTitle, true,
                                              &mIndex, EmptyCString(), &newFolder);
    }

    NS_IMETHOD RedoTransaction() override {
      return DoTransaction();
    }

    NS_IMETHOD GetIsTransient(bool* aResult) override {
      *aResult = false;
      return NS_OK;
    }

    NS_IMETHOD Merge(nsITransaction* aTransaction, bool* aResult) override {
      *aResult = false;
      return NS_OK;
    }

  private:
    ~RemoveFolderTransaction() {}

    int64_t mID;
    int64_t mParent;
    nsCString mTitle;
    int32_t mIndex;
  };

  
  bool mCanNotify;
  nsCategoryCache<nsINavBookmarkObserver> mCacheObservers;

  
  
  bool mBatching;

  





  nsresult UpdateKeywordsForRemovedBookmark(const BookmarkData& aBookmark);
};

#endif 
