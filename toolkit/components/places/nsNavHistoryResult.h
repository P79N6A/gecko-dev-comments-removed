











































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
class nsNavHistoryVisitResultNode;









class nsTrimInt64HashKey : public PLDHashEntryHdr
{
public:
  typedef const PRInt64& KeyType;
  typedef const PRInt64* KeyTypePointer;

  nsTrimInt64HashKey(KeyTypePointer aKey) : mValue(*aKey) { }
  nsTrimInt64HashKey(const nsTrimInt64HashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsTrimInt64HashKey() { }

  KeyType GetKey() const { return mValue; }
  bool KeyEquals(KeyTypePointer aKey) const { return *aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
    { return static_cast<PRUint32>((*aKey) & PR_UINT32_MAX); }
  enum { ALLOW_MEMMOVE = true };

private:
  const PRInt64 mValue;
};




#define NS_DECL_BOOKMARK_HISTORY_OBSERVER                               \
  NS_DECL_NSINAVBOOKMARKOBSERVER                                        \
  NS_IMETHOD OnVisit(nsIURI* aURI, PRInt64 aVisitId, PRTime aTime,      \
                     PRInt64 aSessionId, PRInt64 aReferringId,          \
                     PRUint32 aTransitionType, const nsACString& aGUID, \
                     PRUint32* aAdded);                                 \
  NS_IMETHOD OnTitleChanged(nsIURI* aURI, const nsAString& aPageTitle,  \
                            const nsACString& aGUID);                   \
  NS_IMETHOD OnBeforeDeleteURI(nsIURI *aURI, const nsACString& aGUID,   \
                               PRUint16 aReason);                       \
  NS_IMETHOD OnDeleteURI(nsIURI *aURI, const nsACString& aGUID,         \
                         PRUint16 aReason);                             \
  NS_IMETHOD OnClearHistory();                                          \
  NS_IMETHOD OnPageChanged(nsIURI *aURI, PRUint32 aChangedAttribute,    \
                           const nsAString &aNewValue,                  \
                           const nsACString &aGUID);                    \
  NS_IMETHOD OnDeleteVisits(nsIURI* aURI, PRTime aVisitTime,            \
                            const nsACString& aGUID, PRUint16 aReason);







#define NS_NAVHISTORYRESULT_IID \
  { 0x455d1d40, 0x1b9b, 0x40e6, { 0xa6, 0x41, 0x8b, 0xb7, 0xe8, 0x82, 0x23, 0x87 } }

class nsNavHistoryResult : public nsSupportsWeakReference,
                           public nsINavHistoryResult,
                           public nsINavBookmarkObserver,
                           public nsINavHistoryObserver
{
public:
  static nsresult NewHistoryResult(nsINavHistoryQuery** aQueries,
                                   PRUint32 aQueryCount,
                                   nsNavHistoryQueryOptions* aOptions,
                                   nsNavHistoryContainerResultNode* aRoot,
                                   bool aBatchInProgress,
                                   nsNavHistoryResult** result);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYRESULT_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSINAVHISTORYRESULT
  NS_DECL_BOOKMARK_HISTORY_OBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsNavHistoryResult, nsINavHistoryResult)

  void AddHistoryObserver(nsNavHistoryQueryResultNode* aNode);
  void AddBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode, PRInt64 aFolder);
  void AddAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode);
  void RemoveHistoryObserver(nsNavHistoryQueryResultNode* aNode);
  void RemoveBookmarkFolderObserver(nsNavHistoryFolderResultNode* aNode, PRInt64 aFolder);
  void RemoveAllBookmarksObserver(nsNavHistoryQueryResultNode* aNode);
  void StopObserving();

