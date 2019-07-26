




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
  HRESULT  RestartCompositionIfNecessary(ITfRange* pRangeNew = nullptr);

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

    
    LONG mStart;

    bool IsComposing() const
    {
      return (mView != nullptr);
    }

    LONG EndOffset() const
    {
      return mStart + static_cast<LONG>(mString.Length());
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

  class Selection
  {
  public:
    Selection() : mDirty(true) {}

    bool IsDirty() const { return mDirty; };
    void MarkDirty() { mDirty = true; }

    TS_SELECTION_ACP& ACP()
    {
      MOZ_ASSERT(!mDirty);
      return mACP;
    }

    void SetSelection(const TS_SELECTION_ACP& aSelection)
    {
      mDirty = false;
      mACP = aSelection;
      
      if (mACP.style.ase != TS_AE_START) {
        mACP.style.ase = TS_AE_END;
      }
      
      
      mACP.style.fInterimChar = FALSE;
    }

    void SetSelection(uint32_t aStart, uint32_t aLength, bool aReversed)
    {
      mDirty = false;
      mACP.acpStart = static_cast<LONG>(aStart);
      mACP.acpEnd = static_cast<LONG>(aStart + aLength);
      mACP.style.ase = aReversed ? TS_AE_START : TS_AE_END;
      mACP.style.fInterimChar = FALSE;
    }

    bool IsCollapsed() const
    {
      MOZ_ASSERT(!mDirty);
      return (mACP.acpStart == mACP.acpEnd);
    }

    void CollapseAt(uint32_t aOffset)
    {
      mDirty = false;
      mACP.acpStart = mACP.acpEnd = static_cast<LONG>(aOffset);
      mACP.style.ase = TS_AE_END;
      mACP.style.fInterimChar = FALSE;
    }

    LONG MinOffset() const
    {
      MOZ_ASSERT(!mDirty);
      LONG min = std::min(mACP.acpStart, mACP.acpEnd);
      MOZ_ASSERT(min >= 0);
      return min;
    }

    LONG MaxOffset() const
    {
      MOZ_ASSERT(!mDirty);
      LONG max = std::max(mACP.acpStart, mACP.acpEnd);
      MOZ_ASSERT(max >= 0);
      return max;
    }

    LONG StartOffset() const
    {
      MOZ_ASSERT(!mDirty);
      MOZ_ASSERT(mACP.acpStart >= 0);
      return mACP.acpStart;
    }

    LONG EndOffset() const
    {
      MOZ_ASSERT(!mDirty);
      MOZ_ASSERT(mACP.acpEnd >= 0);
      return mACP.acpEnd;
    }

    LONG Length() const
    {
      MOZ_ASSERT(!mDirty);
      MOZ_ASSERT(mACP.acpEnd >= mACP.acpStart);
      return std::abs(mACP.acpEnd - mACP.acpStart);
    }

    bool IsReversed() const
    {
      MOZ_ASSERT(!mDirty);
      return (mACP.style.ase == TS_AE_START);
    }

    TsActiveSelEnd ActiveSelEnd() const
    {
      MOZ_ASSERT(!mDirty);
      return mACP.style.ase;
    }

    bool IsInterimChar() const
    {
      MOZ_ASSERT(!mDirty);
      return (mACP.style.fInterimChar != FALSE);
    }

  private:
    TS_SELECTION_ACP mACP;
    bool mDirty;
  };
  
  
  
  Selection mSelection;

  
  
  
  
  
  Selection& CurrentSelection();

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

  
  
  
  class NS_STACK_CLASS AutoPendingActionAndContentFlusher MOZ_FINAL
  {
  public:
    AutoPendingActionAndContentFlusher(nsTextStore* aTextStore)
      : mTextStore(aTextStore)
    {
      MOZ_ASSERT(!mTextStore->mIsRecordingActionsWithoutLock);
      if (!mTextStore->IsReadWriteLocked()) {
        mTextStore->mIsRecordingActionsWithoutLock = true;
      }
    }

    ~AutoPendingActionAndContentFlusher()
    {
      if (!mTextStore->mIsRecordingActionsWithoutLock) {
        return;
      }
      mTextStore->FlushPendingActions();
      mTextStore->mIsRecordingActionsWithoutLock = false;
    }

  private:
    AutoPendingActionAndContentFlusher() {}

    nsRefPtr<nsTextStore> mTextStore;
  };

  class Content MOZ_FINAL
  {
  public:
    Content(nsTextStore::Composition& aComposition,
            nsTextStore::Selection& aSelection) :
      mComposition(aComposition), mSelection(aSelection)
    {
      Clear();
    }

    void Clear()
    {
      mText.Truncate();
      mInitialized = false;
    }

    bool IsInitialized() const { return mInitialized; }

    void Init(const nsAString& aText)
    {
      mText = aText;
      mMinTextModifiedOffset = NOT_MODIFIED;
      mInitialized = true;
      mNotifyTSFOfLayoutChange = false;
    }

    const nsDependentSubstring GetSelectedText() const;
    const nsDependentSubstring GetSubstring(uint32_t aStart,
                                            uint32_t aLength) const;
    void ReplaceSelectedTextWith(const nsAString& aString);
    void ReplaceTextWith(LONG aStart, LONG aLength, const nsAString& aString);

    void StartComposition(ITfCompositionView* aCompositionView,
                          const PendingAction& aCompStart,
                          bool aPreserveSelection);
    void EndComposition(const PendingAction& aCompEnd);

    const nsString& Text() const
    {
      MOZ_ASSERT(mInitialized);
      return mText;
    }

    
    
    bool IsLayoutChangedAfter(uint32_t aOffset) const
    {
      return mInitialized && (mMinTextModifiedOffset < aOffset);
    }
    
    
    bool IsLayoutChanged() const
    {
      return mInitialized && (mMinTextModifiedOffset != NOT_MODIFIED);
    }

    void NeedsToNotifyTSFOfLayoutChange()
    {
      mNotifyTSFOfLayoutChange = true;
    }

    bool NeedToNotifyTSFOfLayoutChange() const
    {
      return mInitialized && mNotifyTSFOfLayoutChange;
    }

    nsTextStore::Composition& Composition() { return mComposition; }
    nsTextStore::Selection& Selection() { return mSelection; }

  private:
    nsString mText;
    nsTextStore::Composition& mComposition;
    nsTextStore::Selection& mSelection;

    
    enum MOZ_ENUM_TYPE(uint32_t)
    {
      NOT_MODIFIED = UINT32_MAX
    };
    uint32_t mMinTextModifiedOffset;

    bool mInitialized;
    bool mNotifyTSFOfLayoutChange;
  };
  
  
  
  
  
  
  Content mContent;

  Content& CurrentContent();

  
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
