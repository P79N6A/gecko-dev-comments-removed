





































#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsCOMArray.h"
#include "nsIFrameLoader.h"
#include "nsIFrame.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIScrollableFrame.h"
#include "nsFocusManager.h"
#include "nsIDocument.h"
#include "nsEventStates.h"

class nsIPresShell;
class nsIDocShell;
class nsIDocShellTreeNode;
class nsIDocShellTreeItem;
class imgIContainer;
class nsDOMDataTransfer;

namespace mozilla {
namespace dom {
class TabParent;
}
}





class nsEventStateManager : public nsSupportsWeakReference,
                            public nsIObserver
{
  friend class nsMouseWheelTransaction;
public:
  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult Shutdown();

  






  nsresult PreHandleEvent(nsPresContext* aPresContext,
                          nsEvent *aEvent,
                          nsIFrame* aTargetFrame,
                          nsEventStatus* aStatus,
                          nsIView* aView);

  




  nsresult PostHandleEvent(nsPresContext* aPresContext,
                           nsEvent *aEvent,
                           nsIFrame* aTargetFrame,
                           nsEventStatus* aStatus,
                           nsIView* aView);

  void NotifyDestroyPresContext(nsPresContext* aPresContext);
  void SetPresContext(nsPresContext* aPresContext);
  void ClearFrameRefs(nsIFrame* aFrame);

  nsIFrame* GetEventTarget();
  already_AddRefed<nsIContent> GetEventTargetContent(nsEvent* aEvent);

  










  PRBool SetContentState(nsIContent *aContent, nsEventStates aState);
  void ContentRemoved(nsIDocument* aDocument, nsIContent* aContent);
  PRBool EventStatusOK(nsGUIEvent* aEvent);

  






  void RegisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  





  void UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  





  PRUint32 GetRegisteredAccessKey(nsIContent* aContent);

  PRBool GetAccessKeyLabelPrefix(nsAString& aPrefix);

  nsresult SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                     PRBool aHaveHotspot, float aHotspotX, float aHotspotY,
                     nsIWidget* aWidget, PRBool aLockCursor); 

  static void StartHandlingUserInput()
  {
    ++sUserInputEventDepth;
  }

  static void StopHandlingUserInput()
  {
    --sUserInputEventDepth;
  }

  static PRBool IsHandlingUserInput()
  {
    return sUserInputEventDepth > 0;
  }

  






  NS_IMETHOD_(PRBool) IsHandlingUserInputExternal() { return IsHandlingUserInput(); }
  
  nsPresContext* GetPresContext() { return mPresContext; }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEventStateManager,
                                           nsIObserver)

  static nsIDocument* sMouseOverDocument;

  static nsEventStateManager* GetActiveEventStateManager() { return sActiveESM; }

  
  
  static void SetActiveManager(nsEventStateManager* aNewESM,
                               nsIContent* aContent);
