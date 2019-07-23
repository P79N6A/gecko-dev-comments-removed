





































#ifndef NSTEXTSTORE_H_
#define NSTEXTSTORE_H_

#include "nsAutoPtr.h"
#include "nsString.h"

#include <msctf.h>
#include <textstor.h>

struct ITfThreadMgr;
struct ITfDocumentMgr;
class nsWindow;




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

public:
  static void     Initialize(void);
  static void     Terminate(void);
  static void     SetIMEOpenState(PRBool);
  static PRBool   GetIMEOpenState(void);

  static void     CommitComposition(PRBool aDiscard)
  {
    if (!sTsfTextStore) return;
    sTsfTextStore->CommitCompositionInternal(aDiscard);
  }

  static void     SetIMEEnabled(PRUint32 aState)
  {
    if (!sTsfTextStore) return;
    sTsfTextStore->SetIMEEnabledInternal(aState);
  }

  static nsresult OnFocusChange(PRBool, nsWindow*, PRUint32);

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

  static void*    GetNativeData(void)
  {
    
    
    Initialize(); 
    return (void*) & sTsfThreadMgr;
  }

protected:
  nsTextStore();
  ~nsTextStore();

  PRBool   Create(nsWindow*, PRUint32);
  PRBool   Destroy(void);
  PRBool   Focus(void);
  PRBool   Blur(void);

  HRESULT  LoadManagers(void);
  HRESULT  SetSelectionInternal(const TS_SELECTION_ACP*);
  HRESULT  OnStartCompositionInternal(ITfCompositionView*, ITfRange*, PRBool);
  void     CommitCompositionInternal(PRBool);
  void     SetIMEEnabledInternal(PRUint32 aState);
  nsresult OnTextChangeInternal(PRUint32, PRUint32, PRUint32);
  void     OnTextChangeMsgInternal(void);
  nsresult OnSelectionChangeInternal(void);

  
  nsRefPtr<ITfDisplayAttributeMgr> mDAMgr;
  
  nsRefPtr<ITfCategoryMgr>         mCatMgr;

  
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

  
  static ITfThreadMgr*  sTsfThreadMgr;
  
  static DWORD          sTsfClientId;
  
  
  
  static nsTextStore*   sTsfTextStore;

  
  
  static UINT           sFlushTIPInputMessage;

private:
  ULONG                       mRefCnt;
};

#endif 
