





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
#include "nsThreadUtils.h"
#include "nsWeakReference.h"

class nsIContent;
class nsINode;
class nsISelection;
class nsPresContext;

namespace mozilla {

class EventStateManager;



class IMEContentObserver final : public nsISelectionListener
                               , public nsStubMutationObserver
                               , public nsIReflowObserver
                               , public nsIScrollObserver
                               , public nsSupportsWeakReference
                               , public nsIEditorObserver
{
public:
  typedef widget::IMENotification::TextChangeData TextChangeData;
  typedef widget::IMENotification::TextChangeDataBase TextChangeDataBase;

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

  
  virtual void ScrollPositionChanged() override;

  bool OnMouseButtonEvent(nsPresContext* aPresContext,
                          WidgetMouseEvent* aMouseEvent);

  void Init(nsIWidget* aWidget, nsPresContext* aPresContext,
            nsIContent* aContent, nsIEditor* aEditor);
  void Destroy();
  




  void DisconnectFromEventStateManager();
  






  bool MaybeReinitialize(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsIContent* aContent,
                         nsIEditor* aEditor);
  bool IsManaging(nsPresContext* aPresContext, nsIContent* aContent);
  bool IsEditorHandlingEventForComposition() const;
  bool KeepAliveDuringDeactive() const
  {
    return mUpdatePreference.WantDuringDeactive();
  }
  nsIWidget* GetWidget() const { return mWidget; }
  nsIEditor* GetEditor() const { return mEditor; }
  void SuppressNotifyingIME() { mSuppressNotifications++; }
  void UnsuppressNotifyingIME()
  {
    if (!mSuppressNotifications || --mSuppressNotifications) {
      return;
    }
    FlushMergeableNotifications();
  }
  nsPresContext* GetPresContext() const;
  nsresult GetSelectionAndRoot(nsISelection** aSelection,
                               nsIContent** aRoot) const;

private:
  ~IMEContentObserver() {}

  enum State {
    eState_NotObserving,
    eState_Initializing,
    eState_StoppedObserving,
    eState_Observing
  };
  State GetState() const;
  bool IsObservingContent(nsPresContext* aPresContext,
                          nsIContent* aContent) const;
  bool IsReflowLocked() const;
  bool IsSafeToNotifyIME() const;

  void PostFocusSetNotification();
  void MaybeNotifyIMEOfFocusSet()
  {
    PostFocusSetNotification();
    FlushMergeableNotifications();
  }
  void PostTextChangeNotification(const TextChangeDataBase& aTextChangeData);
  void MaybeNotifyIMEOfTextChange(const TextChangeDataBase& aTextChangeData)
  {
    PostTextChangeNotification(aTextChangeData);
    FlushMergeableNotifications();
  }
  void PostSelectionChangeNotification(bool aCausedByComposition,
                                       bool aCausedBySelectionEvent);
  void MaybeNotifyIMEOfSelectionChange(bool aCausedByComposition,
                                       bool aCausedBySelectionEvent)
  {
    PostSelectionChangeNotification(aCausedByComposition,
                                    aCausedBySelectionEvent);
    FlushMergeableNotifications();
  }
  void PostPositionChangeNotification();
  void MaybeNotifyIMEOfPositionChange()
  {
    PostPositionChangeNotification();
    FlushMergeableNotifications();
  }

  void NotifyContentAdded(nsINode* aContainer, int32_t aStart, int32_t aEnd);
  void ObserveEditableNode();
  


  void NotifyIMEOfBlur();
  


  void UnregisterObservers();
  void FlushMergeableNotifications();
  void ClearPendingNotifications()
  {
    mIsFocusEventPending = false;
    mIsSelectionChangeEventPending = false;
    mIsPositionChangeEventPending = false;
    mTextChangeData.Clear();
  }

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
  uint32_t mSuppressNotifications;
  int64_t mPreCharacterDataChangeLength;

  bool mIsObserving;
  bool mIMEHasFocus;
  bool mIsFocusEventPending;
  bool mIsSelectionChangeEventPending;
  bool mSelectionChangeCausedOnlyByComposition;
  bool mSelectionChangeCausedOnlyBySelectionEvent;
  bool mIsPositionChangeEventPending;
  bool mIsFlushingPendingNotifications;


  



  class AChangeEvent: public nsRunnable
  {
  protected:
    enum ChangeEventType
    {
      eChangeEventType_Focus,
      eChangeEventType_Selection,
      eChangeEventType_Text,
      eChangeEventType_Position,
      eChangeEventType_FlushPendingEvents
    };

    AChangeEvent(ChangeEventType aChangeEventType,
                 IMEContentObserver* aIMEContentObserver)
      : mIMEContentObserver(aIMEContentObserver)
      , mChangeEventType(aChangeEventType)
    {
      MOZ_ASSERT(mIMEContentObserver);
    }

    nsRefPtr<IMEContentObserver> mIMEContentObserver;
    ChangeEventType mChangeEventType;

    


    bool CanNotifyIME() const;

    


    bool IsSafeToNotifyIME() const;
  };

  class FocusSetEvent: public AChangeEvent
  {
  public:
    explicit FocusSetEvent(IMEContentObserver* aIMEContentObserver)
      : AChangeEvent(eChangeEventType_Focus, aIMEContentObserver)
    {
    }
    NS_IMETHOD Run() override;
  };

  class SelectionChangeEvent : public AChangeEvent
  {
  public:
    SelectionChangeEvent(IMEContentObserver* aIMEContentObserver,
                         bool aCausedByComposition,
                         bool aCausedBySelectionEvent)
      : AChangeEvent(eChangeEventType_Selection, aIMEContentObserver)
      , mCausedByComposition(aCausedByComposition)
      , mCausedBySelectionEvent(aCausedBySelectionEvent)
    {
      aIMEContentObserver->mSelectionChangeCausedOnlyByComposition = false;
      aIMEContentObserver->mSelectionChangeCausedOnlyBySelectionEvent = false;
    }
    NS_IMETHOD Run() override;

  private:
    bool mCausedByComposition;
    bool mCausedBySelectionEvent;
  };

  class TextChangeEvent : public AChangeEvent
  {
  public:
    TextChangeEvent(IMEContentObserver* aIMEContentObserver,
                    TextChangeDataBase& aTextChangeData)
      : AChangeEvent(eChangeEventType_Text, aIMEContentObserver)
      , mTextChangeData(aTextChangeData)
    {
      MOZ_ASSERT(mTextChangeData.IsValid());
      
      aTextChangeData.Clear();
    }
    NS_IMETHOD Run() override;

  private:
    TextChangeDataBase mTextChangeData;
  };

  class PositionChangeEvent final : public AChangeEvent
  {
  public:
    explicit PositionChangeEvent(IMEContentObserver* aIMEContentObserver)
      : AChangeEvent(eChangeEventType_Position, aIMEContentObserver)
    {
    }
    NS_IMETHOD Run() override;
  };

  class AsyncMergeableNotificationsFlusher : public AChangeEvent
  {
  public:
    explicit AsyncMergeableNotificationsFlusher(
      IMEContentObserver* aIMEContentObserver)
      : AChangeEvent(eChangeEventType_FlushPendingEvents, aIMEContentObserver)
    {
    }
    NS_IMETHOD Run() override;
  };
};

} 

#endif 