protected:
  void UpdateCursor(nsPresContext* aPresContext, nsEvent* aEvent, nsIFrame* aTargetFrame, nsEventStatus* aStatus);
  




  nsIFrame* DispatchMouseEvent(nsGUIEvent* aEvent, PRUint32 aMessage,
                               nsIContent* aTargetContent,
                               nsIContent* aRelatedContent);
  



  void GenerateMouseEnterExit(nsGUIEvent* aEvent);
  



  void NotifyMouseOver(nsGUIEvent* aEvent, nsIContent* aContent);
  








  void NotifyMouseOut(nsGUIEvent* aEvent, nsIContent* aMovingInto);
  void GenerateDragDropEnterExit(nsPresContext* aPresContext, nsGUIEvent* aEvent);
  







  void FireDragEnterOrExit(nsPresContext* aPresContext,
                           nsGUIEvent* aEvent,
                           PRUint32 aMsg,
                           nsIContent* aRelatedTarget,
                           nsIContent* aTargetContent,
                           nsWeakFrame& aTargetFrame);
  



  void UpdateDragDataTransfer(nsDragEvent* dragEvent);

  nsresult SetClickCount(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  void EnsureDocument(nsPresContext* aPresContext);
  void FlushPendingEvents(nsPresContext* aPresContext);

  


  typedef enum {
    eAccessKeyProcessingNormal = 0,
    eAccessKeyProcessingUp,
    eAccessKeyProcessingDown
  } ProcessingAccessKeyState;

  


















  void HandleAccessKey(nsPresContext* aPresContext,
                       nsKeyEvent* aEvent,
                       nsEventStatus* aStatus,
                       nsIDocShellTreeItem* aBubbledFrom,
                       ProcessingAccessKeyState aAccessKeyState,
                       PRInt32 aModifierMask);

  PRBool ExecuteAccessKey(nsTArray<PRUint32>& aAccessCharCodes,
                          PRBool aIsTrustedEvent);

  
  
  

  nsIContent* GetFocusedContent();
  PRBool IsShellVisible(nsIDocShell* aShell);

  
  void SendLineScrollEvent(nsIFrame* aTargetFrame,
                           nsMouseScrollEvent* aEvent,
                           nsPresContext* aPresContext,
                           nsEventStatus* aStatus,
                           PRInt32 aNumLines);
  void SendPixelScrollEvent(nsIFrame* aTargetFrame,
                            nsMouseScrollEvent* aEvent,
                            nsPresContext* aPresContext,
                            nsEventStatus* aStatus);
  







  nsresult DoScrollText(nsIFrame* aTargetFrame,
                        nsMouseScrollEvent* aMouseEvent,
                        nsIScrollableFrame::ScrollUnit aScrollQuantity,
                        PRBool aAllowScrollSpeedOverride,
                        nsQueryContentEvent* aQueryEvent = nsnull);
  void DoScrollHistory(PRInt32 direction);
  void DoScrollZoom(nsIFrame *aTargetFrame, PRInt32 adjustment);
  nsresult GetMarkupDocumentViewer(nsIMarkupDocumentViewer** aMv);
  nsresult ChangeTextSize(PRInt32 change);
  nsresult ChangeFullZoom(PRInt32 change);
  




  PRInt32 ComputeWheelDeltaFor(nsMouseScrollEvent* aMouseEvent);
  







  PRInt32 ComputeWheelActionFor(nsMouseScrollEvent* aMouseEvent,
                                PRBool aUseSystemSettings);
  




  PRInt32 GetWheelActionFor(nsMouseScrollEvent* aMouseEvent);
  




  PRInt32 GetScrollLinesFor(nsMouseScrollEvent* aMouseEvent);
  



  PRBool UseSystemScrollSettingFor(nsMouseScrollEvent* aMouseEvent);
  

  







  void DecideGestureEvent(nsGestureNotifyEvent* aEvent, nsIFrame* targetFrame);

  
  void BeginTrackingDragGesture ( nsPresContext* aPresContext, nsMouseEvent* inDownEvent,
                                  nsIFrame* inDownFrame ) ;
  void StopTrackingDragGesture ( ) ;
  void GenerateDragGesture ( nsPresContext* aPresContext, nsMouseEvent *aEvent ) ;

  










  void DetermineDragTarget(nsPresContext* aPresContext,
                           nsIContent* aSelectionTarget,
                           nsDOMDataTransfer* aDataTransfer,
                           PRBool* aIsSelection,
                           PRBool* aIsInEditor,
                           nsIContent** aTargetNode);

  









  PRBool DoDefaultDragStart(nsPresContext* aPresContext,
                            nsDragEvent* aDragEvent,
                            nsDOMDataTransfer* aDataTransfer,
                            nsIContent* aDragTarget,
                            PRBool aIsSelection);

  PRBool IsTrackingDragGesture ( ) const { return mGestureDownContent != nsnull; }
  





  void FillInEventFromGestureDown(nsMouseEvent* aEvent);

  nsresult DoContentCommandEvent(nsContentCommandEvent* aEvent);
  nsresult DoContentCommandScrollEvent(nsContentCommandEvent* aEvent);

  void DoQueryScrollTargetInfo(nsQueryContentEvent* aEvent,
                               nsIFrame* aTargetFrame);

  PRBool RemoteQueryContentEvent(nsEvent *aEvent);
  mozilla::dom::TabParent *GetCrossProcessTarget();
  PRBool IsTargetCrossProcess(nsGUIEvent *aEvent);

  void DispatchCrossProcessEvent(nsEvent* aEvent, nsIFrameLoader* remote);
  PRBool IsRemoteTarget(nsIContent* target);
  PRBool HandleCrossProcessEvent(nsEvent *aEvent,
                                 nsIFrame* aTargetFrame,
                                 nsEventStatus *aStatus);

private:
  static inline void DoStateChange(mozilla::dom::Element* aElement,
                                   nsEventStates aState, PRBool aAddState);
  static inline void DoStateChange(nsIContent* aContent, nsEventStates aState,
                                   PRBool aAddState);
  static void UpdateAncestorState(nsIContent* aStartNode,
                                  nsIContent* aStopBefore,
                                  nsEventStates aState,
                                  PRBool aAddState);

  PRInt32     mLockCursor;

  nsWeakFrame mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  nsWeakFrame mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverElement;
  nsWeakFrame mLastDragOverFrame;

  
  nsIntPoint mGestureDownPoint; 
  
  nsCOMPtr<nsIContent> mGestureDownContent;
  
  
  
  nsCOMPtr<nsIContent> mGestureDownFrameOwner;
  
  PRPackedBool mGestureDownShift;
  PRPackedBool mGestureDownControl;
  PRPackedBool mGestureDownAlt;
  PRPackedBool mGestureDownMeta;

  nsCOMPtr<nsIContent> mLastLeftMouseDownContent;
  nsCOMPtr<nsIContent> mLastLeftMouseDownContentParent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContentParent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContentParent;

  nsCOMPtr<nsIContent> mActiveContent;
  nsCOMPtr<nsIContent> mHoverContent;
  nsCOMPtr<nsIContent> mDragOverContent;
  nsCOMPtr<nsIContent> mURLTargetContent;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOverEventElement;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOutEventElement;

  nsPresContext* mPresContext;      
  nsCOMPtr<nsIDocument> mDocument;   

  PRUint32 mLClickCount;
  PRUint32 mMClickCount;
  PRUint32 mRClickCount;

  PRPackedBool m_haveShutdown;


public:
  static nsresult UpdateUserActivityTimer(void);
  
  nsCOMArray<nsIContent> mAccessKeys;

  
  PRPackedBool mLastLineScrollConsumedX;
  PRPackedBool mLastLineScrollConsumedY;

  static PRInt32 sUserInputEventDepth;
  
  static PRBool sNormalLMouseEventInProcess;

  static nsEventStateManager* sActiveESM;
  
  static void ClearGlobalActiveContent(nsEventStateManager* aClearer);

  
  PRBool mClickHoldContextMenu;
  nsCOMPtr<nsITimer> mClickHoldTimer;
  void CreateClickHoldTimer ( nsPresContext* aPresContext, nsIFrame* inDownFrame,
                              nsGUIEvent* inMouseDownEvent ) ;
  void KillClickHoldTimer ( ) ;
  void FireContextClick ( ) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
};