public:
  
  nsNavHistoryResult(nsNavHistoryContainerResultNode* mRoot);
  virtual ~nsNavHistoryResult();
  nsresult Init(nsINavHistoryQuery** aQueries,
                PRUint32 aQueryCount,
                nsNavHistoryQueryOptions *aOptions);

  nsRefPtr<nsNavHistoryContainerResultNode> mRootNode;

  nsCOMArray<nsINavHistoryQuery> mQueries;
  nsCOMPtr<nsNavHistoryQueryOptions> mOptions;

  
  
  PRUint16 mSortingMode;
  
  
  
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
  FolderObserverList* BookmarkFolderObserversForId(PRInt64 aFolderId, bool aCreate);

  typedef nsTArray< nsRefPtr<nsNavHistoryContainerResultNode> > ContainerObserverList;

  void RecursiveExpandCollapse(nsNavHistoryContainerResultNode* aContainer,
                               bool aExpand);

  void InvalidateTree();
  
  bool mBatchInProgress;

  nsMaybeWeakPtrArray<nsINavHistoryResultObserver> mObservers;
  bool mSuppressNotifications;

  ContainerObserverList mRefreshParticipants;
  void requestRefresh(nsNavHistoryContainerResultNode* aContainer);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryResult, NS_NAVHISTORYRESULT_IID)







#define NS_NAVHISTORYRESULTNODE_IID \
  {0x54b61d38, 0x57c1, 0x11da, {0x95, 0xb8, 0x00, 0x13, 0x21, 0xc9, 0xf6, 0x9e}}





#define NS_IMPLEMENT_SIMPLE_RESULTNODE_NO_GETITEMMID \
  NS_IMETHOD GetTitle(nsACString& aTitle) \
    { aTitle = mTitle; return NS_OK; } \
  NS_IMETHOD GetAccessCount(PRUint32* aAccessCount) \
    { *aAccessCount = mAccessCount; return NS_OK; } \
  NS_IMETHOD GetTime(PRTime* aTime) \
    { *aTime = mTime; return NS_OK; } \
  NS_IMETHOD GetIndentLevel(PRInt32* aIndentLevel) \
    { *aIndentLevel = mIndentLevel; return NS_OK; } \
  NS_IMETHOD GetBookmarkIndex(PRInt32* aIndex) \
    { *aIndex = mBookmarkIndex; return NS_OK; } \
  NS_IMETHOD GetDateAdded(PRTime* aDateAdded) \
    { *aDateAdded = mDateAdded; return NS_OK; } \
  NS_IMETHOD GetLastModified(PRTime* aLastModified) \
    { *aLastModified = mLastModified; return NS_OK; }

#define NS_IMPLEMENT_SIMPLE_RESULTNODE \
  NS_IMPLEMENT_SIMPLE_RESULTNODE_NO_GETITEMMID \
  NS_IMETHOD GetItemId(PRInt64* aId) \
    { *aId = mItemId; return NS_OK; }










#define NS_FORWARD_COMMON_RESULTNODE_TO_BASE_NO_GETITEMMID \
  NS_IMPLEMENT_SIMPLE_RESULTNODE_NO_GETITEMMID \
  NS_IMETHOD GetIcon(nsACString& aIcon) \
    { return nsNavHistoryResultNode::GetIcon(aIcon); } \
  NS_IMETHOD GetParent(nsINavHistoryContainerResultNode** aParent) \
    { return nsNavHistoryResultNode::GetParent(aParent); } \
  NS_IMETHOD GetParentResult(nsINavHistoryResult** aResult) \
    { return nsNavHistoryResultNode::GetParentResult(aResult); } \
  NS_IMETHOD GetTags(nsAString& aTags) \
    { return nsNavHistoryResultNode::GetTags(aTags); }

#define NS_FORWARD_COMMON_RESULTNODE_TO_BASE \
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE_NO_GETITEMMID \
  NS_IMETHOD GetItemId(PRInt64* aId) \
    { *aId = mItemId; return NS_OK; }

