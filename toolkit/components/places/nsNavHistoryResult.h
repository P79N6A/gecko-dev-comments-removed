










#ifndef nsNavHistoryResult_h_
#define nsNavHistoryResult_h_

#include "nsTArray.h"
#include "nsInterfaceHashtable.h"
#include "nsDataHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/storage.h"
#include "Helpers.h"

class nsNavHistory;
class nsNavHistoryQuery;
class nsNavHistoryQueryOptions;

class nsNavHistoryContainerResultNode;
class nsNavHistoryFolderResultNode;
class nsNavHistoryQueryResultNode;









class nsTrimInt64HashKey : public PLDHashEntryHdr
{
public:
  typedef const int64_t& KeyType;
  typedef const int64_t* KeyTypePointer;

  explicit nsTrimInt64HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsTrimInt64HashKey(const nsTrimInt64HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsTrimInt64HashKey() { }

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
    { return static_cast<uint32_t>((*aKey) & UINT32_MAX); }
  enum { ALLOW_MEMMOVE = true };

private:
  const int64_t mValue;
};




#define NS_DECL_BOOKMARK_HISTORY_OBSERVER_BASE(...)                     \
  NS_DECL_NSINAVBOOKMARKOBSERVER                                        \
  NS_IMETHOD OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle,  \
                            const nsACString& aGUID) __VA_ARGS__;       \
  NS_IMETHOD OnFrecencyChanged(nsIURI* aURI, int32_t aNewFrecency,      \
                               const nsACString& aGUID, bool aHidden,   \
                               PRTime aLastVisitDate) __VA_ARGS__;      \
  NS_IMETHOD OnManyFrecenciesChanged() __VA_ARGS__;                     \
  NS_IMETHOD OnDeleteURI(nsIURI *aURI, const nsACString& aGUID,         \
                         uint16_t aReason) __VA_ARGS__;                 \
  NS_IMETHOD OnClearHistory() __VA_ARGS__;                              \
  NS_IMETHOD OnPageChanged(nsIURI *aURI, uint32_t aChangedAttribute,    \
                           const nsAString &aNewValue,                  \
                           const nsACString &aGUID) __VA_ARGS__;        \
  NS_IMETHOD OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime,            \
                            const nsACString& aGUID, uint16_t aReason,  \
                            uint32_t aTransitionType) __VA_ARGS__;




#define NS_DECL_BOOKMARK_HISTORY_OBSERVER_INTERNAL                      \
  NS_DECL_BOOKMARK_HISTORY_OBSERVER_BASE()                              \
  NS_IMETHOD OnVisit(nsIURI* aURI, int64_t aVisitId, PRTime aTime,      \
                     int64_t aSessionId, int64_t aReferringId,          \
                     uint32_t aTransitionType, const nsACString& aGUID, \
                     bool aHidden, uint32_t* aAdded);


#define NS_DECL_BOOKMARK_HISTORY_OBSERVER_EXTERNAL(...)                 \
  NS_DECL_BOOKMARK_HISTORY_OBSERVER_BASE(__VA_ARGS__)                   \
  NS_IMETHOD OnVisit(nsIURI* aURI, int64_t aVisitId, PRTime aTime,      \
                     int64_t aSessionId, int64_t aReferringId,          \
                     uint32_t aTransitionType, const nsACString& aGUID, \
                     bool aHidden) __VA_ARGS__;







#define NS_NAVHISTORYRESULT_IID \
  { 0x455d1d40, 0x1b9b, 0x40e6, { 0xa6, 0x41, 0x8b, 0xb7, 0xe8, 0x82, 0x23, 0x87 } }

class nsNavHistoryResult final : public nsSupportsWeakReference,
                                 public nsINavHistoryResult,
                                 public nsINavBookmarkObserver,
                                 public nsINavHistoryObserver
{
public:
  static nsresult NewHistoryResult(nsINavHistoryQuery** aQueries,
                                   uint32_t aQueryCount,
                                   nsNavHistoryQueryOptions* aOptions,
                                   nsNavHistoryContainerResultNode* aRoot,
                                   bool aBatchInProgress,
                                   nsNavHistoryResult** result);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYRESULT_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSINAVHISTORYRESULT
  NS_DECL_BOOKMARK_HISTORY_OBSERVER_EXTERNAL(override)
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsNavHistoryResult, nsINavHistoryResult)

  void AddHistoryObserver(nsNavHistoryQueryResultNode* aNode);
  void AddBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode, int64_t aFolder);
  void AddAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode);
  void RemoveHistoryObserver(nsNavHistoryQueryResultNode* aNode);
  void RemoveBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode, int64_t aFolder);
  void RemoveAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode);
  void StopObserving();

