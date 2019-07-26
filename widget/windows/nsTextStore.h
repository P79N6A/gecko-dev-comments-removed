




#ifndef NSTEXTSTORE_H_
#define NSTEXTSTORE_H_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIWidget.h"
#include "nsWindowBase.h"
#include "mozilla/Attributes.h"

#include <msctf.h>
#include <textstor.h>



#ifdef INPUTSCOPE_INIT_GUID
#include <initguid.h>
#endif
#include <inputscope.h>

struct ITfThreadMgr;
struct ITfDocumentMgr;
struct ITfDisplayAttributeMgr;
struct ITfCategoryMgr;
class nsWindow;
class nsTextEvent;
#ifdef MOZ_METRO
class MetroWidget;
#endif




#define WM_USER_TSF_TEXTCHANGE  (WM_USER + 0x100)





class nsTextStore MOZ_FINAL : public ITextStoreACP,
                              public ITfContextOwnerCompositionSink
{
public: 
  STDMETHODIMP_(ULONG)  AddRef(void);
  STDMETHODIMP          QueryInterface(REFIID, void**);
  STDMETHODIMP_(ULONG)  Release(void);

public: 
  STDMETHODIMP AdviseSink(REFIID, IUnknown*, DWORD);
  STDMETHODIMP UnadviseSink(IUnknown*);
  STDMETHODIMP RequestLock(DWORD, HRESULT*);
  STDMETHODIMP GetStatus(TS_STATUS*);
  STDMETHODIMP QueryInsert(LONG, LONG, ULONG, LONG*, LONG*);
  STDMETHODIMP GetSelection(ULONG, ULONG, TS_SELECTION_ACP*, ULONG*);
  STDMETHODIMP SetSelection(ULONG, const TS_SELECTION_ACP*);
  STDMETHODIMP GetText(LONG, LONG, WCHAR*, ULONG, ULONG*, TS_RUNINFO*, ULONG,
                       ULONG*, LONG*);
  STDMETHODIMP SetText(DWORD, LONG, LONG, const WCHAR*, ULONG, TS_TEXTCHANGE*);
  STDMETHODIMP GetFormattedText(LONG, LONG, IDataObject**);
  STDMETHODIMP GetEmbedded(LONG, REFGUID, REFIID, IUnknown**);
  STDMETHODIMP QueryInsertEmbedded(const GUID*, const FORMATETC*, BOOL*);
  STDMETHODIMP InsertEmbedded(DWORD, LONG, LONG, IDataObject*, TS_TEXTCHANGE*);
  STDMETHODIMP RequestSupportedAttrs(DWORD, ULONG, const TS_ATTRID*);
  STDMETHODIMP RequestAttrsAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD);
  STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG, ULONG,
                                                   const TS_ATTRID*, DWORD);
  STDMETHODIMP FindNextAttrTransition(LONG, LONG, ULONG, const TS_ATTRID*,
                                      DWORD, LONG*, BOOL*, LONG*);
  STDMETHODIMP RetrieveRequestedAttrs(ULONG, TS_ATTRVAL*, ULONG*);
  STDMETHODIMP GetEndACP(LONG*);
  STDMETHODIMP GetActiveView(TsViewCookie*);
  STDMETHODIMP GetACPFromPoint(TsViewCookie, const POINT*, DWORD, LONG*);
  STDMETHODIMP GetTextExt(TsViewCookie, LONG, LONG, RECT*, BOOL*);
  STDMETHODIMP GetScreenExt(TsViewCookie, RECT*);
  STDMETHODIMP GetWnd(TsViewCookie, HWND*);
  STDMETHODIMP InsertTextAtSelection(DWORD, const WCHAR*, ULONG, LONG*, LONG*,
                                     TS_TEXTCHANGE*);
  STDMETHODIMP InsertEmbeddedAtSelection(DWORD, IDataObject*, LONG*, LONG*,
                                         TS_TEXTCHANGE*);

public: 
  STDMETHODIMP OnStartComposition(ITfCompositionView*, BOOL*);
  STDMETHODIMP OnUpdateComposition(ITfCompositionView*, ITfRange*);
  STDMETHODIMP OnEndComposition(ITfCompositionView*);

protected:
  typedef mozilla::widget::IMEState IMEState;
  typedef mozilla::widget::InputContext InputContext;