class nsNavHistoryResultNode : public nsINavHistoryResultNode
{
public:
  nsNavHistoryResultNode(const nsACString& aURI, const nsACString& aTitle,
                         PRUint32 aAccessCount, PRTime aTime,
                         const nsACString& aIconURI);
  virtual ~nsNavHistoryResultNode() {}

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYRESULTNODE_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsNavHistoryResultNode)

  NS_IMPLEMENT_SIMPLE_RESULTNODE
  NS_IMETHOD GetIcon(nsACString& aIcon);
  NS_IMETHOD GetParent(nsINavHistoryContainerResultNode** aParent);
  NS_IMETHOD GetParentResult(nsINavHistoryResult** aResult);
  NS_IMETHOD GetType(PRUint32* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_URI; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI)
    { aURI = mURI; return NS_OK; }
  NS_IMETHOD GetTags(nsAString& aTags);

  virtual void OnRemoving();

  
  
  NS_IMETHOD OnItemChanged(PRInt64 aItemId,
                           const nsACString &aProperty,
                           bool aIsAnnotationProperty,
                           const nsACString &aValue,
                           PRTime aNewLastModified,
                           PRUint16 aItemType,
                           PRInt64 aParentId,
                           const nsACString& aGUID,
                           const nsACString& aParentGUID);

public:

  nsNavHistoryResult* GetResult();
  nsNavHistoryQueryOptions* GetGeneratingOptions();

  
  
  
  bool IsTypeContainer(PRUint32 type) {
    return (type == nsINavHistoryResultNode::RESULT_TYPE_QUERY ||
            type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER ||
            type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT);
  }
  bool IsContainer() {
    PRUint32 type;
    GetType(&type);
    return IsTypeContainer(type);
  }
  static bool IsTypeURI(PRUint32 type) {
    return (type == nsINavHistoryResultNode::RESULT_TYPE_URI ||
            type == nsINavHistoryResultNode::RESULT_TYPE_VISIT ||
            type == nsINavHistoryResultNode::RESULT_TYPE_FULL_VISIT);
  }
  bool IsURI() {
    PRUint32 type;
    GetType(&type);
    return IsTypeURI(type);
  }
  static bool IsTypeVisit(PRUint32 type) {
    return (type == nsINavHistoryResultNode::RESULT_TYPE_VISIT ||
            type == nsINavHistoryResultNode::RESULT_TYPE_FULL_VISIT);
  }
  bool IsVisit() {
    PRUint32 type;
    GetType(&type);
    return IsTypeVisit(type);
  }
  static bool IsTypeFolder(PRUint32 type) {
    return (type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER ||
            type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT);
  }
  bool IsFolder() {
    PRUint32 type;
    GetType(&type);
    return IsTypeFolder(type);
  }
  static bool IsTypeQuery(PRUint32 type) {
    return (type == nsINavHistoryResultNode::RESULT_TYPE_QUERY);
  }
  bool IsQuery() {
    PRUint32 type;
    GetType(&type);
    return IsTypeQuery(type);
  }
  bool IsSeparator() {
    PRUint32 type;
    GetType(&type);
    return (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR);
  }
  nsNavHistoryContainerResultNode* GetAsContainer() {
    NS_ASSERTION(IsContainer(), "Not a container");
    return reinterpret_cast<nsNavHistoryContainerResultNode*>(this);
  }
  nsNavHistoryVisitResultNode* GetAsVisit() {
    NS_ASSERTION(IsVisit(), "Not a visit");
    return reinterpret_cast<nsNavHistoryVisitResultNode*>(this);
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
  PRUint32 mAccessCount;
  PRInt64 mTime;
  nsCString mFaviconURI;
  PRInt32 mBookmarkIndex;
  PRInt64 mItemId;
  PRInt64 mFolderId;
  PRTime mDateAdded;
  PRTime mLastModified;

  
  
  PRInt32 mIndentLevel;

  PRInt32 mFrecency; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryResultNode, NS_NAVHISTORYRESULTNODE_IID)