public:
  
  explicit nsNavHistoryResult(nsNavHistoryContainerResultNode* mRoot);
  nsresult Init(nsINavHistoryQuery** aQueries,
                uint32_t aQueryCount,
                nsNavHistoryQueryOptions *aOptions);

  nsRefPtr<nsNavHistoryContainerResultNode> mRootNode;

  nsCOMArray<nsINavHistoryQuery> mQueries;
  nsCOMPtr<nsNavHistoryQueryOptions> mOptions;

  
  
  uint16_t mSortingMode;
  
  
  
  bool mNeedsToApplySortingMode;

  
  nsCString mSortingAnnotation;

  
  bool mIsHistoryObserver;
  bool mIsBookmarkFolderObserver;
  bool mIsAllBookmarksObserver;

  typedef nsTArray< nsRefPtr<nsNavHistoryQueryResultNode> > QueryObserverList;
  QueryObserverList mHistoryObservers;
  QueryObserverList mAllBookmarksObservers;

  typedef nsTArray< nsRefPtr<nsNavHistoryFolderResultNode> > FolderObserverList;
  nsDataHashtable<nsTrimInt64HashKey, FolderObserverList*> mBookmarkFolderObservers;
  FolderObserverList* BookmarkFolderObserversForId(int64_t aFolderId, bool aCreate);

  typedef nsTArray< nsRefPtr<nsNavHistoryContainerResultNode> > ContainerObserverList;

  void RecursiveExpandCollapse(nsNavHistoryContainerResultNode* aContainer,
                               bool aExpand);

  void InvalidateTree();
  
  bool mBatchInProgress;

  nsMaybeWeakPtrArray<nsINavHistoryResultObserver> mObservers;
  bool mSuppressNotifications;

  ContainerObserverList mRefreshParticipants;
  void requestRefresh(nsNavHistoryContainerResultNode* aContainer);

protected:
  virtual ~nsNavHistoryResult();
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryResult, NS_NAVHISTORYRESULT_IID)







#define NS_NAVHISTORYRESULTNODE_IID \
  {0x54b61d38, 0x57c1, 0x11da, {0x95, 0xb8, 0x00, 0x13, 0x21, 0xc9, 0xf6, 0x9e}}





#define NS_IMPLEMENT_SIMPLE_RESULTNODE \
  NS_IMETHOD GetTitle(nsACString& aTitle) override \
    { aTitle = mTitle; return NS_OK; } \
  NS_IMETHOD GetAccessCount(uint32_t* aAccessCount) override \
    { *aAccessCount = mAccessCount; return NS_OK; } \
  NS_IMETHOD GetTime(PRTime* aTime) override \
    { *aTime = mTime; return NS_OK; } \
  NS_IMETHOD GetIndentLevel(int32_t* aIndentLevel) override \
    { *aIndentLevel = mIndentLevel; return NS_OK; } \
  NS_IMETHOD GetBookmarkIndex(int32_t* aIndex) override \
    { *aIndex = mBookmarkIndex; return NS_OK; } \
  NS_IMETHOD GetDateAdded(PRTime* aDateAdded) override \
    { *aDateAdded = mDateAdded; return NS_OK; } \
  NS_IMETHOD GetLastModified(PRTime* aLastModified) override \
    { *aLastModified = mLastModified; return NS_OK; } \
  NS_IMETHOD GetItemId(int64_t* aId) override \
    { *aId = mItemId; return NS_OK; }










#define NS_FORWARD_COMMON_RESULTNODE_TO_BASE \
  NS_IMPLEMENT_SIMPLE_RESULTNODE \
  NS_IMETHOD GetIcon(nsACString& aIcon) override \
    { return nsNavHistoryResultNode::GetIcon(aIcon); } \
  NS_IMETHOD GetParent(nsINavHistoryContainerResultNode** aParent) override \
    { return nsNavHistoryResultNode::GetParent(aParent); } \
  NS_IMETHOD GetParentResult(nsINavHistoryResult** aResult) override \
    { return nsNavHistoryResultNode::GetParentResult(aResult); } \
  NS_IMETHOD GetTags(nsAString& aTags) override \
    { return nsNavHistoryResultNode::GetTags(aTags); } \
  NS_IMETHOD GetPageGuid(nsACString& aPageGuid) override \
    { return nsNavHistoryResultNode::GetPageGuid(aPageGuid); } \
  NS_IMETHOD GetBookmarkGuid(nsACString& aBookmarkGuid) override \
    { return nsNavHistoryResultNode::GetBookmarkGuid(aBookmarkGuid); }