public:
  static void     Initialize(void);
  static void     Terminate(void);
  static void     SetIMEOpenState(bool);
  static bool     GetIMEOpenState(void);

  static void     CommitComposition(bool aDiscard)
  {
    NS_ENSURE_TRUE_VOID(sTsfTextStore);
    sTsfTextStore->CommitCompositionInternal(aDiscard);
  }

  static void     SetInputContext(const InputContext& aContext)
  {
    NS_ENSURE_TRUE_VOID(sTsfTextStore);
    sTsfTextStore->SetInputScope(aContext.mHTMLInputType);
    sTsfTextStore->SetInputContextInternal(aContext.mIMEState.mEnabled);
  }

  static nsresult OnFocusChange(bool aGotFocus,
                                nsWindowBase* aFocusedWidget,
                                IMEState::Enabled aIMEEnabled);
  static nsresult OnTextChange(uint32_t aStart,
                               uint32_t aOldEnd,
                               uint32_t aNewEnd)
  {
    NS_ENSURE_TRUE(sTsfTextStore, NS_ERROR_NOT_AVAILABLE);
    return sTsfTextStore->OnTextChangeInternal(aStart, aOldEnd, aNewEnd);
  }

  static void     OnTextChangeMsg(void)
  {
    NS_ENSURE_TRUE_VOID(sTsfTextStore);
    
    
    sTsfTextStore->OnTextChangeMsgInternal();
  }

  static nsresult OnSelectionChange(void)
  {
    NS_ENSURE_TRUE(sTsfTextStore, NS_ERROR_NOT_AVAILABLE);
    return sTsfTextStore->OnSelectionChangeInternal();
  }

  static nsIMEUpdatePreference GetIMEUpdatePreference();

  static bool CanOptimizeKeyAndIMEMessages()
  {
    
    return true;
  }

  
  
  static void* GetNativeData(uint32_t aDataType)
  {
    switch (aDataType) {
      case NS_NATIVE_TSF_THREAD_MGR:
        Initialize(); 
        return static_cast<void*>(&sTsfThreadMgr);
      case NS_NATIVE_TSF_CATEGORY_MGR:
        return static_cast<void*>(&sCategoryMgr);
      case NS_NATIVE_TSF_DISPLAY_ATTR_MGR:
        return static_cast<void*>(&sDisplayAttrMgr);
      default:
        return nullptr;
    }
  }

  static void*    GetTextStore()
  {
    return static_cast<void*>(sTsfTextStore);
  }

  static bool     ThinksHavingFocus()
  {
    return (sTsfTextStore && sTsfTextStore->mContext);
  }

  static bool     IsInTSFMode()
  {
    return sTsfThreadMgr != nullptr;
  }

  static bool     IsComposing()
  {
    return (sTsfTextStore && sTsfTextStore->mComposition.IsComposing());
  }

  static bool     IsComposingOn(nsWindowBase* aWidget)
  {
    return (IsComposing() && sTsfTextStore->mWidget == aWidget);
  }

#ifdef DEBUG
  
  static bool     CurrentKeyboardLayoutHasIME();
#endif 