#define NS_IMPLEMENT_VISITRESULT \
  NS_IMETHOD GetUri(nsACString& aURI) { aURI = mURI; return NS_OK; } \
  NS_IMETHOD GetSessionId(PRInt64* aSessionId) \
    { *aSessionId = mSessionId; return NS_OK; }

class nsNavHistoryVisitResultNode : public nsNavHistoryResultNode,
                                    public nsINavHistoryVisitResultNode
{
public:
  nsNavHistoryVisitResultNode(const nsACString& aURI, const nsACString& aTitle,
                              PRUint32 aAccessCount, PRTime aTime,
                              const nsACString& aIconURI, PRInt64 aSession);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(PRUint32* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_VISIT; return NS_OK; }
  NS_IMPLEMENT_VISITRESULT

public:

  PRInt64 mSessionId;
};




#define NS_IMPLEMENT_FULLVISITRESULT \
  NS_IMPLEMENT_VISITRESULT \
  NS_IMETHOD GetVisitId(PRInt64 *aVisitId) \
    { *aVisitId = mVisitId; return NS_OK; } \
  NS_IMETHOD GetReferringVisitId(PRInt64 *aReferringVisitId) \
    { *aReferringVisitId = mReferringVisitId; return NS_OK; } \
  NS_IMETHOD GetTransitionType(PRInt32 *aTransitionType) \
    { *aTransitionType = mTransitionType; return NS_OK; }

class nsNavHistoryFullVisitResultNode : public nsNavHistoryVisitResultNode,
                                        public nsINavHistoryFullVisitResultNode
{
public:
  nsNavHistoryFullVisitResultNode(
    const nsACString& aURI, const nsACString& aTitle, PRUint32 aAccessCount,
    PRTime aTime, const nsACString& aIconURI, PRInt64 aSession,
    PRInt64 aVisitId, PRInt64 aReferringVisitId, PRInt32 aTransitionType);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(PRUint32* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_FULL_VISIT; return NS_OK; }
  NS_IMPLEMENT_FULLVISITRESULT

public:
  PRInt64 mVisitId;
  PRInt64 mReferringVisitId;
  PRInt32 mTransitionType;
};











#define NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN_AND_READONLY \
  NS_IMETHOD GetState(PRUint16* _state) \
    { return nsNavHistoryContainerResultNode::GetState(_state); } \
  NS_IMETHOD GetContainerOpen(bool *aContainerOpen) \
    { return nsNavHistoryContainerResultNode::GetContainerOpen(aContainerOpen); } \
  NS_IMETHOD SetContainerOpen(bool aContainerOpen) \
    { return nsNavHistoryContainerResultNode::SetContainerOpen(aContainerOpen); } \
  NS_IMETHOD GetChildCount(PRUint32 *aChildCount) \
    { return nsNavHistoryContainerResultNode::GetChildCount(aChildCount); } \
  NS_IMETHOD GetChild(PRUint32 index, nsINavHistoryResultNode **_retval) \
    { return nsNavHistoryContainerResultNode::GetChild(index, _retval); } \
  NS_IMETHOD GetChildIndex(nsINavHistoryResultNode* aNode, PRUint32* _retval) \
    { return nsNavHistoryContainerResultNode::GetChildIndex(aNode, _retval); } \
  NS_IMETHOD FindNodeByDetails(const nsACString& aURIString, PRTime aTime, \
                               PRInt64 aItemId, bool aRecursive, \
                               nsINavHistoryResultNode** _retval) \
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
    const nsACString& aIconURI, PRUint32 aContainerType,
    bool aReadOnly, nsNavHistoryQueryOptions* aOptions);
  nsNavHistoryContainerResultNode(
    const nsACString& aURI, const nsACString& aTitle,
    PRTime aTime,
    const nsACString& aIconURI, PRUint32 aContainerType,
    bool aReadOnly, nsNavHistoryQueryOptions* aOptions);

  virtual nsresult Refresh();
  virtual ~nsNavHistoryContainerResultNode();

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYCONTAINERRESULTNODE_IID)

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsNavHistoryContainerResultNode, nsNavHistoryResultNode)
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(PRUint32* type)
    { *type = mContainerType; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI)
    { aURI = mURI; return NS_OK; }
  NS_DECL_NSINAVHISTORYCONTAINERRESULTNODE