class nsNavHistoryResultNode : public nsINavHistoryResultNode
{
public:
  nsNavHistoryResultNode(const nsACString& aURI, const nsACString& aTitle,
                         uint32_t aAccessCount, PRTime aTime,
                         const nsACString& aIconURI);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYRESULTNODE_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsNavHistoryResultNode)

  NS_IMPLEMENT_SIMPLE_RESULTNODE
  NS_IMETHOD GetIcon(nsACString& aIcon) override;
  NS_IMETHOD GetParent(nsINavHistoryContainerResultNode** aParent) override;
  NS_IMETHOD GetParentResult(nsINavHistoryResult** aResult) override;
  NS_IMETHOD GetType(uint32_t* type) override
    { *type = nsNavHistoryResultNode::RESULT_TYPE_URI; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI) override
    { aURI = mURI; return NS_OK; }
  NS_IMETHOD GetTags(nsAString& aTags) override;
  NS_IMETHOD GetPageGuid(nsACString& aPageGuid) override;
  NS_IMETHOD GetBookmarkGuid(nsACString& aBookmarkGuid) override;

  virtual void OnRemoving();

  
  
  NS_IMETHOD OnItemChanged(int64_t aItemId,
                           const nsACString &aProperty,
                           bool aIsAnnotationProperty,
                           const nsACString &aValue,
                           PRTime aNewLastModified,
                           uint16_t aItemType,
                           int64_t aParentId,
                           const nsACString& aGUID,
                           const nsACString& aParentGUID);

protected:
  virtual ~nsNavHistoryResultNode() {}

public:

  nsNavHistoryResult* GetResult();
  nsNavHistoryQueryOptions* GetGeneratingOptions();

  
  
  
  bool IsTypeContainer(uint32_t type) {
    return type == nsINavHistoryResultNode::RESULT_TYPE_QUERY ||
           type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER ||
           type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT;
  }
  bool IsContainer() {
    uint32_t type;
    GetType(&type);
    return IsTypeContainer(type);
  }
  static bool IsTypeURI(uint32_t type) {
    return type == nsINavHistoryResultNode::RESULT_TYPE_URI;
  }
  bool IsURI() {
    uint32_t type;
    GetType(&type);
    return IsTypeURI(type);
  }
  static bool IsTypeFolder(uint32_t type) {
    return type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER ||
           type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT;
  }
  bool IsFolder() {
    uint32_t type;
    GetType(&type);
    return IsTypeFolder(type);
  }
  static bool IsTypeQuery(uint32_t type) {
    return type == nsINavHistoryResultNode::RESULT_TYPE_QUERY;
  }
  bool IsQuery() {
    uint32_t type;
    GetType(&type);
    return IsTypeQuery(type);
  }
  bool IsSeparator() {
    uint32_t type;
    GetType(&type);
    return type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR;
  }
  nsNavHistoryContainerResultNode* GetAsContainer() {
    NS_ASSERTION(IsContainer(), "Not a container");
    return reinterpret_cast<nsNavHistoryContainerResultNode*>(this);
  }
  nsNavHistoryFolderResultNode* GetAsFolder() {
    NS_ASSERTION(IsFolder(), "Not a folder");
    return reinterpret_cast<nsNavHistoryFolderResultNode*>(this);
  }
  nsNavHistoryQueryResultNode* GetAsQuery() {
    NS_ASSERTION(IsQuery(), "Not a query");
    return reinterpret_cast<nsNavHistoryQueryResultNode*>(this);
  }

  nsRefPtr<nsNavHistoryContainerResultNode> mParent;
  nsCString mURI; 
  nsCString mTitle;
  nsString mTags;
  bool mAreTagsSorted;
  uint32_t mAccessCount;
  int64_t mTime;
  nsCString mFaviconURI;
  int32_t mBookmarkIndex;
  int64_t mItemId;
  int64_t mFolderId;
  PRTime mDateAdded;
  PRTime mLastModified;

  
  
  int32_t mIndentLevel;

  
  int32_t mFrecency;

  
  bool mHidden;

  
  uint32_t mTransitionType;

  
  nsCString mPageGuid;

  
  nsCString mBookmarkGuid;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryResultNode, NS_NAVHISTORYRESULTNODE_IID)