class nsAutoHandlingUserInputStatePusher
{
public:
  nsAutoHandlingUserInputStatePusher(PRBool aIsHandlingUserInput,
                                     nsEvent* aEvent,
                                     nsIDocument* aDocument)
    : mIsHandlingUserInput(aIsHandlingUserInput),
      mIsMouseDown(aEvent && aEvent->message == NS_MOUSE_BUTTON_DOWN),
      mResetFMMouseDownState(PR_FALSE)
  {
    if (aIsHandlingUserInput) {
      nsEventStateManager::StartHandlingUserInput();
      if (mIsMouseDown) {
        nsIPresShell::SetCapturingContent(nsnull, 0);
        nsIPresShell::AllowMouseCapture(PR_TRUE);
        if (aDocument && NS_IS_TRUSTED_EVENT(aEvent)) {
          nsFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            fm->SetMouseButtonDownHandlingDocument(aDocument);
            mResetFMMouseDownState = PR_TRUE;
          }
        }
      }
    }
  }

  ~nsAutoHandlingUserInputStatePusher()
  {
    if (mIsHandlingUserInput) {
      nsEventStateManager::StopHandlingUserInput();
      if (mIsMouseDown) {
        nsIPresShell::AllowMouseCapture(PR_FALSE);
        if (mResetFMMouseDownState) {
          nsFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            fm->SetMouseButtonDownHandlingDocument(nsnull);
          }
        }
      }
    }
  }

protected:
  PRBool mIsHandlingUserInput;
  PRBool mIsMouseDown;
  PRBool mResetFMMouseDownState;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nsnull; }
  static void operator delete(void* ) {}
};

#define NS_EVENT_NEEDS_FRAME(event) (!NS_IS_ACTIVATION_EVENT(event))

#endif 