public:

  virtual void OnRemoving();

  bool AreChildrenVisible();

  
  virtual nsresult OpenContainer();
  nsresult CloseContainer(bool aSuppressNotifications = false);

  virtual nsresult OpenContainerAsync();

  
  
  
  
  nsRefPtr<nsNavHistoryResult> mResult;

  
  
  PRUint32 mContainerType;

  
  
  bool mExpanded;

  
  nsCOMArray<nsNavHistoryResultNode> mChildren;

  bool mChildrenReadOnly;

  nsCOMPtr<nsNavHistoryQueryOptions> mOptions;

  void FillStats();
  nsresult ReverseUpdateStats(PRInt32 aAccessCountChange);

  
  typedef nsCOMArray<nsNavHistoryResultNode>::nsCOMArrayComparatorFunc SortComparator;
  virtual PRUint16 GetSortType();
  virtual void GetSortingAnnotation(nsACString& aSortingAnnotation);

  static SortComparator GetSortingComparator(PRUint16 aSortType);
  virtual void RecursiveSort(const char* aData,
                             SortComparator aComparator);
  PRUint32 FindInsertionPoint(nsNavHistoryResultNode* aNode, SortComparator aComparator,
                              const char* aData, bool* aItemExists);
  bool DoesChildNeedResorting(PRUint32 aIndex, SortComparator aComparator,
                                const char* aData);

  static PRInt32 SortComparison_StringLess(const nsAString& a, const nsAString& b);

  static PRInt32 SortComparison_Bookmark(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static PRInt32 SortComparison_TitleLess(nsNavHistoryResultNode* a,
                                          nsNavHistoryResultNode* b,
                                          void* closure);
  static PRInt32 SortComparison_TitleGreater(nsNavHistoryResultNode* a,
                                             nsNavHistoryResultNode* b,
                                             void* closure);
  static PRInt32 SortComparison_DateLess(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static PRInt32 SortComparison_DateGreater(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static PRInt32 SortComparison_URILess(nsNavHistoryResultNode* a,
                                        nsNavHistoryResultNode* b,
                                        void* closure);
  static PRInt32 SortComparison_URIGreater(nsNavHistoryResultNode* a,
                                           nsNavHistoryResultNode* b,
                                           void* closure);
  static PRInt32 SortComparison_VisitCountLess(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static PRInt32 SortComparison_VisitCountGreater(nsNavHistoryResultNode* a,
                                                  nsNavHistoryResultNode* b,
                                                  void* closure);
  static PRInt32 SortComparison_KeywordLess(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static PRInt32 SortComparison_KeywordGreater(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static PRInt32 SortComparison_AnnotationLess(nsNavHistoryResultNode* a,
                                               nsNavHistoryResultNode* b,
                                               void* closure);
  static PRInt32 SortComparison_AnnotationGreater(nsNavHistoryResultNode* a,
                                                  nsNavHistoryResultNode* b,
                                                  void* closure);
  static PRInt32 SortComparison_DateAddedLess(nsNavHistoryResultNode* a,
                                              nsNavHistoryResultNode* b,
                                              void* closure);
  static PRInt32 SortComparison_DateAddedGreater(nsNavHistoryResultNode* a,
                                                 nsNavHistoryResultNode* b,
                                                 void* closure);
  static PRInt32 SortComparison_LastModifiedLess(nsNavHistoryResultNode* a,
                                                 nsNavHistoryResultNode* b,
                                                 void* closure);
  static PRInt32 SortComparison_LastModifiedGreater(nsNavHistoryResultNode* a,
                                                    nsNavHistoryResultNode* b,
                                                    void* closure);
  static PRInt32 SortComparison_TagsLess(nsNavHistoryResultNode* a,
                                         nsNavHistoryResultNode* b,
                                         void* closure);
  static PRInt32 SortComparison_TagsGreater(nsNavHistoryResultNode* a,
                                            nsNavHistoryResultNode* b,
                                            void* closure);
  static PRInt32 SortComparison_FrecencyLess(nsNavHistoryResultNode* a,
                                             nsNavHistoryResultNode* b,
                                             void* closure);
  static PRInt32 SortComparison_FrecencyGreater(nsNavHistoryResultNode* a,
                                                nsNavHistoryResultNode* b,
                                                void* closure);

  
  nsNavHistoryResultNode* FindChildURI(nsIURI* aURI, PRUint32* aNodeIndex)
  {
    nsCAutoString spec;
    if (NS_FAILED(aURI->GetSpec(spec)))
      return false;
    return FindChildURI(spec, aNodeIndex);
  }
  nsNavHistoryResultNode* FindChildURI(const nsACString& aSpec,
                                       PRUint32* aNodeIndex);
  nsNavHistoryContainerResultNode* FindChildContainerByName(const nsACString& aTitle,
                                                            PRUint32* aNodeIndex);
  
  PRInt32 FindChild(nsNavHistoryResultNode* aNode)
    { return mChildren.IndexOf(aNode); }

  nsresult InsertChildAt(nsNavHistoryResultNode* aNode, PRInt32 aIndex,
                         bool aIsTemporary = false);
  nsresult InsertSortedChild(nsNavHistoryResultNode* aNode,
                             bool aIsTemporary = false,
                             bool aIgnoreDuplicates = false);
  bool EnsureItemPosition(PRUint32 aIndex);
  void MergeResults(nsCOMArray<nsNavHistoryResultNode>* aNodes);
  nsresult ReplaceChildURIAt(PRUint32 aIndex, nsNavHistoryResultNode* aNode);
  nsresult RemoveChildAt(PRInt32 aIndex, bool aIsTemporary = false);

  void RecursiveFindURIs(bool aOnlyOne,
                         nsNavHistoryContainerResultNode* aContainer,
                         const nsCString& aSpec,
                         nsCOMArray<nsNavHistoryResultNode>* aMatches);
  nsresult UpdateURIs(bool aRecursive, bool aOnlyOne, bool aUpdateSort,
                      const nsCString& aSpec,
                      nsresult (*aCallback)(nsNavHistoryResultNode*,void*, nsNavHistoryResult*),
                      void* aClosure);
  nsresult ChangeTitles(nsIURI* aURI, const nsACString& aNewTitle,
                        bool aRecursive, bool aOnlyOne);

protected:

  enum AsyncCanceledState {
    NOT_CANCELED, CANCELED, CANCELED_RESTART_NEEDED
  };

  void CancelAsyncOpen(bool aRestart);
  nsresult NotifyOnStateChange(PRUint16 aOldState);

  nsCOMPtr<mozIStoragePendingStatement> mAsyncPendingStmt;
  AsyncCanceledState mAsyncCanceledState;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryContainerResultNode,
                              NS_NAVHISTORYCONTAINERRESULTNODE_IID)







class nsNavHistoryQueryResultNode : public nsNavHistoryContainerResultNode,
                                    public nsINavHistoryQueryResultNode
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

  virtual ~nsNavHistoryQueryResultNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE
  NS_IMETHOD GetType(PRUint32* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_QUERY; return NS_OK; }
  NS_IMETHOD GetUri(nsACString& aURI); 
  NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN_AND_READONLY
  NS_IMETHOD GetHasChildren(bool* aHasChildren);
  NS_IMETHOD GetChildrenReadOnly(bool *aChildrenReadOnly)
    { return nsNavHistoryContainerResultNode::GetChildrenReadOnly(aChildrenReadOnly); }
  NS_DECL_NSINAVHISTORYQUERYRESULTNODE

  bool CanExpand();
  bool IsContainersQuery();

  virtual nsresult OpenContainer();

  NS_DECL_BOOKMARK_HISTORY_OBSERVER
  virtual void OnRemoving();

public:
  
  
  nsresult VerifyQueriesSerialized();

  
  
  nsCOMArray<nsNavHistoryQuery> mQueries;
  PRUint32 mLiveUpdate; 
  bool mHasSearchTerms;
  nsresult VerifyQueriesParsed();

  
  nsNavHistoryQueryOptions* Options();

  
  
  bool mContentsValid;

  nsresult FillChildren();
  void ClearChildren(bool unregister);
  nsresult Refresh();

  virtual PRUint16 GetSortType();
  virtual void GetSortingAnnotation(nsACString& aSortingAnnotation);
  virtual void RecursiveSort(const char* aData,
                             SortComparator aComparator);

  nsCOMPtr<nsIURI> mRemovingURI;
  nsresult NotifyIfTagsChanged(nsIURI* aURI);

  PRUint32 mBatchChanges;
};







class nsNavHistoryFolderResultNode : public nsNavHistoryContainerResultNode,
                                     public nsINavHistoryQueryResultNode,
                                     public mozilla::places::AsyncStatementCallback
{
public:
  nsNavHistoryFolderResultNode(const nsACString& aTitle,
                               nsNavHistoryQueryOptions* options,
                               PRInt64 aFolderId);

  virtual ~nsNavHistoryFolderResultNode();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_COMMON_RESULTNODE_TO_BASE_NO_GETITEMMID
  NS_IMETHOD GetType(PRUint32* type) {
    if (mQueryItemId != -1) {
      *type = nsNavHistoryResultNode::RESULT_TYPE_FOLDER_SHORTCUT;
    } else {
      *type = nsNavHistoryResultNode::RESULT_TYPE_FOLDER;
    }
    return NS_OK;
  }
  NS_IMETHOD GetUri(nsACString& aURI);
  NS_FORWARD_CONTAINERNODE_EXCEPT_HASCHILDREN_AND_READONLY
  NS_IMETHOD GetHasChildren(bool* aHasChildren);
  NS_IMETHOD GetChildrenReadOnly(bool *aChildrenReadOnly);
  NS_IMETHOD GetItemId(PRInt64 *aItemId);
  NS_DECL_NSINAVHISTORYQUERYRESULTNODE

  virtual nsresult OpenContainer();

  virtual nsresult OpenContainerAsync();
  NS_DECL_ASYNCSTATEMENTCALLBACK

  
  
  
  NS_DECL_NSINAVBOOKMARKOBSERVER

  virtual void OnRemoving();

  
  
  bool mContentsValid;

  
  
  PRInt64 mQueryItemId;

  nsresult FillChildren();
  void ClearChildren(bool aUnregister);
  nsresult Refresh();

  bool StartIncrementalUpdate();
  void ReindexRange(PRInt32 aStartIndex, PRInt32 aEndIndex, PRInt32 aDelta);

  nsNavHistoryResultNode* FindChildById(PRInt64 aItemId,
                                        PRUint32* aNodeIndex);

private:

  nsresult OnChildrenFilled();
  void EnsureRegisteredAsFolderObserver();
  nsresult FillChildrenAsync();

  bool mIsRegisteredFolderObserver;
  PRInt32 mAsyncBookmarkIndex;
};




class nsNavHistorySeparatorResultNode : public nsNavHistoryResultNode
{
public:
  nsNavHistorySeparatorResultNode();

  NS_IMETHOD GetType(PRUint32* type)
    { *type = nsNavHistoryResultNode::RESULT_TYPE_SEPARATOR; return NS_OK; }
};

#endif