#define NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN \
  NS_IMETHOD GetState(uint16_t* _state) override \
    { return nsNavHistoryContainerResultNode::GetState(_state); } \
  NS_IMETHOD GetContainerOpen(bool *aContainerOpen) override \
    { return nsNavHistoryContainerResultNode::GetContainerOpen(aContainerOpen); } \
  NS_IMETHOD SetContainerOpen(bool aContainerOpen) override \
    { return nsNavHistoryContainerResultNode::SetContainerOpen(aContainerOpen); } \
  NS_IMETHOD GetChildCount(uint32_t *aChildCount) override \
    { return nsNavHistoryContainerResultNode::GetChildCount(aChildCount); } \
  NS_IMETHOD GetChild(uint32_t index, nsINavHistoryResultNode **_retval) override \
    { return nsNavHistoryContainerResultNode::GetChild(index, _retval); } \
  NS_IMETHOD GetChildIndex(nsINavHistoryResultNode* aNode, uint32_t* _retval) override \
    { return nsNavHistoryContainerResultNode::GetChildIndex(aNode, _retval); } \
  NS_IMETHOD FindNodeByDetails(const nsACString& aURIString, PRTime aTime, \
                               int64_t aItemId, bool aRecursive, \
                               nsINavHistoryResultNode** _retval) override \
    { return nsNavHistoryContainerResultNode::FindNodeByDetails(aURIString, aTime, aItemId, \
                                                                aRecursive, _retval); }

#define NS_NAVHISTORYCONTAINERRESULTNODE_IID \
  { 0x6e3bf8d3, 0x22aa, 0x4065, { 0x86, 0xbc, 0x37, 0x46, 0xb5, 0xb3, 0x2c, 0xe8 } }

class nsNavHistoryContainerResultNode : public nsNavHistoryResultNode,
                                        public nsINavHistoryContainerResultNode
{
public:
  nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    const nsACString& aIconURI, uint32_t aContainerType,
    nsNavHistoryQueryOptions* aOptions);
  nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    PRTime aTime,
    const nsACString& aIconURI, uint32_t aContainerType,
    nsNavHistoryQueryOptions* aOptions);

  virtual nsresult Refresh();

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYCONTAINERRESULTNODE_IID)

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(uint32_t* type) override
    { *type = mContainerType; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI) override
    { aURI = mURI; return NS_OK; }
  NS_DECL_NSINAVHISTORYCONTAINERRESULTNODE

