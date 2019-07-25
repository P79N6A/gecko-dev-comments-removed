





































#ifndef NSTEXTSTORE_H_
#define NSTEXTSTORE_H_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIWidget.h"

#include <msctf.h>
#include <textstor.h>

struct ITfThreadMgr;
struct ITfDocumentMgr;
struct ITfDisplayAttributeMgr;
struct ITfCategoryMgr;
class nsWindow;
class nsTextEvent;




#define WM_USER_TSF_TEXTCHANGE  (WM_USER + 0x100)





class nsTextStore : public ITextStoreACP,
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
    if (!sTsfTextStore) return;
    sTsfTextStore->CommitCompositionInternal(aDiscard);
  }

  static void     SetInputContext(const InputContext& aContext)
  {
    if (!sTsfTextStore) return;
    sTsfTextStore->SetInputContextInternal(aContext.mIMEState.mEnabled);
  }

  static nsresult OnFocusChange(bool, nsWindow*, IMEState::Enabled);

  static nsresult OnTextChange(PRUint32 aStart,
                               PRUint32 aOldEnd,
                               PRUint32 aNewEnd)
  {
    if (!sTsfTextStore) return NS_OK;
    return sTsfTextStore->OnTextChangeInternal(aStart, aOldEnd, aNewEnd);
  }

  static void     OnTextChangeMsg(void)
  {
    if (!sTsfTextStore) return;
    
    
    sTsfTextStore->OnTextChangeMsgInternal();
  }

  static nsresult OnSelectionChange(void)
  {
    if (!sTsfTextStore) return NS_OK;
    return sTsfTextStore->OnSelectionChangeInternal();
  }

  static void CompositionTimerCallbackFunc(nsITimer *aTimer, void *aClosure)
  {
    nsTextStore *ts = static_cast<nsTextStore*>(aClosure);
    ts->OnCompositionTimer();
  }

  
  
  static void*    GetThreadMgr(void)
  {
    Initialize(); 
    return (void*) & sTsfThreadMgr;
  }

  static void*    GetCategoryMgr(void)
  {
    return (void*) & sCategoryMgr;
  }

  static void*    GetDisplayAttrMgr(void)
  {
    return (void*) & sDisplayAttrMgr;
  }

protected:
  nsTextStore();
  ~nsTextStore();

  bool     Create(nsWindow*, IMEState::Enabled);
  bool     Destroy(void);

  
  
  
  
  HRESULT  SetSelectionInternal(const TS_SELECTION_ACP*,
                                bool aDispatchTextEvent = false);
  HRESULT  OnStartCompositionInternal(ITfCompositionView*, ITfRange*, bool);
  void     CommitCompositionInternal(bool);
  void     SetInputContextInternal(IMEState::Enabled aState);
  nsresult OnTextChangeInternal(PRUint32, PRUint32, PRUint32);
  void     OnTextChangeMsgInternal(void);
  nsresult OnSelectionChangeInternal(void);
  HRESULT  GetDisplayAttribute(ITfProperty* aProperty,
                               ITfRange* aRange,
                               TF_DISPLAYATTRIBUTE* aResult);
  HRESULT  UpdateCompositionExtent(ITfRange* pRangeNew);
  HRESULT  SendTextEventForCompositionString();
  HRESULT  SaveTextEvent(const nsTextEvent* aEvent);
  nsresult OnCompositionTimer();

  
  nsRefPtr<ITfDocumentMgr>     mDocumentMgr;
  
  DWORD                        mEditCookie;
  
  nsRefPtr<ITfContext>         mContext;
  
  nsRefPtr<ITextStoreACPSink>  mSink;
  
  DWORD                        mSinkMask;
  
  nsWindow*                    mWindow;
  
  DWORD                        mLock;
  
  DWORD                        mLockQueued;
  
  TS_TEXTCHANGE                mTextChange;
  
  nsRefPtr<ITfCompositionView> mCompositionView;
  
  
  
  
  
  nsString                     mCompositionString;
  
  
  
  
  
  TS_SELECTION_ACP             mCompositionSelection;
  
  LONG                         mCompositionStart;
  LONG                         mCompositionLength;
  
  
  nsTextEvent*                 mLastDispatchedTextEvent;
  
  
  nsString                     mLastDispatchedCompositionString;
  
  
  nsCOMPtr<nsITimer>           mCompositionTimer;

  
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