protected:
  nsTextStore();
  ~nsTextStore();

  bool     Create(nsWindowBase* aWidget,
                  IMEState::Enabled aIMEEnabled);
  bool     Destroy(void);

  bool     IsReadLock(DWORD aLock) const
  {
    return (TS_LF_READ == (aLock & TS_LF_READ));
  }
  bool     IsReadWriteLock(DWORD aLock) const
  {
    return (TS_LF_READWRITE == (aLock & TS_LF_READWRITE));
  }
  bool     IsReadLocked() const { return IsReadLock(mLock); }
  bool     IsReadWriteLocked() const { return IsReadWriteLock(mLock); }

  bool     GetScreenExtInternal(RECT &aScreenExt);
  bool     GetSelectionInternal(TS_SELECTION_ACP &aSelectionACP);
  
  
  
  
  HRESULT  SetSelectionInternal(const TS_SELECTION_ACP*,
                                bool aDispatchTextEvent = false);
  bool     InsertTextAtSelectionInternal(const nsAString &aInsertStr,
                                         TS_TEXTCHANGE* aTextChange);
  HRESULT  OnStartCompositionInternal(ITfCompositionView*, ITfRange*, bool);
  void     CommitCompositionInternal(bool);
  void     SetInputContextInternal(IMEState::Enabled aState);
  nsresult OnTextChangeInternal(uint32_t, uint32_t, uint32_t);
  void     OnTextChangeMsgInternal(void);
  nsresult OnSelectionChangeInternal(void);
  HRESULT  GetDisplayAttribute(ITfProperty* aProperty,
                               ITfRange* aRange,
                               TF_DISPLAYATTRIBUTE* aResult);
  HRESULT  UpdateCompositionExtent(ITfRange* pRangeNew);

  HRESULT  RecordCompositionUpdateAction();
  void     FlushPendingActions();

  nsresult OnLayoutChange();
  HRESULT  ProcessScopeRequest(DWORD dwFlags,
                               ULONG cFilterAttrs,
                               const TS_ATTRID *paFilterAttrs);
  void     SetInputScope(const nsString& aHTMLInputType);

  
  nsRefPtr<nsWindowBase>       mWidget;
  
  nsRefPtr<ITfDocumentMgr>     mDocumentMgr;
  
  DWORD                        mEditCookie;
  
  nsRefPtr<ITfContext>         mContext;
  
  nsRefPtr<ITextStoreACPSink>  mSink;
  
  DWORD                        mSinkMask;
  
  DWORD                        mLock;
  
  DWORD                        mLockQueued;
  
  TS_TEXTCHANGE                mTextChange;

  class Composition MOZ_FINAL
  {
  public:
    
    nsRefPtr<ITfCompositionView> mView;

    
    
    
    
    
    
    nsString mString;

    
    
    nsString mLastData;

    
    
    
    
    
    TS_SELECTION_ACP mSelection;

    
    LONG mStart;
    LONG mLength;

    bool IsComposing() const
    {
      return (mView != nullptr);
    }

    bool IsSelectionCollapsed() const
    {
      return (mSelection.acpStart == mSelection.acpEnd);
    }

    LONG MinSelectionOffset() const
    {
      return std::min(mSelection.acpStart, mSelection.acpEnd);
    }

    LONG MaxSelectionOffset() const
    {
      return std::max(mSelection.acpStart, mSelection.acpEnd);
    }

    LONG StringEndOffset() const
    {
      return mStart + static_cast<LONG>(mString.Length());
    }

    LONG EndOffset() const
    {
      return mStart + mLength;
    }

    
    
    
    void Start(ITfCompositionView* aCompositionView,
               LONG aCompositionStartOffset,
               const nsAString& aCompositionString);
    void End();

    void StartLayoutChangeTimer(nsTextStore* aTextStore);
    void EnsureLayoutChangeTimerStopped();

  private:
    
    
    nsCOMPtr<nsITimer> mLayoutChangeTimer;

    static void TimerCallback(nsITimer* aTimer, void *aClosure);
    static uint32_t GetLayoutChangeIntervalTime();
  };
  
  
  
  
  
  
  Composition mComposition;

  struct PendingAction MOZ_FINAL
  {
    enum ActionType MOZ_ENUM_TYPE(uint8_t)
    {
      COMPOSITION_START,
      COMPOSITION_UPDATE,
      COMPOSITION_END,
      SELECTION_SET
    };
    ActionType mType;
    
    LONG mSelectionStart;
    LONG mSelectionLength;
    
    nsString mData;
    
    nsTArray<nsTextRange> mRanges;
    
    bool mSelectionReversed;
  };
  
  
  
  
  nsTArray<PendingAction> mPendingActions;

  PendingAction* GetPendingCompositionUpdate()
  {
    if (!mPendingActions.IsEmpty()) {
      PendingAction& lastAction = mPendingActions.LastElement();
      if (lastAction.mType == PendingAction::COMPOSITION_UPDATE) {
        return &lastAction;
      }
    }
    PendingAction* newAction = mPendingActions.AppendElement();
    newAction->mType = PendingAction::COMPOSITION_UPDATE;
    
    
    newAction->mRanges.SetCapacity(4);
    return newAction;
  }

  
  
  
  class NS_STACK_CLASS AutoPendingActionFlusher MOZ_FINAL
  {
  public:
    AutoPendingActionFlusher(nsTextStore* aTextStore)
      : mTextStore(aTextStore)
    {
      MOZ_ASSERT(!mTextStore->mIsRecordingActionsWithoutLock);
      if (!mTextStore->IsReadWriteLocked()) {
        mTextStore->mIsRecordingActionsWithoutLock = true;
      }
    }

    ~AutoPendingActionFlusher()
    {
      if (!mTextStore->mIsRecordingActionsWithoutLock) {
        return;
      }
      mTextStore->FlushPendingActions();
      mTextStore->mIsRecordingActionsWithoutLock = false;
    }

  private:
    AutoPendingActionFlusher() {}

    nsRefPtr<nsTextStore> mTextStore;
  };

  
  nsTArray<InputScope>         mInputScopes;
  bool                         mInputScopeDetected;
  bool                         mInputScopeRequested;
  
  
  bool                         mIsRecordingActionsWithoutLock;
  
  
  
  
  
  
  
  bool                         mNotifySelectionChange;

  
  static ITfThreadMgr*  sTsfThreadMgr;
  
  static ITfDisplayAttributeMgr* sDisplayAttrMgr;
  
  static ITfCategoryMgr* sCategoryMgr;

  
  static DWORD          sTsfClientId;
  
  
  
  static nsTextStore*   sTsfTextStore;

  
  
  static UINT           sFlushTIPInputMessage;

private:
  ULONG                       mRefCnt;
};

#endif
