




#ifndef NSTEXTSTORE_H_
#define NSTEXTSTORE_H_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIWidget.h"
#include "nsWindowBase.h"
#include "WinUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/TextRange.h"
#include "mozilla/WindowsVersion.h"

#include <msctf.h>
#include <textstor.h>



#ifdef INPUTSCOPE_INIT_GUID
#include <initguid.h>
#endif
#ifdef TEXTATTRS_INIT_GUID
#include <tsattrs.h>
#endif
#include <inputscope.h>


#define IS_SEARCH static_cast<InputScope>(50)

struct ITfThreadMgr;
struct ITfDocumentMgr;
struct ITfDisplayAttributeMgr;
struct ITfCategoryMgr;
class nsWindow;

namespace mozilla {
namespace widget {
struct MSGResult;
} 
} 





class nsTextStore final : public ITextStoreACP
                        , public ITfContextOwnerCompositionSink
                        , public ITfMouseTrackerACP
{
public: 
  STDMETHODIMP          QueryInterface(REFIID, void**);

  NS_INLINE_DECL_IUNKNOWN_REFCOUNTING(nsTextStore)

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

public: 
  STDMETHODIMP AdviseMouseSink(ITfRangeACP*, ITfMouseSink*, DWORD*);
  STDMETHODIMP UnadviseMouseSink(DWORD);

protected:
  typedef mozilla::widget::IMENotification IMENotification;
  typedef mozilla::widget::IMEState IMEState;
  typedef mozilla::widget::InputContext InputContext;
  typedef mozilla::widget::InputContextAction InputContextAction;

public:
  static void     Initialize(void);
  static void     Terminate(void);

  static bool     ProcessRawKeyMessage(const MSG& aMsg);
  static void     ProcessMessage(nsWindowBase* aWindow, UINT aMessage,
                                 WPARAM& aWParam, LPARAM& aLParam,
                                 mozilla::widget::MSGResult& aResult);


  static void     SetIMEOpenState(bool);
  static bool     GetIMEOpenState(void);

  static void     CommitComposition(bool aDiscard)
  {
    NS_ASSERTION(IsInTSFMode(), "Not in TSF mode, shouldn't be called");
    if (sEnabledTextStore) {
      sEnabledTextStore->CommitCompositionInternal(aDiscard);
    }
  }

  static void SetInputContext(nsWindowBase* aWidget,
                              const InputContext& aContext,
                              const InputContextAction& aAction);

  static nsresult OnFocusChange(bool aGotFocus,
                                nsWindowBase* aFocusedWidget,
                                const InputContext& aContext);
  static nsresult OnTextChange(const IMENotification& aIMENotification)
  {
    NS_ASSERTION(IsInTSFMode(), "Not in TSF mode, shouldn't be called");
    return sEnabledTextStore ?
      sEnabledTextStore->OnTextChangeInternal(aIMENotification) : NS_OK;
  }

  static nsresult OnSelectionChange(void)
  {
    NS_ASSERTION(IsInTSFMode(), "Not in TSF mode, shouldn't be called");
    return sEnabledTextStore ?
      sEnabledTextStore->OnSelectionChangeInternal() : NS_OK;
  }

  static nsresult OnLayoutChange()
  {
    NS_ASSERTION(IsInTSFMode(), "Not in TSF mode, shouldn't be called");
    return sEnabledTextStore ?
      sEnabledTextStore->OnLayoutChangeInternal() : NS_OK;
  }

  static nsresult OnMouseButtonEvent(const IMENotification& aIMENotification)
  {
    NS_ASSERTION(IsInTSFMode(), "Not in TSF mode, shouldn't be called");
    return sEnabledTextStore ?
      sEnabledTextStore->OnMouseButtonEventInternal(aIMENotification) : NS_OK;
  }

  static nsIMEUpdatePreference GetIMEUpdatePreference();

  
  
  
  static void* GetNativeData(uint32_t aDataType)
  {
    switch (aDataType) {
      case NS_NATIVE_TSF_THREAD_MGR:
        Initialize(); 
        return static_cast<void*>(&sThreadMgr);
      case NS_NATIVE_TSF_CATEGORY_MGR:
        return static_cast<void*>(&sCategoryMgr);
      case NS_NATIVE_TSF_DISPLAY_ATTR_MGR:
        return static_cast<void*>(&sDisplayAttrMgr);
      default:
        return nullptr;
    }
  }

  static ITfMessagePump* GetMessagePump()
  {
    return sMessagePump;
  }

  static void* GetThreadManager()
  {
    return static_cast<void*>(sThreadMgr);
  }

  static bool ThinksHavingFocus()
  {
    return (sEnabledTextStore && sEnabledTextStore->mContext);
  }

  static bool IsInTSFMode()
  {
    return sThreadMgr != nullptr;
  }

  static bool IsComposing()
  {
    return (sEnabledTextStore && sEnabledTextStore->mComposition.IsComposing());
  }

  static bool IsComposingOn(nsWindowBase* aWidget)
  {
    return (IsComposing() && sEnabledTextStore->mWidget == aWidget);
  }

  static bool IsIMM_IME();

#ifdef DEBUG
  
  static bool     CurrentKeyboardLayoutHasIME();
#endif 

protected:
  nsTextStore();
  ~nsTextStore();

  static bool CreateAndSetFocus(nsWindowBase* aFocusedWidget,
                                const InputContext& aContext);
  static void MarkContextAsKeyboardDisabled(ITfContext* aContext);
  static void MarkContextAsEmpty(ITfContext* aContext);

  bool     Init(nsWindowBase* aWidget);
  bool     Destroy();

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

  
  
  void     DidLockGranted();

  bool     GetScreenExtInternal(RECT &aScreenExt);
  
  
  
  
  
  HRESULT  SetSelectionInternal(const TS_SELECTION_ACP*,
                                bool aDispatchCompositionChangeEvent = false);
  bool     InsertTextAtSelectionInternal(const nsAString &aInsertStr,
                                         TS_TEXTCHANGE* aTextChange);
  void     CommitCompositionInternal(bool);
  nsresult OnTextChangeInternal(const IMENotification& aIMENotification);
  nsresult OnSelectionChangeInternal(void);
  nsresult OnMouseButtonEventInternal(const IMENotification& aIMENotification);
  HRESULT  GetDisplayAttribute(ITfProperty* aProperty,
                               ITfRange* aRange,
                               TF_DISPLAYATTRIBUTE* aResult);
  HRESULT  RestartCompositionIfNecessary(ITfRange* pRangeNew = nullptr);
  HRESULT  RestartComposition(ITfCompositionView* aCompositionView,
                              ITfRange* aNewRange);

  
  
  HRESULT  RecordCompositionStartAction(ITfCompositionView* aCompositionView,
                                        ITfRange* aRange,
                                        bool aPreserveSelection);
  HRESULT  RecordCompositionStartAction(ITfCompositionView* aComposition,
                                        LONG aStart,
                                        LONG aLength,
                                        bool aPreserveSelection);
  HRESULT  RecordCompositionUpdateAction();
  HRESULT  RecordCompositionEndAction();

  
  
  void     FlushPendingActions();

  nsresult OnLayoutChangeInternal();
  HRESULT  HandleRequestAttrs(DWORD aFlags,
                              ULONG aFilterCount,
                              const TS_ATTRID* aFilterAttrs);
  void     SetInputScope(const nsString& aHTMLInputType);

  
  
  void     CreateNativeCaret();

  
  nsRefPtr<nsWindowBase>       mWidget;
  
  nsRefPtr<ITfDocumentMgr>     mDocumentMgr;
  
  DWORD                        mEditCookie;
  
  nsRefPtr<ITfContext>         mContext;
  
  nsRefPtr<ITextStoreACPSink>  mSink;
  
  DWORD                        mSinkMask;
  
  DWORD                        mLock;
  
  DWORD                        mLockQueued;

  class Composition final
  {
  public:
    
    nsRefPtr<ITfCompositionView> mView;

    
    
    
    
    
    
    nsString mString;

    
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

    void SetSelection(uint32_t aStart, uint32_t aLength, bool aReversed,
                      mozilla::WritingMode aWritingMode)
    {
      mDirty = false;
      mACP.acpStart = static_cast<LONG>(aStart);
      mACP.acpEnd = static_cast<LONG>(aStart + aLength);
      mACP.style.ase = aReversed ? TS_AE_START : TS_AE_END;
      mACP.style.fInterimChar = FALSE;
      mWritingMode = aWritingMode;
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

    mozilla::WritingMode GetWritingMode() const
    {
      MOZ_ASSERT(!mDirty);
      return mWritingMode;
    }

  private:
    TS_SELECTION_ACP mACP;
    mozilla::WritingMode mWritingMode;
    bool mDirty;
  };
  
  
  
  Selection mSelection;

  
  
  
  
  
  Selection& CurrentSelection();

  struct PendingAction final
  {
    enum ActionType : uint8_t
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
    
    nsRefPtr<mozilla::TextRangeArray> mRanges;
    
    bool mSelectionReversed;
    
    bool mIncomplete;
    
    bool mAdjustSelection;
  };
  
  
  
  
  nsTArray<PendingAction> mPendingActions;

  PendingAction* LastOrNewPendingCompositionUpdate()
  {
    if (!mPendingActions.IsEmpty()) {
      PendingAction& lastAction = mPendingActions.LastElement();
      if (lastAction.mType == PendingAction::COMPOSITION_UPDATE) {
        return &lastAction;
      }
    }
    PendingAction* newAction = mPendingActions.AppendElement();
    newAction->mType = PendingAction::COMPOSITION_UPDATE;
    newAction->mRanges = new mozilla::TextRangeArray();
    newAction->mIncomplete = true;
    return newAction;
  }

  bool IsPendingCompositionUpdateIncomplete() const
  {
    if (mPendingActions.IsEmpty()) {
      return false;
    }
    const PendingAction& lastAction = mPendingActions.LastElement();
    return lastAction.mType == PendingAction::COMPOSITION_UPDATE &&
           lastAction.mIncomplete;
  }

  void CompleteLastActionIfStillIncomplete()
  {
    if (!IsPendingCompositionUpdateIncomplete()) {
      return;
    }
    RecordCompositionUpdateAction();
  }

  
  
  
  class MOZ_STACK_CLASS AutoPendingActionAndContentFlusher final
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

  class Content final
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
      mLastCompositionString.Truncate();
      mInitialized = false;
    }

    bool IsInitialized() const { return mInitialized; }

    void Init(const nsAString& aText)
    {
      mText = aText;
      if (mComposition.IsComposing()) {
        mLastCompositionString = mComposition.mString;
      }
      mMinTextModifiedOffset = NOT_MODIFIED;
      mInitialized = true;
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
    
    uint32_t MinOffsetOfLayoutChanged() const
    {
      return mInitialized ? mMinTextModifiedOffset : NOT_MODIFIED;
    }

    nsTextStore::Composition& Composition() { return mComposition; }
    nsTextStore::Selection& Selection() { return mSelection; }

  private:
    nsString mText;
    
    
    nsString mLastCompositionString;
    nsTextStore::Composition& mComposition;
    nsTextStore::Selection& mSelection;

    
    enum : uint32_t
    {
      NOT_MODIFIED = UINT32_MAX
    };
    uint32_t mMinTextModifiedOffset;

    bool mInitialized;
  };
  
  
  
  
  
  
  Content mLockedContent;

  Content& LockedContent();

  
  
  bool GetCurrentText(nsAString& aTextContent);

  class MouseTracker final
  {
  public:
    static const DWORD kInvalidCookie = static_cast<DWORD>(-1);

    MouseTracker();

    HRESULT Init(nsTextStore* aTextStore);
    HRESULT AdviseSink(nsTextStore* aTextStore,
                       ITfRangeACP* aTextRange, ITfMouseSink* aMouseSink);
    void UnadviseSink();

    bool IsUsing() const { return mSink != nullptr; }
    bool InRange(uint32_t aOffset) const
    {
      if (NS_WARN_IF(mStart < 0) ||
          NS_WARN_IF(mLength <= 0)) {
        return false;
      }
      return aOffset >= static_cast<uint32_t>(mStart) &&
             aOffset < static_cast<uint32_t>(mStart + mLength);
    }
    DWORD Cookie() const { return mCookie; }
    bool OnMouseButtonEvent(ULONG aEdge, ULONG aQuadrant, DWORD aButtonStatus);
    LONG RangeStart() const { return mStart; }
  
  private:
    nsRefPtr<ITfMouseSink> mSink;
    LONG mStart;
    LONG mLength;
    DWORD mCookie;
  };
  
  
  nsTArray<MouseTracker> mMouseTrackers;

  
  nsTArray<InputScope>         mInputScopes;

  
  
  enum
  {
    
    eNotSupported = -1,

    
    eInputScope = 0,
    eTextVerticalWriting,
    eTextOrientation,

    
    NUM_OF_SUPPORTED_ATTRS
  };
  bool mRequestedAttrs[NUM_OF_SUPPORTED_ATTRS];

  int32_t GetRequestedAttrIndex(const TS_ATTRID& aAttrID);
  TS_ATTRID GetAttrID(int32_t aIndex);

  bool mRequestedAttrValues;

  
  
  bool                         mIsRecordingActionsWithoutLock;
  
  
  
  
  
  
  
  bool                         mPendingOnSelectionChange;
  
  
  
  
  
  bool                         mPendingOnLayoutChange;
  
  
  bool                         mPendingDestroy;
  
  bool                         mNativeCaretIsCreated;

  
  static mozilla::StaticRefPtr<ITfThreadMgr> sThreadMgr;
  
  static mozilla::StaticRefPtr<ITfMessagePump> sMessagePump;
  
  static mozilla::StaticRefPtr<ITfKeystrokeMgr> sKeystrokeMgr;
  
  static mozilla::StaticRefPtr<ITfDisplayAttributeMgr> sDisplayAttrMgr;
  
  static mozilla::StaticRefPtr<ITfCategoryMgr> sCategoryMgr;

  
  
  
  
  static mozilla::StaticRefPtr<nsTextStore> sEnabledTextStore;

  
  static mozilla::StaticRefPtr<ITfDocumentMgr> sDisabledDocumentMgr;
  static mozilla::StaticRefPtr<ITfContext> sDisabledContext;

  static mozilla::StaticRefPtr<ITfInputProcessorProfiles>
    sInputProcessorProfiles;

  
  static DWORD sClientId;

  
  static bool sCreateNativeCaretForATOK;
  static bool sDoNotReturnNoLayoutErrorToFreeChangJie;
  static bool sDoNotReturnNoLayoutErrorToEasyChangjei;
  static bool sDoNotReturnNoLayoutErrorToGoogleJaInputAtFirstChar;
  static bool sDoNotReturnNoLayoutErrorToGoogleJaInputAtCaret;
};

#endif
