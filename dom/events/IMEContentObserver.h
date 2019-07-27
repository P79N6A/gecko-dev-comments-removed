





#ifndef mozilla_IMEContentObserver_h_
#define mozilla_IMEContentObserver_h_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocShell.h" 
#include "nsIEditor.h"
#include "nsIEditorObserver.h"
#include "nsIReflowObserver.h"
#include "nsISelectionListener.h"
#include "nsIScrollObserver.h"
#include "nsIWidget.h" 
#include "nsStubMutationObserver.h"
#include "nsWeakReference.h"

class nsIContent;
class nsINode;
class nsISelection;
class nsPresContext;

namespace mozilla {

class EventStateManager;



class IMEContentObserver MOZ_FINAL : public nsISelectionListener
                                   , public nsStubMutationObserver
                                   , public nsIReflowObserver
                                   , public nsIScrollObserver
                                   , public nsSupportsWeakReference
                                   , public nsIEditorObserver
{
  friend class AsyncMergeableNotificationsFlusher;

public:
  IMEContentObserver();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(IMEContentObserver,
                                           nsISelectionListener)
  NS_DECL_NSIEDITOROBSERVER
  NS_DECL_NSISELECTIONLISTENER
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATAWILLCHANGE
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIREFLOWOBSERVER

  
  virtual void ScrollPositionChanged() MOZ_OVERRIDE;

  bool OnMouseButtonEvent(nsPresContext* aPresContext,
                          WidgetMouseEvent* aMouseEvent);

  void Init(nsIWidget* aWidget, nsPresContext* aPresContext,
            nsIContent* aContent);
  void Destroy();
  




  void DisconnectFromEventStateManager();
  bool IsManaging(nsPresContext* aPresContext, nsIContent* aContent);
  bool IsEditorHandlingEventForComposition() const;
  bool KeepAliveDuringDeactive() const
  {
    return mUpdatePreference.WantDuringDeactive();
  }
  nsIWidget* GetWidget() const { return mWidget; }
  nsresult GetSelectionAndRoot(nsISelection** aSelection,
                               nsIContent** aRoot) const;

  struct TextChangeData
  {
    
    
    uint32_t mStartOffset;
    
    
    
    uint32_t mRemovedEndOffset;
    
    
    uint32_t mAddedEndOffset;

    bool mCausedOnlyByComposition;
    bool mStored;

    TextChangeData()
      : mStartOffset(0)
      , mRemovedEndOffset(0)
      , mAddedEndOffset(0)
      , mCausedOnlyByComposition(false)
      , mStored(false)
    {
    }

    TextChangeData(uint32_t aStartOffset,
                   uint32_t aRemovedEndOffset,
                   uint32_t aAddedEndOffset,
                   bool aCausedByComposition)
      : mStartOffset(aStartOffset)
      , mRemovedEndOffset(aRemovedEndOffset)
      , mAddedEndOffset(aAddedEndOffset)
      , mCausedOnlyByComposition(aCausedByComposition)
      , mStored(true)
    {
      MOZ_ASSERT(aRemovedEndOffset >= aStartOffset,
                 "removed end offset must not be smaller than start offset");
      MOZ_ASSERT(aAddedEndOffset >= aStartOffset,
                 "added end offset must not be smaller than start offset");
    }
    
    int64_t Difference() const 
    {
      return mAddedEndOffset - mRemovedEndOffset;
    }
  };

private:
  ~IMEContentObserver() {}

  void MaybeNotifyIMEOfTextChange(const TextChangeData& aTextChangeData);
  void MaybeNotifyIMEOfSelectionChange(bool aCausedByComposition);
  void MaybeNotifyIMEOfPositionChange();

  void NotifyContentAdded(nsINode* aContainer, int32_t aStart, int32_t aEnd);
  void ObserveEditableNode();
  




  void UnregisterObservers(bool aPostEvent);
  void StoreTextChangeData(const TextChangeData& aTextChangeData);
  void FlushMergeableNotifications();

#ifdef DEBUG
  void TestMergingTextChangeData();
#endif

  nsCOMPtr<nsIWidget> mWidget;
  nsCOMPtr<nsISelection> mSelection;
  nsCOMPtr<nsIContent> mRootContent;
  nsCOMPtr<nsINode> mEditableNode;
  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIEditor> mEditor;

  



  struct FlatTextCache
  {
    
    
    nsCOMPtr<nsINode> mContainerNode;
    int32_t mNodeOffset;
    
    
    uint32_t mFlatTextLength;

    FlatTextCache()
      : mNodeOffset(0)
      , mFlatTextLength(0)
    {
    }

    void Clear()
    {
      mContainerNode = nullptr;
      mNodeOffset = 0;
      mFlatTextLength = 0;
    }

    void Cache(nsINode* aContainer, int32_t aNodeOffset,
               uint32_t aFlatTextLength)
    {
      MOZ_ASSERT(aContainer, "aContainer must not be null");
      MOZ_ASSERT(
        aNodeOffset <= static_cast<int32_t>(aContainer->GetChildCount()),
        "aNodeOffset must be same as or less than the count of children");
      mContainerNode = aContainer;
      mNodeOffset = aNodeOffset;
      mFlatTextLength = aFlatTextLength;
    }

    bool Match(nsINode* aContainer, int32_t aNodeOffset) const
    {
      return aContainer == mContainerNode && aNodeOffset == mNodeOffset;
    }
  };
  
  
  
  
  FlatTextCache mEndOfAddedTextCache;
  
  
  
  FlatTextCache mStartOfRemovingTextRangeCache;

  TextChangeData mTextChangeData;

  EventStateManager* mESM;

  nsIMEUpdatePreference mUpdatePreference;
  uint32_t mPreAttrChangeLength;
  int64_t mPreCharacterDataChangeLength;

  bool mIsSelectionChangeEventPending;
  bool mSelectionChangeCausedOnlyByComposition;
  bool mIsPositionChangeEventPending;
  bool mIsFlushingPendingNotifications;
};

} 

#endif 