public:

  virtual void OnRemoving() override;

  bool AreChildrenVisible();

  
  virtual nsresult OpenContainer();
  nsresult CloseContainer(bool aSuppressNotifications = false);

  virtual nsresult OpenContainerAsync();

  
  
  
  
  nsRefPtr<nsNavHistoryResult> mResult;

  
  
  uint32_t mContainerType;

  
  
  bool mExpanded;

  
  nsCOMArray<nsNavHistoryResultNode> mChildren;

  nsCOMPtr<nsNavHistoryQueryOptions> mOptions;

  void FillStats();
  nsresult ReverseUpdateStats(int32_t aAccessCountChange);

  
  typedef nsCOMArray<nsNavHistoryResultNode>::nsCOMArrayComparatorFunc SortComparator;
  virtual uint16_t GetSortType();
  virtual void GetSortingAnnotation(nsACString& aSortingAnnotation);

  static SortComparator GetSortingComparator(uint16_t aSortType);
  virtual void RecursiveSort(const char* aData,
                             SortComparator aComparator);
  uint32_t FindInsertionPoint(nsNavHistoryResultNode* aNode, SortComparator aComparator,
                              const char* aData, bool* aItemExists);
  bool DoesChildNeedResorting(uint32_t aIndex, SortComparator aComparator,
                                const char* aData);

  static int32_t SortComparison_StringLess(const nsAString& a, const nsAString& b);

  static int32_t SortComparison_Bookmark(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static int32_t SortComparison_TitleLess(nsNavHistoryResultNode* a,
                                          nsNavHistoryResultNode* b,
                                          void* closure);
  static int32_t SortComparison_TitleGreater(nsNavHistoryResultNode* a,
                                             nsNavHistoryResultNode* b,
                                             void* closure);
  static int32_t SortComparison_DateLess(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static int32_t SortComparison_DateGreater(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static int32_t SortComparison_URILess(nsNavHistoryResultNode* a,
                                        nsNavHistoryResultNode* b,
                                        void* closure);
  static int32_t SortComparison_URIGreater(nsNavHistoryResultNode* a,
                                           nsNavHistoryResultNode* b,
                                           void* closure);
  static int32_t SortComparison_VisitCountLess(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static int32_t SortComparison_VisitCountGreater(nsNavHistoryResultNode* a,
                                                  nsNavHistoryResultNode* b,
                                                  void* closure);
  static int32_t SortComparison_KeywordLess(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static int32_t SortComparison_KeywordGreater(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static int32_t SortComparison_AnnotationLess(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static int32_t SortComparison_AnnotationGreater(nsNavHistoryResultNode* a,
                                                  nsNavHistoryResultNode* b,
                                                  void* closure);
  static int32_t SortComparison_DateAddedLess(nsNavHistoryResultNode* a,
                                              nsNavHistoryResultNode* b,
                                              void* closure);
  static int32_t SortComparison_DateAddedGreater(nsNavHistoryResultNode* a,
                                                 nsNavHistoryResultNode* b,
                                                 void* closure);
  static int32_t SortComparison_LastModifiedLess(nsNavHistoryResultNode* a,
                                                 nsNavHistoryResultNode* b,
                                                 void* closure);
  static int32_t SortComparison_LastModifiedGreater(nsNavHistoryResultNode* a,
                                                    nsNavHistoryResultNode* b,
                                                    void* closure);
  static int32_t SortComparison_TagsLess(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static int32_t SortComparison_TagsGreater(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static int32_t SortComparison_FrecencyLess(nsNavHistoryResultNode* a,
                                             nsNavHistoryResultNode* b,
                                             void* closure);
  static int32_t SortComparison_FrecencyGreater(nsNavHistoryResultNode* a,
                                                nsNavHistoryResultNode* b,
                                                void* closure);

  
  nsNavHistoryResultNode* FindChildURI(nsIURI* aURI, uint32_t* aNodeIndex)
  {
    nsAutoCString spec;
    if (NS_FAILED(aURI->GetSpec(spec)))
      return nullptr;
    return FindChildURI(spec, aNodeIndex);
  }
  nsNavHistoryResultNode* FindChildURI(const nsACString& aSpec,
                                       uint32_t* aNodeIndex);
  
  int32_t FindChild(nsNavHistoryResultNode* aNode)
    { return mChildren.IndexOf(aNode); }

  nsresult InsertChildAt(nsNavHistoryResultNode* aNode, int32_t aIndex);
  nsresult InsertSortedChild(nsNavHistoryResultNode* aNode,
                             bool aIgnoreDuplicates = false);
  bool EnsureItemPosition(uint32_t aIndex);

  nsresult RemoveChildAt(int32_t aIndex);

  void RecursiveFindURIs(bool aOnlyOne,
                         nsNavHistoryContainerResultNode* aContainer,
                         const nsCString& aSpec,
                         nsCOMArray<nsNavHistoryResultNode>* aMatches);
  bool UpdateURIs(bool aRecursive, bool aOnlyOne, bool aUpdateSort,
                  const nsCString& aSpec,
                  nsresult (*aCallback)(nsNavHistoryResultNode*, const void*,
                                        const nsNavHistoryResult*),
                  const void* aClosure);
  nsresult ChangeTitles(nsIURI* aURI, const nsACString& aNewTitle,
                        bool aRecursive, bool aOnlyOne);

protected:
  virtual ~nsNavHistoryContainerResultNode();

  enum AsyncCanceledState {
    NOT_CANCELED, CANCELED, CANCELED_RESTART_NEEDED
  };

  void CancelAsyncOpen(bool aRestart);
  nsresult NotifyOnStateChange(uint16_t aOldState);

  nsCOMPtr<mozIStoragePendingStatement> mAsyncPendingStmt;
  AsyncCanceledState mAsyncCanceledState;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryContainerResultNode,
                              NS_NAVHISTORYCONTAINERRESULTNODE_IID)







class nsNavHistoryQueryResultNode final : public nsNavHistoryContainerResultNode,
                                          public nsINavHistoryQueryResultNode,
                                          public nsINavBookmarkObserver
{
public:
  nsNavHistoryQueryResultNode(const nsACString& aTitle,
                              const nsACString& aIconURI,
                              const nsACString& aQueryURI);
  nsNavHistoryQueryResultNode(const nsACString& aTitle,
                              const nsACString& aIconURI,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions);
  nsNavHistoryQueryResultNode(const nsACString& aTitle,
                              const nsACString& aIconURI,
                              PRTime aTime,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(uint32_t* type) override
    { *type = nsNavHistoryResultNode::RESULT_TYPE_QUERY; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI) override; 
  NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN
  NS_IMETHOD GetHasChildren(bool* aHasChildren) override;
  NS_DECL_NSINAVHISTORYQUERYRESULTNODE

  bool CanExpand();
  bool IsContainersQuery();

  virtual nsresult OpenContainer() override;

  NS_DECL_BOOKMARK_HISTORY_OBSERVER_INTERNAL
  virtual void OnRemoving() override;

public:
  
  
  nsresult VerifyQueriesSerialized();

  
  
  nsCOMArray<nsNavHistoryQuery> mQueries;
  uint32_t mLiveUpdate; 
  bool mHasSearchTerms;
  nsresult VerifyQueriesParsed();

  
  nsNavHistoryQueryOptions* Options();

  
  
  bool mContentsValid;

  nsresult FillChildren();
  void ClearChildren(bool unregister);
  nsresult Refresh() override;

  virtual uint16_t GetSortType() override;
  virtual void GetSortingAnnotation(nsACString& aSortingAnnotation) override;
  virtual void RecursiveSort(const char* aData,
                             SortComparator aComparator) override;

  nsresult NotifyIfTagsChanged(nsIURI* aURI);

  uint32_t mBatchChanges;

  
  nsTArray<uint32_t> mTransitions;

protected:
  virtual ~nsNavHistoryQueryResultNode();
};







class nsNavHistoryFolderResultNode final : public nsNavHistoryContainerResultNode,
                                           public nsINavHistoryQueryResultNode,
                                           public nsINavBookmarkObserver,
                                           public mozilla::places::AsyncStatementCallback
{
public:
  nsNavHistoryFolderResultNode(const nsACString& aTitle,
                               nsNavHistoryQueryOptions* options,
                               int64_t aFolderId);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(uint32_t* type) override {
    if (mTargetFolderItemId != mItemId) {
      *type = nsNavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT;
    } else {
      *type = nsNavHistoryResultNode::RESULT_TYPE_FOLDER;
    }
    return NS_OK;
  }
  NS_IMETHOD GetUri(nsACString& aURI) override;
  NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN
  NS_IMETHOD GetHasChildren(bool* aHasChildren) override;
  NS_DECL_NSINAVHISTORYQUERYRESULTNODE

  virtual nsresult OpenContainer() override;

  virtual nsresult OpenContainerAsync() override;
  NS_DECL_ASYNCSTATEMENTCALLBACK

  
  
  NS_DECL_NSINAVBOOKMARKOBSERVER

  virtual void OnRemoving() override;

  
  
  bool mContentsValid;

  
  
  
  
  int64_t mTargetFolderItemId;
  nsCString mTargetFolderGuid;

  nsresult FillChildren();
  void ClearChildren(bool aUnregister);
  nsresult Refresh() override;

  bool StartIncrementalUpdate();
  void ReindexRange(int32_t aStartIndex, int32_t aEndIndex, int32_t aDelta);

  nsNavHistoryResultNode* FindChildById(int64_t aItemId,
                                        uint32_t* aNodeIndex);

protected:
  virtual ~nsNavHistoryFolderResultNode();

private:

  nsresult OnChildrenFilled();
  void EnsureRegisteredAsFolderObserver();
  nsresult FillChildrenAsync();

  bool mIsRegisteredFolderObserver;
  int32_t mAsyncBookmarkIndex;
};




class nsNavHistorySeparatorResultNode : public nsNavHistoryResultNode
{
public:
  nsNavHistorySeparatorResultNode();

  NS_IMETHOD GetType(uint32_t* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_SEPARATOR; return NS_OK; }
};

#endif 
