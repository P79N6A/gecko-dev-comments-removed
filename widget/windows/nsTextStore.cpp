




#include <olectl.h>
#include <algorithm>

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nscore.h"
#include "nsTextStore.h"
#include "nsWindow.h"
#ifdef MOZ_METRO
#include "winrt/MetroWidget.h"
#endif
#include "nsPrintfCString.h"
#include "WinUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::widget;

#ifdef PR_LOGGING


















PRLogModuleInfo* sTextStoreLog = nullptr;
#endif 

#define IS_SEARCH static_cast<InputScope>(50)






static const GUID GUID_PROP_INPUTSCOPE =
  { 0x1713dd5a, 0x68e7, 0x4a5b,
    { 0x9a, 0xf6, 0x59, 0x2a, 0x59, 0x5c, 0x77, 0x8d } };

class InputScopeImpl MOZ_FINAL : public ITfInputScope
{
public:
  InputScopeImpl(const nsTArray<InputScope>& aList) :
    mRefCnt(1),
    mInputScopes(aList)
  {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF: 0x%p InputScopeImpl()", this));
  }

  STDMETHODIMP_(ULONG) AddRef(void) { return ++mRefCnt; }

  STDMETHODIMP_(ULONG) Release(void)
  {
    --mRefCnt;
    if (mRefCnt) {
      return mRefCnt;
    }
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF: 0x%p InputScopeImpl::Release() final", this));
    delete this;
    return 0;
  }

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
  {
    *ppv=NULL;
    if ( (IID_IUnknown == riid) || (IID_ITfInputScope == riid) ) {
      *ppv = static_cast<ITfInputScope*>(this);
    }
    if (*ppv) {
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHODIMP GetInputScopes(InputScope** pprgInputScopes, UINT* pcCount)
  {
    uint32_t count = (mInputScopes.IsEmpty() ? 1 : mInputScopes.Length());

    InputScope* pScope = (InputScope*) CoTaskMemAlloc(sizeof(InputScope) * count);
    NS_ENSURE_TRUE(pScope, E_OUTOFMEMORY);

    if (mInputScopes.IsEmpty()) {
      *pScope = IS_DEFAULT;
      *pcCount = 1;
      *pprgInputScopes = pScope;
      return S_OK;
    }

    *pcCount = 0;

    for (uint32_t idx = 0; idx < count; idx++) {
      *(pScope + idx) = mInputScopes[idx];
      (*pcCount)++;
    }

    *pprgInputScopes = pScope;
    return S_OK;
  }

  STDMETHODIMP GetPhrase(BSTR **ppbstrPhrases, UINT *pcCount) { return E_NOTIMPL; }
  STDMETHODIMP GetRegularExpression(BSTR *pbstrRegExp) { return E_NOTIMPL; }
  STDMETHODIMP GetSRGS(BSTR *pbstrSRGS) { return E_NOTIMPL; }
  STDMETHODIMP GetXML(BSTR *pbstrXML) { return E_NOTIMPL; }

private:
  DWORD mRefCnt;
  nsTArray<InputScope> mInputScopes;
};





ITfThreadMgr*           nsTextStore::sTsfThreadMgr   = NULL;
ITfDisplayAttributeMgr* nsTextStore::sDisplayAttrMgr = NULL;
ITfCategoryMgr*         nsTextStore::sCategoryMgr    = NULL;
DWORD         nsTextStore::sTsfClientId  = 0;
nsTextStore*  nsTextStore::sTsfTextStore = NULL;

UINT nsTextStore::sFlushTIPInputMessage  = 0;

#ifdef PR_LOGGING

static const char*
GetBoolName(bool aBool)
{
  return aBool ? "true" : "false";
}

static void
HandleSeparator(nsCString& aDesc)
{
  if (!aDesc.IsEmpty()) {
    aDesc.AppendLiteral(" | ");
  }
}

static const nsCString
GetFindFlagName(DWORD aFindFlag)
{
  nsAutoCString description;
  if (!aFindFlag) {
    description.AppendLiteral("no flags (0)");
    return description;
  }
  if (aFindFlag & TS_ATTR_FIND_BACKWARDS) {
    description.AppendLiteral("TS_ATTR_FIND_BACKWARDS");
  }
  if (aFindFlag & TS_ATTR_FIND_WANT_OFFSET) {
    HandleSeparator(description);
    description.AppendLiteral("TS_ATTR_FIND_WANT_OFFSET");
  }
  if (aFindFlag & TS_ATTR_FIND_UPDATESTART) {
    HandleSeparator(description);
    description.AppendLiteral("TS_ATTR_FIND_UPDATESTART");
  }
  if (aFindFlag & TS_ATTR_FIND_WANT_VALUE) {
    HandleSeparator(description);
    description.AppendLiteral("TS_ATTR_FIND_WANT_VALUE");
  }
  if (aFindFlag & TS_ATTR_FIND_WANT_END) {
    HandleSeparator(description);
    description.AppendLiteral("TS_ATTR_FIND_WANT_END");
  }
  if (aFindFlag & TS_ATTR_FIND_HIDDEN) {
    HandleSeparator(description);
    description.AppendLiteral("TS_ATTR_FIND_HIDDEN");
  }
  if (description.IsEmpty()) {
    description.AppendLiteral("Unknown (");
    description.AppendInt(static_cast<uint32_t>(aFindFlag));
    description.AppendLiteral(")");
  }
  return description;
}

static const char*
GetIMEEnabledName(IMEState::Enabled aIMEEnabled)
{
  switch (aIMEEnabled) {
    case IMEState::DISABLED:
      return "DISABLED";
    case IMEState::ENABLED:
      return "ENABLED";
    case IMEState::PASSWORD:
      return "PASSWORD";
    case IMEState::PLUGIN:
      return "PLUGIN";
    default:
      return "Invalid";
  }
}

static nsCString
GetCLSIDNameStr(REFCLSID aCLSID)
{
  LPOLESTR str = nullptr;
  HRESULT hr = ::StringFromCLSID(aCLSID, &str);
  if (FAILED(hr) || !str || !str[0]) {
    return EmptyCString();
  }

  nsAutoCString result;
  result = NS_ConvertUTF16toUTF8(str);
  ::CoTaskMemFree(str);
  return result;
}

static nsCString
GetRIIDNameStr(REFIID aRIID)
{
  LPOLESTR str = nullptr;
  HRESULT hr = ::StringFromIID(aRIID, &str);
  if (FAILED(hr) || !str || !str[0]) {
    return EmptyCString();
  }

  nsAutoString key(L"Interface\\");
  key += str;

  nsAutoCString result;
  PRUnichar buf[256];
  if (WinUtils::GetRegistryKey(HKEY_CLASSES_ROOT, key.get(), nullptr,
                               buf, sizeof(buf))) {
    result = NS_ConvertUTF16toUTF8(buf);
  } else {
    result = NS_ConvertUTF16toUTF8(str);
  }

  ::CoTaskMemFree(str);
  return result;
}

static const char*
GetCommonReturnValueName(HRESULT aResult)
{
  switch (aResult) {
    case S_OK:
      return "S_OK";
    case E_ABORT:
      return "E_ABORT";
    case E_ACCESSDENIED:
      return "E_ACCESSDENIED";
    case E_FAIL:
      return "E_FAIL";
    case E_HANDLE:
      return "E_HANDLE";
    case E_INVALIDARG:
      return "E_INVALIDARG";
    case E_NOINTERFACE:
      return "E_NOINTERFACE";
    case E_NOTIMPL:
      return "E_NOTIMPL";
    case E_OUTOFMEMORY:
      return "E_OUTOFMEMORY";
    case E_POINTER:
      return "E_POINTER";
    case E_UNEXPECTED:
      return "E_UNEXPECTED";
    default:
      return SUCCEEDED(aResult) ? "Succeeded" : "Failed";
  }
}

static const char*
GetTextStoreReturnValueName(HRESULT aResult)
{
  switch (aResult) {
    case TS_E_FORMAT:
      return "TS_E_FORMAT";
    case TS_E_INVALIDPOINT:
      return "TS_E_INVALIDPOINT";
    case TS_E_INVALIDPOS:
      return "TS_E_INVALIDPOS";
    case TS_E_NOINTERFACE:
      return "TS_E_NOINTERFACE";
    case TS_E_NOLAYOUT:
      return "TS_E_NOLAYOUT";
    case TS_E_NOLOCK:
      return "TS_E_NOLOCK";
    case TS_E_NOOBJECT:
      return "TS_E_NOOBJECT";
    case TS_E_NOSELECTION:
      return "TS_E_NOSELECTION";
    case TS_E_NOSERVICE:
      return "TS_E_NOSERVICE";
    case TS_E_READONLY:
      return "TS_E_READONLY";
    case TS_E_SYNCHRONOUS:
      return "TS_E_SYNCHRONOUS";
    case TS_S_ASYNC:
      return "TS_S_ASYNC";
    default:
      return GetCommonReturnValueName(aResult);
  }
}

static const nsCString
GetSinkMaskNameStr(DWORD aSinkMask)
{
  nsAutoCString description;
  if (aSinkMask & TS_AS_TEXT_CHANGE) {
    description.AppendLiteral("TS_AS_TEXT_CHANGE");
  }
  if (aSinkMask & TS_AS_SEL_CHANGE) {
    HandleSeparator(description);
    description.AppendLiteral("TS_AS_SEL_CHANGE");
  }
  if (aSinkMask & TS_AS_LAYOUT_CHANGE) {
    HandleSeparator(description);
    description.AppendLiteral("TS_AS_LAYOUT_CHANGE");
  }
  if (aSinkMask & TS_AS_ATTR_CHANGE) {
    HandleSeparator(description);
    description.AppendLiteral("TS_AS_ATTR_CHANGE");
  }
  if (aSinkMask & TS_AS_STATUS_CHANGE) {
    HandleSeparator(description);
    description.AppendLiteral("TS_AS_STATUS_CHANGE");
  }
  if (description.IsEmpty()) {
    description.AppendLiteral("not-specified");
  }
  return description;
}

static const char*
GetActiveSelEndName(TsActiveSelEnd aSelEnd)
{
  return aSelEnd == TS_AE_NONE  ? "TS_AE_NONE" :
         aSelEnd == TS_AE_START ? "TS_AE_START" :
         aSelEnd == TS_AE_END   ? "TS_AE_END" : "Unknown";
}

static const nsCString
GetLockFlagNameStr(DWORD aLockFlags)
{
  nsAutoCString description;
  if ((aLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE) {
    description.AppendLiteral("TS_LF_READWRITE");
  } else if (aLockFlags & TS_LF_READ) {
    description.AppendLiteral("TS_LF_READ");
  }
  if (aLockFlags & TS_LF_SYNC) {
    if (!description.IsEmpty()) {
      description.AppendLiteral(" | ");
    }
    description.AppendLiteral("TS_LF_SYNC");
  }
  if (description.IsEmpty()) {
    description.AppendLiteral("not-specified");
  }
  return description;
}

static const char*
GetTextRunTypeName(TsRunType aRunType)
{
  switch (aRunType) {
    case TS_RT_PLAIN:
      return "TS_RT_PLAIN";
    case TS_RT_HIDDEN:
      return "TS_RT_HIDDEN";
    case  TS_RT_OPAQUE:
      return "TS_RT_OPAQUE";
    default:
      return "Unknown";
  }
}

static nsCString
GetColorName(const TF_DA_COLOR &aColor)
{
  switch (aColor.type) {
    case TF_CT_NONE:
      return NS_LITERAL_CSTRING("TF_CT_NONE");
    case TF_CT_SYSCOLOR:
      return nsPrintfCString("TF_CT_SYSCOLOR, nIndex:0x%08X",
                             static_cast<int32_t>(aColor.nIndex));
    case TF_CT_COLORREF:
      return nsPrintfCString("TF_CT_COLORREF, cr:0x%08X",
                             static_cast<int32_t>(aColor.cr));
      break;
    default:
      return nsPrintfCString("Unknown(%08X)",
                             static_cast<int32_t>(aColor.type));
  }
}

static nsCString
GetLineStyleName(TF_DA_LINESTYLE aLineStyle)
{
  switch (aLineStyle) {
    case TF_LS_NONE:
      return NS_LITERAL_CSTRING("TF_LS_NONE");
    case TF_LS_SOLID:
      return NS_LITERAL_CSTRING("TF_LS_SOLID");
    case TF_LS_DOT:
      return NS_LITERAL_CSTRING("TF_LS_DOT");
    case TF_LS_DASH:
      return NS_LITERAL_CSTRING("TF_LS_DASH");
    case TF_LS_SQUIGGLE:
      return NS_LITERAL_CSTRING("TF_LS_SQUIGGLE");
    default: {
      return nsPrintfCString("Unknown(%08X)", static_cast<int32_t>(aLineStyle));
    }
  }
}

static nsCString
GetClauseAttrName(TF_DA_ATTR_INFO aAttr)
{
  switch (aAttr) {
    case TF_ATTR_INPUT:
      return NS_LITERAL_CSTRING("TF_ATTR_INPUT");
    case TF_ATTR_TARGET_CONVERTED:
      return NS_LITERAL_CSTRING("TF_ATTR_TARGET_CONVERTED");
    case TF_ATTR_CONVERTED:
      return NS_LITERAL_CSTRING("TF_ATTR_CONVERTED");
    case TF_ATTR_TARGET_NOTCONVERTED:
      return NS_LITERAL_CSTRING("TF_ATTR_TARGET_NOTCONVERTED");
    case TF_ATTR_INPUT_ERROR:
      return NS_LITERAL_CSTRING("TF_ATTR_INPUT_ERROR");
    case TF_ATTR_FIXEDCONVERTED:
      return NS_LITERAL_CSTRING("TF_ATTR_FIXEDCONVERTED");
    case TF_ATTR_OTHER:
      return NS_LITERAL_CSTRING("TF_ATTR_OTHER");
    default: {
      return nsPrintfCString("Unknown(%08X)", static_cast<int32_t>(aAttr));
    }
  }
}

static nsCString
GetDisplayAttrStr(const TF_DISPLAYATTRIBUTE &aDispAttr)
{
  nsAutoCString str;
  str = "crText:{ ";
  str += GetColorName(aDispAttr.crText);
  str += " }, crBk:{ ";
  str += GetColorName(aDispAttr.crBk);
  str += " }, lsStyle: ";
  str += GetLineStyleName(aDispAttr.lsStyle);
  str += ", fBoldLine: ";
  str += GetBoolName(aDispAttr.fBoldLine);
  str += ", crLine:{ ";
  str += GetColorName(aDispAttr.crLine);
  str += " }, bAttr: ";
  str += GetClauseAttrName(aDispAttr.bAttr);
  return str;
}

#endif 

nsTextStore::nsTextStore()
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::nsTestStore(): instance is created", this));

  mRefCnt = 1;
  mEditCookie = 0;
  mSinkMask = 0;
  mLock = 0;
  mLockQueued = 0;
  mTextChange.acpStart = INT32_MAX;
  mTextChange.acpOldEnd = mTextChange.acpNewEnd = 0;
  mLastDispatchedTextEvent = nullptr;
  mInputScopeDetected = false;
  mInputScopeRequested = false;
}

nsTextStore::~nsTextStore()
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore instance is destroyed, "
     "mWidget=0x%p, mDocumentMgr=0x%p, mContext=0x%p",
     this, mWidget.get(), mDocumentMgr.get(), mContext.get()));

  if (mCompositionTimer) {
    mCompositionTimer->Cancel();
    mCompositionTimer = nullptr;
  }
  SaveTextEvent(nullptr);
}

bool
nsTextStore::Create(nsWindowBase* aWidget,
                    IMEState::Enabled aIMEEnabled)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::Create(aWidget=0x%p, aIMEEnabled=%s)",
     this, aWidget, GetIMEEnabledName(aIMEEnabled)));

  if (mDocumentMgr) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::Create() FAILED due to already initialized",
       this));
    return false;
  }

  
  HRESULT hr = sTsfThreadMgr->CreateDocumentMgr(
                                  getter_AddRefs(mDocumentMgr));
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::Create() FAILED to create DocumentMgr "
       "(0x%08X)", this, hr));
    return false;
  }
  mWidget = aWidget;

  
  hr = mDocumentMgr->CreateContext(sTsfClientId, 0,
                                   static_cast<ITextStoreACP*>(this),
                                   getter_AddRefs(mContext), &mEditCookie);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::Create() FAILED to create the context "
       "(0x%08X)", this, hr));
    mDocumentMgr = NULL;
    return false;
  }

  SetInputContextInternal(aIMEEnabled);

  hr = mDocumentMgr->Push(mContext);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::Create() FAILED to push the context (0x%08X)",
       this, hr));
    
    mContext = NULL;
    mDocumentMgr = NULL;
    return false;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p   nsTextStore::Create() succeeded: "
     "mDocumentMgr=0x%p, mContext=0x%p, mEditCookie=0x%08X",
     this, mDocumentMgr.get(), mContext.get(), mEditCookie));

  return true;
}

bool
nsTextStore::Destroy(void)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::Destroy()", this));

  if (mWidget) {
    
    
    
    
    MSG msg;
    if (::PeekMessageW(&msg, mWidget->GetWindowHandle(),
                       sFlushTIPInputMessage, sFlushTIPInputMessage,
                       PM_REMOVE)) {
      ::DispatchMessageW(&msg);
    }
  }
  mContext = NULL;
  if (mDocumentMgr) {
    mDocumentMgr->Pop(TF_POPF_ALL);
    mDocumentMgr = NULL;
  }
  mSink = NULL;
  mWidget = nullptr;

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p   nsTextStore::Destroy() succeeded", this));
  return true;
}

STDMETHODIMP
nsTextStore::QueryInterface(REFIID riid,
                            void** ppv)
{
  *ppv=NULL;
  if ( (IID_IUnknown == riid) || (IID_ITextStoreACP == riid) ) {
    *ppv = static_cast<ITextStoreACP*>(this);
  } else if (IID_ITfContextOwnerCompositionSink == riid) {
    *ppv = static_cast<ITfContextOwnerCompositionSink*>(this);
  }
  if (*ppv) {
    AddRef();
    return S_OK;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ERROR,
    ("TSF: 0x%p nsTextStore::QueryInterface() FAILED, riid=%s",
     this, GetRIIDNameStr(riid).get()));
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) nsTextStore::AddRef()
{
  return ++mRefCnt;
}

STDMETHODIMP_(ULONG) nsTextStore::Release()
{
  --mRefCnt;
  if (0 != mRefCnt)
    return mRefCnt;
  delete this;
  return 0;
}

STDMETHODIMP
nsTextStore::AdviseSink(REFIID riid,
                        IUnknown *punk,
                        DWORD dwMask)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::AdviseSink(riid=%s, punk=0x%p, dwMask=%s), "
     "mSink=0x%p, mSinkMask=%s",
     this, GetRIIDNameStr(riid).get(), punk, GetSinkMaskNameStr(dwMask).get(),
     mSink.get(), GetSinkMaskNameStr(mSinkMask).get()));

  if (!punk) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::AdviseSink() FAILED due to the null punk",
       this));
    return E_UNEXPECTED;
  }

  if (IID_ITextStoreACPSink != riid) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::AdviseSink() FAILED due to "
       "unsupported interface", this));
    return E_INVALIDARG; 
  }

  if (!mSink) {
    
    punk->QueryInterface(IID_ITextStoreACPSink, getter_AddRefs(mSink));
    if (!mSink) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
        ("TSF: 0x%p   nsTextStore::AdviseSink() FAILED due to "
         "punk not having the interface", this));
      return E_UNEXPECTED;
    }
  } else {
    
    
    nsRefPtr<IUnknown> comparison1, comparison2;
    punk->QueryInterface(IID_IUnknown, getter_AddRefs(comparison1));
    mSink->QueryInterface(IID_IUnknown, getter_AddRefs(comparison2));
    if (comparison1 != comparison2) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
        ("TSF: 0x%p   nsTextStore::AdviseSink() FAILED due to "
         "the sink being different from the stored sink", this));
      return CONNECT_E_ADVISELIMIT;
    }
  }
  
  mSinkMask = dwMask;
  return S_OK;
}

STDMETHODIMP
nsTextStore::UnadviseSink(IUnknown *punk)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::UnadviseSink(punk=0x%p), mSink=0x%p",
     this, punk, mSink.get()));

  if (!punk) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::UnadviseSink() FAILED due to the null punk",
       this));
    return E_INVALIDARG;
  }
  if (!mSink) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::UnadviseSink() FAILED due to "
       "any sink not stored", this));
    return CONNECT_E_NOCONNECTION;
  }
  
  nsRefPtr<IUnknown> comparison1, comparison2;
  punk->QueryInterface(IID_IUnknown, getter_AddRefs(comparison1));
  mSink->QueryInterface(IID_IUnknown, getter_AddRefs(comparison2));
  
  if (comparison1 != comparison2) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::UnadviseSink() FAILED due to "
       "the sink being different from the stored sink", this));
    return CONNECT_E_NOCONNECTION;
  }
  mSink = NULL;
  mSinkMask = 0;
  return S_OK;
}

STDMETHODIMP
nsTextStore::RequestLock(DWORD dwLockFlags,
                         HRESULT *phrSession)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::RequestLock(dwLockFlags=%s, phrSession=0x%p), "
     "mLock=%s", this, GetLockFlagNameStr(dwLockFlags).get(), phrSession,
     GetLockFlagNameStr(mLock).get()));

  if (!mSink) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::RequestLock() FAILED due to "
       "any sink not stored", this));
    return E_FAIL;
  }
  if (!phrSession) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::RequestLock() FAILED due to "
       "null phrSession", this));
    return E_INVALIDARG;
  }

  if (!mLock) {
    
    mLock = dwLockFlags & (~TS_LF_SYNC);
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
      ("TSF: 0x%p   nsTextStore::RequestLock() notifying OnLockGranted()...",
       this));
    *phrSession = mSink->OnLockGranted(mLock);
    while (mLockQueued) {
      mLock = mLockQueued;
      mLockQueued = 0;
      PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
        ("TSF: 0x%p   nsTextStore::RequestLock() notifying OnLockGranted() "
         "with mLockQueued (%s)...",
         this, GetLockFlagNameStr(mLock).get()));
      mSink->OnLockGranted(mLock);
    }
    mLock = 0;
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
      ("TSF: 0x%p   nsTextStore::RequestLock() succeeded: *phrSession=%s",
       this, GetTextStoreReturnValueName(*phrSession)));
    return S_OK;
  }

  
  
  if (IsReadLocked() && !IsReadWriteLocked() && IsReadWriteLock(dwLockFlags) &&
      !(dwLockFlags & TS_LF_SYNC)) {
    *phrSession = TS_S_ASYNC;
    mLockQueued = dwLockFlags & (~TS_LF_SYNC);

    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF: 0x%p   nsTextStore::RequestLock() stores the request in the "
       "queue, *phrSession=TS_S_ASYNC", this));
    return S_OK;
  }

  
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p   nsTextStore::RequestLock() didn't allow to lock, "
     "*phrSession=TS_E_SYNCHRONOUS", this));
  *phrSession = TS_E_SYNCHRONOUS;
  return E_FAIL;
}

STDMETHODIMP
nsTextStore::GetStatus(TS_STATUS *pdcs)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: 0x%p nsTextStore::GetStatus(pdcs=0x%p)", this, pdcs));

  if (!pdcs) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
      ("TSF: 0x%p   nsTextStore::GetStatus() FAILED due to null pdcs", this));
    return E_INVALIDARG;
  }
  pdcs->dwDynamicFlags = 0;
  
  pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT;
  return S_OK;
}

STDMETHODIMP
nsTextStore::QueryInsert(LONG acpTestStart,
                         LONG acpTestEnd,
                         ULONG cch,
                         LONG *pacpResultStart,
                         LONG *pacpResultEnd)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::QueryInsert(acpTestStart=%ld, "
          "acpTestEnd=%ld, cch=%lu, pacpResultStart=0x%p, pacpResultEnd=0x%p)",
          this, acpTestStart, acpTestEnd, cch, acpTestStart, acpTestEnd));

  if (!pacpResultStart || !pacpResultEnd) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::QueryInsert() FAILED due to "
            "the null argument", this));
    return E_INVALIDARG;
  }

  if (acpTestStart < 0 || acpTestStart > acpTestEnd) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::QueryInsert() FAILED due to "
            "wrong argument", this));
    return E_INVALIDARG;
  }

  
  
  *pacpResultStart = acpTestStart;
  *pacpResultEnd = acpTestStart + cch;

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p  nsTextStore::QueryInsert() succeeded: "
          "*pacpResultStart=%ld, *pacpResultEnd=%ld)",
          this, *pacpResultStart, *pacpResultEnd));
  return S_OK;
}

STDMETHODIMP
nsTextStore::GetSelection(ULONG ulIndex,
                          ULONG ulCount,
                          TS_SELECTION_ACP *pSelection,
                          ULONG *pcFetched)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetSelection(ulIndex=%lu, ulCount=%lu, "
          "pSelection=0x%p, pcFetched=0x%p)",
          this, ulIndex, ulCount, pSelection, pcFetched));

  if (!IsReadLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetSelection() FAILED due to not locked",
            this));
    return TS_E_NOLOCK;
  }
  if (!ulCount || !pSelection || !pcFetched) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetSelection() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  *pcFetched = 0;

  if (ulIndex != static_cast<ULONG>(TS_DEFAULT_SELECTION) &&
      ulIndex != 0) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetSelection() FAILED due to "
            "unsupported selection", this));
    return TS_E_NOSELECTION;
  }

  if (!GetSelectionInternal(*pSelection)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetSelection() FAILED to get selection",
            this));
    return E_FAIL;
  }

  *pcFetched = 1;
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetSelection() succeeded", this));
  return S_OK;
}

bool
nsTextStore::GetSelectionInternal(TS_SELECTION_ACP &aSelectionACP)
{
  if (mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::GetSelectionInternal(), "
            "there is no composition view", this));

    
    aSelectionACP = mCompositionSelection;
  } else {
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::GetSelectionInternal(), "
            "try to get normal selection...", this));

    
    nsQueryContentEvent event(true, NS_QUERY_SELECTED_TEXT, mWidget);
    mWidget->InitEvent(event);
    mWidget->DispatchWindowEvent(&event);
    if (!event.mSucceeded) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetSelectionInternal() FAILED to "
              "query selected text", this));
      return false;
    }
    
    
    
    
    aSelectionACP.acpStart = event.mReply.mOffset;
    aSelectionACP.acpEnd =
      aSelectionACP.acpStart + event.mReply.mString.Length();
    aSelectionACP.style.ase =
      event.mReply.mString.Length() && event.mReply.mReversed ? TS_AE_START :
                                                                TS_AE_END;
    
    aSelectionACP.style.fInterimChar = FALSE;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetSelectionInternal() succeeded: "
          "acpStart=%lu, acpEnd=%lu, style.ase=%s, style.fInterimChar=%s",
          this, aSelectionACP.acpStart, aSelectionACP.acpEnd,
          GetActiveSelEndName(aSelectionACP.style.ase),
          GetBoolName(aSelectionACP.style.fInterimChar)));

  return true;
}

static HRESULT
GetRangeExtent(ITfRange* aRange, LONG* aStart, LONG* aLength)
{
  nsRefPtr<ITfRangeACP> rangeACP;
  aRange->QueryInterface(IID_ITfRangeACP, getter_AddRefs(rangeACP));
  NS_ENSURE_TRUE(rangeACP, E_FAIL);
  return rangeACP->GetExtent(aStart, aLength);
}

static uint32_t
GetGeckoSelectionValue(TF_DISPLAYATTRIBUTE &aDisplayAttr)
{
  uint32_t result;
  switch (aDisplayAttr.bAttr) {
    case TF_ATTR_TARGET_CONVERTED:
      result = NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
      break;
    case TF_ATTR_CONVERTED:
      result = NS_TEXTRANGE_CONVERTEDTEXT;
      break;
    case TF_ATTR_TARGET_NOTCONVERTED:
      result = NS_TEXTRANGE_SELECTEDRAWTEXT;
      break;
    default:
      result = NS_TEXTRANGE_RAWINPUT;
      break;
  }
  return result;
}

HRESULT
nsTextStore::GetDisplayAttribute(ITfProperty* aAttrProperty,
                                 ITfRange* aRange,
                                 TF_DISPLAYATTRIBUTE* aResult)
{
  NS_ENSURE_TRUE(aAttrProperty, E_FAIL);
  NS_ENSURE_TRUE(aRange, E_FAIL);
  NS_ENSURE_TRUE(aResult, E_FAIL);

  HRESULT hr;

#ifdef PR_LOGGING
  if (PR_LOG_TEST(sTextStoreLog, PR_LOG_DEBUG)) {
    LONG start = 0, length = 0;
    hr = GetRangeExtent(aRange, &start, &length);
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute(): "
            "GetDisplayAttribute range=%ld-%ld (hr=%s)",
            this, start - mCompositionStart, start - mCompositionStart + length,
            GetCommonReturnValueName(hr)));
  }
#endif

  VARIANT propValue;
  ::VariantInit(&propValue);
  hr = aAttrProperty->GetValue(TfEditCookie(mEditCookie), aRange, &propValue);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() FAILED due to "
            "ITfProperty::GetValue() failed", this));
    return hr;
  }
  if (VT_I4 != propValue.vt) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() FAILED due to "
            "ITfProperty::GetValue() returns non-VT_I4 value", this));
    ::VariantClear(&propValue);
    return E_FAIL;
  }

  NS_ENSURE_TRUE(sCategoryMgr, E_FAIL);
  GUID guid;
  hr = sCategoryMgr->GetGUID(DWORD(propValue.lVal), &guid);
  ::VariantClear(&propValue);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() FAILED due to "
            "ITfCategoryMgr::GetGUID() failed", this));
    return hr;
  }

  NS_ENSURE_TRUE(sDisplayAttrMgr, E_FAIL);
  nsRefPtr<ITfDisplayAttributeInfo> info;
  hr = sDisplayAttrMgr->GetDisplayAttributeInfo(guid, getter_AddRefs(info),
                                                NULL);
  if (FAILED(hr) || !info) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() FAILED due to "
            "ITfDisplayAttributeMgr::GetDisplayAttributeInfo() failed", this));
    return hr;
  }

  hr = info->GetAttributeInfo(aResult);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() FAILED due to "
            "ITfDisplayAttributeInfo::GetAttributeInfo() failed", this));
    return hr;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::GetDisplayAttribute() succeeded: "
          "Result={ %s }", this, GetDisplayAttrStr(*aResult).get()));
  return S_OK;
}

HRESULT
nsTextStore::SaveTextEvent(const nsTextEvent* aEvent)
{
  if (mLastDispatchedTextEvent) {
    if (mLastDispatchedTextEvent->rangeArray)
      delete [] mLastDispatchedTextEvent->rangeArray;
    delete mLastDispatchedTextEvent;
    mLastDispatchedTextEvent = nullptr;
  }
  if (!aEvent)
    return S_OK;

  mLastDispatchedTextEvent = new nsTextEvent(true, NS_TEXT_TEXT, nullptr);
  if (!mLastDispatchedTextEvent)
    return E_OUTOFMEMORY;
  mLastDispatchedTextEvent->rangeCount = aEvent->rangeCount;
  mLastDispatchedTextEvent->theText = aEvent->theText;
  mLastDispatchedTextEvent->rangeArray = nullptr;

  if (aEvent->rangeCount == 0)
    return S_OK;

  NS_ENSURE_TRUE(aEvent->rangeArray, E_FAIL);

  mLastDispatchedTextEvent->rangeArray = new nsTextRange[aEvent->rangeCount];
  if (!mLastDispatchedTextEvent->rangeArray) {
    delete mLastDispatchedTextEvent;
    mLastDispatchedTextEvent = nullptr;
    return E_OUTOFMEMORY;
  }
  memcpy(mLastDispatchedTextEvent->rangeArray, aEvent->rangeArray,
         sizeof(nsTextRange) * aEvent->rangeCount);
  return S_OK;
}

static bool
IsSameTextEvent(const nsTextEvent* aEvent1, const nsTextEvent* aEvent2)
{
  NS_PRECONDITION(aEvent1 || aEvent2, "both events are null");
  NS_PRECONDITION(aEvent2 && (aEvent1 != aEvent2),
                  "both events are same instance");

  return (aEvent1 && aEvent2 &&
          aEvent1->rangeCount == aEvent2->rangeCount &&
          aEvent1->theText == aEvent2->theText &&
          (aEvent1->rangeCount == 0 ||
           ((aEvent1->rangeArray && aEvent2->rangeArray) &&
           !memcmp(aEvent1->rangeArray, aEvent2->rangeArray,
                   sizeof(nsTextRange) * aEvent1->rangeCount))));
}

HRESULT
nsTextStore::UpdateCompositionExtent(ITfRange* aRangeNew)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent(aRangeNew=0x%p), "
          "mCompositionView=0x%p", this, aRangeNew, mCompositionView.get()));

  if (!mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent() FAILED due to "
            "no composition view", this));
    return E_FAIL;
  }

  HRESULT hr;
  nsRefPtr<ITfCompositionView> pComposition(mCompositionView);
  nsRefPtr<ITfRange> composingRange(aRangeNew);
  if (!composingRange) {
    hr = pComposition->GetRange(getter_AddRefs(composingRange));
    if (FAILED(hr)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent() FAILED due to "
              "pComposition->GetRange() failure", this));
      return hr;
    }
  }

  
  LONG compStart = 0, compLength = 0;
  hr = GetRangeExtent(composingRange, &compStart, &compLength);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent() FAILED due to "
            "GetRangeExtent() failure", this));
    return hr;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent(), range=%ld-%ld, "
          "mCompositionStart=%ld, mCompositionString.Length()=%lu",
          this, compStart, compStart + compLength, mCompositionStart,
          mCompositionString.Length()));

  if (mCompositionStart != compStart ||
      mCompositionString.Length() != (ULONG)compLength) {
    
    
    
    
    
    
    OnEndComposition(pComposition);
    OnStartCompositionInternal(pComposition, composingRange, true);
  } else {
    mCompositionLength = compLength;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::UpdateCompositionExtent() succeeded", this));
  return S_OK;
}

static bool
GetColor(const TF_DA_COLOR &aTSFColor, nscolor &aResult)
{
  switch (aTSFColor.type) {
    case TF_CT_SYSCOLOR: {
      DWORD sysColor = ::GetSysColor(aTSFColor.nIndex);
      aResult = NS_RGB(GetRValue(sysColor), GetGValue(sysColor),
                       GetBValue(sysColor));
      return true;
    }
    case TF_CT_COLORREF:
      aResult = NS_RGB(GetRValue(aTSFColor.cr), GetGValue(aTSFColor.cr),
                       GetBValue(aTSFColor.cr));
      return true;
    case TF_CT_NONE:
    default:
      return false;
  }
}

static bool
GetLineStyle(TF_DA_LINESTYLE aTSFLineStyle, uint8_t &aTextRangeLineStyle)
{
  switch (aTSFLineStyle) {
    case TF_LS_NONE:
      aTextRangeLineStyle = nsTextRangeStyle::LINESTYLE_NONE;
      return true;
    case TF_LS_SOLID:
      aTextRangeLineStyle = nsTextRangeStyle::LINESTYLE_SOLID;
      return true;
    case TF_LS_DOT:
      aTextRangeLineStyle = nsTextRangeStyle::LINESTYLE_DOTTED;
      return true;
    case TF_LS_DASH:
      aTextRangeLineStyle = nsTextRangeStyle::LINESTYLE_DASHED;
      return true;
    case TF_LS_SQUIGGLE:
      aTextRangeLineStyle = nsTextRangeStyle::LINESTYLE_WAVY;
      return true;
    default:
      return false;
  }
}

HRESULT
nsTextStore::SendTextEventForCompositionString()
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString(), "
          "mCompositionView=0x%p, mCompositionString=\"%s\"",
          this, mCompositionView.get(),
          NS_ConvertUTF16toUTF8(mCompositionString).get()));

  if (!mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() FAILED "
            "due to no composition view", this));
    return E_FAIL;
  }

  
  
  
  
  
  
  

  nsRefPtr<ITfProperty> attrPropetry;
  HRESULT hr = mContext->GetProperty(GUID_PROP_ATTRIBUTE,
                                     getter_AddRefs(attrPropetry));
  if (FAILED(hr) || !attrPropetry) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() FAILED "
            "due to mContext->GetProperty() failure", this));
    return FAILED(hr) ? hr : E_FAIL;
  }

  
  nsTextEvent event(true, NS_TEXT_TEXT, mWidget);
  mWidget->InitEvent(event);

  nsRefPtr<ITfRange> composingRange;
  hr = mCompositionView->GetRange(getter_AddRefs(composingRange));
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() FAILED "
            "due to mCompositionView->GetRange() failure", this));
    return hr;
  }

  nsRefPtr<IEnumTfRanges> enumRanges;
  hr = attrPropetry->EnumRanges(TfEditCookie(mEditCookie),
                                getter_AddRefs(enumRanges), composingRange);
  if (FAILED(hr) || !enumRanges) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() FAILED "
            "due to attrPropetry->EnumRanges() failure", this));
    return FAILED(hr) ? hr : E_FAIL;
  }

  nsAutoTArray<nsTextRange, 4> textRanges;
  nsTextRange newRange;
  
  
  newRange.mStartOffset = 0;
  newRange.mEndOffset = mCompositionString.Length();
  newRange.mRangeType = NS_TEXTRANGE_RAWINPUT;
  textRanges.AppendElement(newRange);

  nsRefPtr<ITfRange> range;
  while (S_OK == enumRanges->Next(1, getter_AddRefs(range), NULL) && range) {

    LONG start = 0, length = 0;
    if (FAILED(GetRangeExtent(range, &start, &length)))
      continue;

    nsTextRange newRange;
    newRange.mStartOffset = uint32_t(start - mCompositionStart);
    
    
    newRange.mEndOffset = mCompositionString.Length();

    TF_DISPLAYATTRIBUTE attr;
    hr = GetDisplayAttribute(attrPropetry, range, &attr);
    if (FAILED(hr)) {
      newRange.mRangeType = NS_TEXTRANGE_RAWINPUT;
    } else {
      newRange.mRangeType = GetGeckoSelectionValue(attr);
      if (GetColor(attr.crText, newRange.mRangeStyle.mForegroundColor)) {
        newRange.mRangeStyle.mDefinedStyles |=
                               nsTextRangeStyle::DEFINED_FOREGROUND_COLOR;
      }
      if (GetColor(attr.crBk, newRange.mRangeStyle.mBackgroundColor)) {
        newRange.mRangeStyle.mDefinedStyles |=
                               nsTextRangeStyle::DEFINED_BACKGROUND_COLOR;
      }
      if (GetColor(attr.crLine, newRange.mRangeStyle.mUnderlineColor)) {
        newRange.mRangeStyle.mDefinedStyles |=
                               nsTextRangeStyle::DEFINED_UNDERLINE_COLOR;
      }
      if (GetLineStyle(attr.lsStyle, newRange.mRangeStyle.mLineStyle)) {
        newRange.mRangeStyle.mDefinedStyles |=
                               nsTextRangeStyle::DEFINED_LINESTYLE;
        newRange.mRangeStyle.mIsBoldLine = attr.fBoldLine != 0;
      }
    }

    nsTextRange& lastRange = textRanges[textRanges.Length() - 1];
    if (lastRange.mStartOffset == newRange.mStartOffset) {
      
      
      lastRange = newRange;
    } else {
      lastRange.mEndOffset = newRange.mStartOffset;
      textRanges.AppendElement(newRange);
    }
  }

  
  
  
  
  
  
  
  
  if (mCompositionSelection.acpStart != mCompositionSelection.acpEnd &&
      textRanges.Length() == 1) {
    nsTextRange& range = textRanges[0];
    LONG start = std::min(mCompositionSelection.acpStart,
                        mCompositionSelection.acpEnd);
    LONG end = std::max(mCompositionSelection.acpStart,
                      mCompositionSelection.acpEnd);
    if ((LONG)range.mStartOffset == start - mCompositionStart &&
        (LONG)range.mEndOffset == end - mCompositionStart &&
        range.mRangeStyle.IsNoChangeStyle()) {
      range.mRangeStyle.Clear();
      
      range.mRangeType = NS_TEXTRANGE_SELECTEDRAWTEXT;
    }
  }

  
  LONG caretPosition = std::max(mCompositionSelection.acpStart,
                              mCompositionSelection.acpEnd);
  caretPosition -= mCompositionStart;
  nsTextRange caretRange;
  caretRange.mStartOffset = caretRange.mEndOffset = uint32_t(caretPosition);
  caretRange.mRangeType = NS_TEXTRANGE_CARETPOSITION;
  textRanges.AppendElement(caretRange);

  event.theText = mCompositionString;
  event.rangeArray = textRanges.Elements();
  event.rangeCount = textRanges.Length();

  
  
  if (IsSameTextEvent(mLastDispatchedTextEvent, &event)) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() "
            "succeeded but any DOM events are not dispatched", this));
    return S_OK;
  }

  if (mCompositionString != mLastDispatchedCompositionString) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() "
            "dispatching compositionupdate event...", this));
    nsCompositionEvent compositionUpdate(true, NS_COMPOSITION_UPDATE,
                                         mWidget);
    mWidget->InitEvent(compositionUpdate);
    compositionUpdate.data = mCompositionString;
    mLastDispatchedCompositionString = mCompositionString;
    mWidget->DispatchWindowEvent(&compositionUpdate);
  }

  if (mWidget && !mWidget->Destroyed()) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() "
            "dispatching text event...", this));
    mWidget->DispatchWindowEvent(&event);
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::SendTextEventForCompositionString() "
          "succeeded", this));

  return SaveTextEvent(&event);
}

HRESULT
nsTextStore::SetSelectionInternal(const TS_SELECTION_ACP* pSelection,
                                  bool aDispatchTextEvent)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::SetSelectionInternal(pSelection=%ld-%ld, "
          "aDispatchTextEvent=%s), %s",
          this, pSelection->acpStart, pSelection->acpEnd,
          GetBoolName(aDispatchTextEvent),
          mCompositionView ? "there is composition view" :
                             "there is no composition view"));

  if (mCompositionView) {
    if (aDispatchTextEvent) {
      HRESULT hr = UpdateCompositionExtent(nullptr);
      if (FAILED(hr)) {
        PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelectionInternal() FAILED due to "
            "UpdateCompositionExtent() failure", this));
        return hr;
      }
    }
    if (pSelection->acpStart < mCompositionStart ||
        pSelection->acpEnd >
          mCompositionStart + static_cast<LONG>(mCompositionString.Length())) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
         ("TSF: 0x%p   nsTextStore::SetSelectionInternal() FAILED due to "
          "the selection being out of the composition string", this));
      return TS_E_INVALIDPOS;
    }
    
    mCompositionSelection = *pSelection;
    if (aDispatchTextEvent) {
      HRESULT hr = SendTextEventForCompositionString();
      if (FAILED(hr)) {
        PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelectionInternal() FAILED due to "
            "SendTextEventForCompositionString() failure", this));
        return hr;
      }
    }
    return S_OK;
  } else {
    nsSelectionEvent event(true, NS_SELECTION_SET, mWidget);
    event.mOffset = pSelection->acpStart;
    event.mLength = uint32_t(pSelection->acpEnd - pSelection->acpStart);
    event.mReversed = pSelection->style.ase == TS_AE_START;
    mWidget->InitEvent(event);
    mWidget->DispatchWindowEvent(&event);
    if (!event.mSucceeded) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
         ("TSF: 0x%p   nsTextStore::SetSelectionInternal() FAILED due to "
          "NS_SELECTION_SET failure", this));
      return E_FAIL;
    }
  }
  return S_OK;
}

STDMETHODIMP
nsTextStore::SetSelection(ULONG ulCount,
                          const TS_SELECTION_ACP *pSelection)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::SetSelection(ulCount=%lu)",
          this, ulCount));

  if (!IsReadWriteLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelection() FAILED due to "
            "not locked (read-write)", this));
    return TS_E_NOLOCK;
  }
  if (ulCount != 1) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelection() FAILED due to "
            "trying setting multiple selection", this));
    return E_INVALIDARG;
  }
  if (!pSelection) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelection() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  HRESULT hr = SetSelectionInternal(pSelection, true);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetSelection() FAILED due to "
            "SetSelectionInternal() failure", this));
  } else {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::SetSelection() succeeded", this));
  }
  return hr;
}

STDMETHODIMP
nsTextStore::GetText(LONG acpStart,
                     LONG acpEnd,
                     WCHAR *pchPlain,
                     ULONG cchPlainReq,
                     ULONG *pcchPlainOut,
                     TS_RUNINFO *prgRunInfo,
                     ULONG ulRunInfoReq,
                     ULONG *pulRunInfoOut,
                     LONG *pacpNext)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetText(acpStart=%ld, acpEnd=%ld, pchPlain=0x%p, "
          "cchPlainReq=%lu, pcchPlainOut=0x%p, prgRunInfo=0x%p, ulRunInfoReq=%lu, "
          "pulRunInfoOut=0x%p, pacpNext=0x%p), %s, mCompositionStart=%ld, "
          "mCompositionLength=%ld, mCompositionString.Length()=%lu",
          this, acpStart, acpEnd, pchPlain, cchPlainReq, pcchPlainOut,
          prgRunInfo, ulRunInfoReq, pulRunInfoOut, pacpNext,
          mCompositionView ? "there is composition view" :
                             "there is no composition view",
          mCompositionStart, mCompositionLength,
          mCompositionString.Length()));

  if (!IsReadLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetText() FAILED due to "
            "not locked (read)", this));
    return TS_E_NOLOCK;
  }

  if (!pcchPlainOut || (!pchPlain && !prgRunInfo) ||
      !cchPlainReq != !pchPlain || !ulRunInfoReq != !prgRunInfo) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetText() FAILED due to "
            "invalid argument", this));
    return E_INVALIDARG;
  }

  if (acpStart < 0 || acpEnd < -1 || (acpEnd != -1 && acpStart > acpEnd)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetText() FAILED due to "
            "invalid position", this));
    return TS_E_INVALIDPOS;
  }

  
  *pcchPlainOut = 0;
  if (pchPlain && cchPlainReq) *pchPlain = 0;
  if (pulRunInfoOut) *pulRunInfoOut = 0;
  if (pacpNext) *pacpNext = acpStart;
  if (prgRunInfo && ulRunInfoReq) {
    prgRunInfo->uCount = 0;
    prgRunInfo->type = TS_RT_PLAIN;
  }
  uint32_t length = -1 == acpEnd ? UINT32_MAX : uint32_t(acpEnd - acpStart);
  if (cchPlainReq && cchPlainReq - 1 < length) {
    length = cchPlainReq - 1;
  }
  if (length) {
    LONG compNewStart = 0, compOldEnd = 0, compNewEnd = 0;
    if (mCompositionView) {
      
      
      
      
      compOldEnd = std::min(LONG(length) + acpStart,
                       mCompositionLength + mCompositionStart);
      compNewEnd = std::min(LONG(length) + acpStart,
                       LONG(mCompositionString.Length()) + mCompositionStart);
      compNewStart = std::max(acpStart, mCompositionStart);
      
      if (compOldEnd > compNewStart || compNewEnd > compNewStart) {
        NS_ASSERTION(compOldEnd >= mCompositionStart &&
            compNewEnd >= mCompositionStart, "Range end is less than start\n");
        length = uint32_t(LONG(length) + compOldEnd - compNewEnd);
      }
    }
    
    nsQueryContentEvent event(true, NS_QUERY_TEXT_CONTENT, mWidget);
    mWidget->InitEvent(event);
    event.InitForQueryTextContent(uint32_t(acpStart), length);
    mWidget->DispatchWindowEvent(&event);
    if (!event.mSucceeded) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetText() FAILED due to "
              "NS_QUERY_TEXT_CONTENT failure: length=%lu", this, length));
      return E_FAIL;
    }

    if (compOldEnd > compNewStart || compNewEnd > compNewStart) {
      
      const PRUnichar* compStrStart = mCompositionString.BeginReading() +
          std::max<LONG>(compNewStart - mCompositionStart, 0);
      event.mReply.mString.Replace(compNewStart - acpStart,
          compOldEnd - mCompositionStart, compStrStart,
          compNewEnd - mCompositionStart);
      length = uint32_t(LONG(length) - compOldEnd + compNewEnd);
    }
    if (-1 != acpEnd && event.mReply.mString.Length() != length) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetText() FAILED due to "
              "unexpected length=%lu", this, length));
      return TS_E_INVALIDPOS;
    }
    length = std::min(length, event.mReply.mString.Length());

    if (pchPlain && cchPlainReq) {
      memcpy(pchPlain, event.mReply.mString.BeginReading(),
             length * sizeof(*pchPlain));
      pchPlain[length] = 0;
      *pcchPlainOut = length;
    }
    if (prgRunInfo && ulRunInfoReq) {
      prgRunInfo->uCount = length;
      prgRunInfo->type = TS_RT_PLAIN;
      if (pulRunInfoOut) *pulRunInfoOut = 1;
    }
    if (pacpNext) *pacpNext = acpStart + length;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetText() succeeded: pcchPlainOut=0x%p, "
          "*prgRunInfo={ uCount=%lu, type=%s }, *pulRunInfoOut=%lu, "
          "*pacpNext=%ld)",
          this, pcchPlainOut, prgRunInfo ? prgRunInfo->uCount : 0,
          prgRunInfo ? GetTextRunTypeName(prgRunInfo->type) : "N/A",
          pulRunInfoOut ? pulRunInfoOut : 0, pacpNext ? pacpNext : 0));
  return S_OK;
}

STDMETHODIMP
nsTextStore::SetText(DWORD dwFlags,
                     LONG acpStart,
                     LONG acpEnd,
                     const WCHAR *pchText,
                     ULONG cch,
                     TS_TEXTCHANGE *pChange)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::SetText(dwFlags=%s, acpStart=%ld, acpEnd=%ld, "
          "pchText=0x%p \"%s\", cch=%lu, pChange=0x%p), %s",
          this, dwFlags == TS_ST_CORRECTION ? "TS_ST_CORRECTION" :
                                              "not-specified",
          acpStart, acpEnd, pchText,
          pchText && cch ?
            NS_ConvertUTF16toUTF8(pchText, cch).get() : "",
          cch, pChange,
          mCompositionView ? "there is composition view" :
                             "there is no composition view"));

  
  
  
  if (!IsReadWriteLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetText() FAILED due to "
            "not locked (read)", this));
    return TS_E_NOLOCK;
  }

  TS_SELECTION_ACP selection;
  selection.acpStart = acpStart;
  selection.acpEnd = acpEnd;
  selection.style.ase = TS_AE_END;
  selection.style.fInterimChar = 0;
  
  HRESULT hr = SetSelectionInternal(&selection);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetText() FAILED due to "
            "SetSelectionInternal() failure", this));
    return hr;
  }
  
  if (!InsertTextAtSelectionInternal(nsDependentSubstring(pchText, cch),
                                     pChange)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::SetText() FAILED due to "
            "InsertTextAtSelectionInternal() failure", this));
    return E_FAIL;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::SetText() succeeded: pChange={ "
          "acpStart=%ld, acpOldEnd=%ld, acpNewEnd=%ld }",
          this, pChange ? pChange->acpStart  : 0,
          pChange ? pChange->acpOldEnd : 0, pChange ? pChange->acpNewEnd : 0));
  return S_OK;
}

STDMETHODIMP
nsTextStore::GetFormattedText(LONG acpStart,
                              LONG acpEnd,
                              IDataObject **ppDataObject)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetFormattedText() called "
          "but not supported (E_NOTIMPL)", this));

  
  return E_NOTIMPL;
}

STDMETHODIMP
nsTextStore::GetEmbedded(LONG acpPos,
                         REFGUID rguidService,
                         REFIID riid,
                         IUnknown **ppunk)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetEmbedded() called "
          "but not supported (E_NOTIMPL)", this));

  
  return E_NOTIMPL;
}

STDMETHODIMP
nsTextStore::QueryInsertEmbedded(const GUID *pguidService,
                                 const FORMATETC *pFormatEtc,
                                 BOOL *pfInsertable)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::QueryInsertEmbedded() called "
          "but not supported, *pfInsertable=FALSE (S_OK)", this));

  
  *pfInsertable = FALSE;
  return S_OK;
}

STDMETHODIMP
nsTextStore::InsertEmbedded(DWORD dwFlags,
                            LONG acpStart,
                            LONG acpEnd,
                            IDataObject *pDataObject,
                            TS_TEXTCHANGE *pChange)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::InsertEmbedded() called "
          "but not supported (E_NOTIMPL)", this));

  
  return E_NOTIMPL;
}

void
nsTextStore::SetInputScope(const nsString& aHTMLInputType)
{
  mInputScopes.Clear();
  if (aHTMLInputType.IsEmpty() || aHTMLInputType.EqualsLiteral("text")) {
    return;
  }
  
  
  if (aHTMLInputType.EqualsLiteral("url")) {
    mInputScopes.AppendElement(IS_URL);
  } else if (aHTMLInputType.EqualsLiteral("search")) {
    mInputScopes.AppendElement(IS_SEARCH);
  } else if (aHTMLInputType.EqualsLiteral("email")) {
    mInputScopes.AppendElement(IS_EMAIL_SMTPEMAILADDRESS);
  } else if (aHTMLInputType.EqualsLiteral("password")) {
    mInputScopes.AppendElement(IS_PASSWORD);
  } else if (aHTMLInputType.EqualsLiteral("datetime") ||
             aHTMLInputType.EqualsLiteral("datetime-local")) {
    mInputScopes.AppendElement(IS_DATE_FULLDATE);
    mInputScopes.AppendElement(IS_TIME_FULLTIME);
  } else if (aHTMLInputType.EqualsLiteral("date") ||
             aHTMLInputType.EqualsLiteral("month") ||
             aHTMLInputType.EqualsLiteral("week")) {
    mInputScopes.AppendElement(IS_DATE_FULLDATE);
  } else if (aHTMLInputType.EqualsLiteral("time")) {
    mInputScopes.AppendElement(IS_TIME_FULLTIME);
  } else if (aHTMLInputType.EqualsLiteral("tel")) {
    mInputScopes.AppendElement(IS_TELEPHONE_FULLTELEPHONENUMBER);
    mInputScopes.AppendElement(IS_TELEPHONE_LOCALNUMBER);
  } else if (aHTMLInputType.EqualsLiteral("number")) {
    mInputScopes.AppendElement(IS_NUMBER);
  }
}

HRESULT
nsTextStore::ProcessScopeRequest(DWORD dwFlags,
                                 ULONG cFilterAttrs,
                                 const TS_ATTRID *paFilterAttrs)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::ProcessScopeRequest() called "
          "cFilterAttrs=%d dwFlags=%s", this, cFilterAttrs,
          GetFindFlagName(dwFlags).get()));

  
  
  
  
  
  
  mInputScopeDetected = mInputScopeRequested = false;

  
  for (uint32_t idx = 0; idx < cFilterAttrs; ++idx) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::ProcessScopeRequest() "
            "requested attr=%s", this, GetCLSIDNameStr(paFilterAttrs[idx]).get()));
    if (IsEqualGUID(paFilterAttrs[idx], GUID_PROP_INPUTSCOPE)) {
      PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
             ("TSF: 0x%p   nsTextStore::ProcessScopeRequest() "
              "GUID_PROP_INPUTSCOPE queried", this));
      mInputScopeDetected = true;
      if (dwFlags & TS_ATTR_FIND_WANT_VALUE) {
        PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
               ("TSF: 0x%p   nsTextStore::ProcessScopeRequest() "
                "TS_ATTR_FIND_WANT_VALUE specified", this));
        mInputScopeRequested = true;
      }
      break;
    }
  }
  return S_OK;
}

STDMETHODIMP
nsTextStore::RequestSupportedAttrs(DWORD dwFlags,
                                   ULONG cFilterAttrs,
                                   const TS_ATTRID *paFilterAttrs)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::RequestSupportedAttrs() called "
          "cFilterAttrs=%d dwFlags=%s", this, cFilterAttrs,
          GetFindFlagName(dwFlags).get()));

  return ProcessScopeRequest(dwFlags, cFilterAttrs, paFilterAttrs);
}

STDMETHODIMP
nsTextStore::RequestAttrsAtPosition(LONG acpPos,
                                    ULONG cFilterAttrs,
                                    const TS_ATTRID *paFilterAttrs,
                                    DWORD dwFlags)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::RequestAttrsAtPosition() called "
          "acpPos=%d cFilterAttrs=%d dwFlags=%s", this, acpPos, cFilterAttrs,
          GetFindFlagName(dwFlags).get()));

  return ProcessScopeRequest(dwFlags | TS_ATTR_FIND_WANT_VALUE,
                             cFilterAttrs, paFilterAttrs);
}

STDMETHODIMP
nsTextStore::RequestAttrsTransitioningAtPosition(LONG acpPos,
                                                 ULONG cFilterAttrs,
                                                 const TS_ATTRID *paFilterAttr,
                                                 DWORD dwFlags)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::RequestAttrsTransitioningAtPosition() called "
          "but not supported (S_OK)", this));

  
  return S_OK;
}

STDMETHODIMP
nsTextStore::FindNextAttrTransition(LONG acpStart,
                                    LONG acpHalt,
                                    ULONG cFilterAttrs,
                                    const TS_ATTRID *paFilterAttrs,
                                    DWORD dwFlags,
                                    LONG *pacpNext,
                                    BOOL *pfFound,
                                    LONG *plFoundOffset)
{
  if (!pacpNext || !pfFound || !plFoundOffset) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p nsTextStore::FindNextAttrTransition() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::FindNextAttrTransition() called "
          "but not supported (S_OK)", this));

  
  *pacpNext = *plFoundOffset = acpHalt;
  *pfFound = FALSE;
  return S_OK;
}

STDMETHODIMP
nsTextStore::RetrieveRequestedAttrs(ULONG ulCount,
                                    TS_ATTRVAL *paAttrVals,
                                    ULONG *pcFetched)
{
  if (!pcFetched || !ulCount || !paAttrVals) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p nsTextStore::RetrieveRequestedAttrs() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::RetrieveRequestedAttrs() called "
          "ulCount=%d", this, ulCount));

  if (mInputScopeDetected || mInputScopeRequested) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::RetrieveRequestedAttrs() for "
            "GUID_PROP_INPUTSCOPE: "
            "mInputScopeDetected=%s mInputScopeRequested=%s",
            this, GetBoolName(mInputScopeDetected),
            GetBoolName(mInputScopeRequested)));

    paAttrVals->idAttr = GUID_PROP_INPUTSCOPE;
    paAttrVals->dwOverlapId = 0;
    paAttrVals->varValue.vt = VT_EMPTY;
    *pcFetched = 1;

    if (mInputScopeRequested) {
      paAttrVals->varValue.vt = VT_UNKNOWN;
      paAttrVals->varValue.punkVal = (IUnknown*) new InputScopeImpl(mInputScopes);
    }

    mInputScopeDetected = mInputScopeRequested = false;
    return S_OK;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::RetrieveRequestedAttrs() called "
          "for unknown TS_ATTRVAL, *pcFetched=0 (S_OK)", this));

  paAttrVals->dwOverlapId = 0;
  paAttrVals->varValue.vt = VT_EMPTY;
  *pcFetched = 0;
  return S_OK;
}

STDMETHODIMP
nsTextStore::GetEndACP(LONG *pacp)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetEndACP(pacp=0x%p)", this, pacp));

  if (!IsReadLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetEndACP() FAILED due to "
            "not locked (read)", this));
    return TS_E_NOLOCK;
  }

  if (!pacp) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetEndACP() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  
  nsQueryContentEvent event(true, NS_QUERY_TEXT_CONTENT, mWidget);
  mWidget->InitEvent(event);
  
  event.InitForQueryTextContent(0, INT32_MAX);
  mWidget->DispatchWindowEvent(&event);
  if (!event.mSucceeded) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetEndACP() FAILED due to "
            "NS_QUERY_TEXT_CONTENT failure", this));
    return E_FAIL;
  }
  *pacp = LONG(event.mReply.mString.Length());
  return S_OK;
}

#define TEXTSTORE_DEFAULT_VIEW    (1)

STDMETHODIMP
nsTextStore::GetActiveView(TsViewCookie *pvcView)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetActiveView(pvcView=0x%p)", this, pvcView));

  if (!pvcView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetActiveView() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  *pvcView = TEXTSTORE_DEFAULT_VIEW;

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetActiveView() succeeded: *pvcView=%ld",
          this, *pvcView));
  return S_OK;
}

STDMETHODIMP
nsTextStore::GetACPFromPoint(TsViewCookie vcView,
                             const POINT *pt,
                             DWORD dwFlags,
                             LONG *pacp)
{
  if (!IsReadLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetACPFromPoint() FAILED due to "
            "not locked (read)", this));
    return TS_E_NOLOCK;
  }

  if (vcView != TEXTSTORE_DEFAULT_VIEW) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetACPFromPoint() FAILED due to "
            "called with invalid view", this));
    return E_INVALIDARG;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetACPFromPoint(vcView=%ld, "
          "pt(0x%p)={ x=%ld, y=%ld }, dwFlags=%s, pacp=0x%p) called "
          "but not supported (E_NOTIMPL)", this));

  
  return E_NOTIMPL;
}

STDMETHODIMP
nsTextStore::GetTextExt(TsViewCookie vcView,
                        LONG acpStart,
                        LONG acpEnd,
                        RECT *prc,
                        BOOL *pfClipped)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetTextExt(vcView=%ld, "
          "acpStart=%ld, acpEnd=%ld, prc=0x%p, pfClipped=0x%p)",
          this, vcView, acpStart, acpEnd, prc, pfClipped));

  if (!IsReadLocked()) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "not locked (read)", this));
    return TS_E_NOLOCK;
  }

  if (vcView != TEXTSTORE_DEFAULT_VIEW) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "called with invalid view", this));
    return E_INVALIDARG;
  }

  if (!prc || !pfClipped) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  if (acpStart < 0 || acpEnd < acpStart) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "invalid position", this));
    return TS_E_INVALIDPOS;
  }

  
  nsQueryContentEvent event(true, NS_QUERY_TEXT_RECT, mWidget);
  mWidget->InitEvent(event);
  event.InitForQueryTextRect(acpStart, acpEnd - acpStart);
  mWidget->DispatchWindowEvent(&event);
  if (!event.mSucceeded) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "NS_QUERY_TEXT_RECT failure", this));
    return TS_E_INVALIDPOS; 
  }
  
  if (event.mReply.mRect.width <= 0)
    event.mReply.mRect.width = 1;
  if (event.mReply.mRect.height <= 0)
    event.mReply.mRect.height = 1;

  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop) {
    
    nsWindow* refWindow = static_cast<nsWindow*>(
      event.mReply.mFocusedWidget ? event.mReply.mFocusedWidget : mWidget);
    
    refWindow = refWindow->GetTopLevelWindow(false);
    if (!refWindow) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
              "no top level window", this));
      return E_FAIL;
    }

    event.mReply.mRect.MoveBy(refWindow->WidgetToScreenOffset());
  }

  
  if (!GetScreenExtInternal(*prc)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetTextExt() FAILED due to "
            "GetScreenExtInternal() failure", this));
    return E_FAIL;
  }

  
  RECT textRect;
  ::SetRect(&textRect, event.mReply.mRect.x, event.mReply.mRect.y,
            event.mReply.mRect.XMost(), event.mReply.mRect.YMost());
  if (!::IntersectRect(prc, prc, &textRect))
    
    ::SetRectEmpty(prc);

  
  *pfClipped = !::EqualRect(prc, &textRect);

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetTextExt() succeeded: "
          "*prc={ left=%ld, top=%ld, right=%ld, bottom=%ld }, *pfClipped=%s",
          this, prc->left, prc->top, prc->right, prc->bottom,
          GetBoolName(*pfClipped)));

  return S_OK;
}

STDMETHODIMP
nsTextStore::GetScreenExt(TsViewCookie vcView,
                          RECT *prc)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetScreenExt(vcView=%ld, prc=0x%p)",
          this, vcView, prc));

  if (vcView != TEXTSTORE_DEFAULT_VIEW) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetScreenExt() FAILED due to "
            "called with invalid view", this));
    return E_INVALIDARG;
  }

  if (!prc) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetScreenExt() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  if (!GetScreenExtInternal(*prc)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetScreenExt() FAILED due to "
            "GetScreenExtInternal() failure", this));
    return E_FAIL;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetScreenExt() succeeded: "
          "*prc={ left=%ld, top=%ld, right=%ld, bottom=%ld }",
          this, prc->left, prc->top, prc->right, prc->bottom));
  return S_OK;
}

bool
nsTextStore::GetScreenExtInternal(RECT &aScreenExt)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::GetScreenExtInternal()", this));

  
  nsQueryContentEvent event(true, NS_QUERY_EDITOR_RECT, mWidget);
  mWidget->InitEvent(event);
  mWidget->DispatchWindowEvent(&event);
  if (!event.mSucceeded) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetScreenExtInternal() FAILED due to "
            "NS_QUERY_EDITOR_RECT failure", this));
    return false;
  }

  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    nsIntRect boundRect;
    if (NS_FAILED(mWidget->GetClientBounds(boundRect))) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetScreenExtInternal() FAILED due to "
              "failed to get the client bounds", this));
      return false;
    }
    ::SetRect(&aScreenExt, boundRect.x, boundRect.y,
              boundRect.XMost(), boundRect.YMost());
  } else {
    NS_ASSERTION(XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Desktop,
                 "environment isn't WindowsEnvironmentType_Desktop!");
    nsWindow* refWindow = static_cast<nsWindow*>(
      event.mReply.mFocusedWidget ?
        event.mReply.mFocusedWidget : mWidget);
    
    refWindow = refWindow->GetTopLevelWindow(false);
    if (!refWindow) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetScreenExtInternal() FAILED due to "
              "no top level window", this));
      return false;
    }

    nsIntRect boundRect;
    if (NS_FAILED(refWindow->GetClientBounds(boundRect))) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::GetScreenExtInternal() FAILED due to "
              "failed to get the client bounds", this));
      return false;
    }

    boundRect.MoveTo(0, 0);

    
    boundRect.IntersectRect(event.mReply.mRect, boundRect);
    if (!boundRect.IsEmpty()) {
      boundRect.MoveBy(refWindow->WidgetToScreenOffset());
      ::SetRect(&aScreenExt, boundRect.x, boundRect.y,
                boundRect.XMost(), boundRect.YMost());
    } else {
      ::SetRectEmpty(&aScreenExt);
    }
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::GetScreenExtInternal() succeeded: "
          "aScreenExt={ left=%ld, top=%ld, right=%ld, bottom=%ld }",
          this, aScreenExt.left, aScreenExt.top,
          aScreenExt.right, aScreenExt.bottom));
  return true;
}

STDMETHODIMP
nsTextStore::GetWnd(TsViewCookie vcView,
                    HWND *phwnd)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::GetWnd(vcView=%ld, phwnd=0x%p), "
          "mWidget=0x%p",
          this, vcView, phwnd, mWidget.get()));

  if (vcView != TEXTSTORE_DEFAULT_VIEW) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetWnd() FAILED due to "
            "called with invalid view", this));
    return E_INVALIDARG;
  }

  if (!phwnd) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::GetScreenExt() FAILED due to "
            "null argument", this));
    return E_INVALIDARG;
  }

  *phwnd = mWidget->GetWindowHandle();

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::GetWnd() succeeded: *phwnd=0x%p",
          this, static_cast<void*>(*phwnd)));
  return S_OK;
}

STDMETHODIMP
nsTextStore::InsertTextAtSelection(DWORD dwFlags,
                                   const WCHAR *pchText,
                                   ULONG cch,
                                   LONG *pacpStart,
                                   LONG *pacpEnd,
                                   TS_TEXTCHANGE *pChange)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::InsertTextAtSelection(dwFlags=%s, "
          "pchText=0x%p \"%s\", cch=%lu, pacpStart=0x%p, pacpEnd=0x%p, "
          "pChange=0x%p), %s",
          this, dwFlags == 0 ? "0" :
                dwFlags == TF_IAS_NOQUERY ? "TF_IAS_NOQUERY" :
                dwFlags == TF_IAS_QUERYONLY ? "TF_IAS_QUERYONLY" : "Unknown",
          pchText,
          pchText && cch ? NS_ConvertUTF16toUTF8(pchText, cch).get() : "",
          cch, pacpStart, pacpEnd, pChange,
          mCompositionView ? "there is composition view" :
                             "there is no composition view"));

  if (cch && !pchText) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
            "null pchText", this));
    return E_INVALIDARG;
  }

  if (TS_IAS_QUERYONLY == dwFlags) {
    if (!IsReadLocked()) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "not locked (read)", this));
      return TS_E_NOLOCK;
    }

    if (!pacpStart || !pacpEnd) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "null argument", this));
      return E_INVALIDARG;
    }

    
    TS_SELECTION_ACP sel;
    if (!GetSelectionInternal(sel)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "GetSelectionInternal() failure", this));
      return E_FAIL;
    }

    
    *pacpStart = sel.acpStart;
    *pacpEnd = sel.acpEnd;
    if (pChange) {
      pChange->acpStart = sel.acpStart;
      pChange->acpOldEnd = sel.acpEnd;
      pChange->acpNewEnd = sel.acpStart + cch;
    }
  } else {
    if (!IsReadWriteLocked()) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "not locked (read-write)", this));
      return TS_E_NOLOCK;
    }

    if (!pChange) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "null pChange", this));
      return E_INVALIDARG;
    }

    if (TS_IAS_NOQUERY != dwFlags && (!pacpStart || !pacpEnd)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "null argument", this));
      return E_INVALIDARG;
    }

    if (!InsertTextAtSelectionInternal(nsDependentSubstring(pchText, cch),
                                       pChange)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() FAILED due to "
              "InsertTextAtSelectionInternal() failure", this));
      return E_FAIL;
    }

    if (TS_IAS_NOQUERY != dwFlags) {
      *pacpStart = pChange->acpStart;
      *pacpEnd = pChange->acpNewEnd;
    }
  }
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::InsertTextAtSelection() succeeded: "
          "*pacpStart=%ld, *pacpEnd=%ld, "
          "*pChange={ acpStart=%ld, acpOldEnd=%ld, acpNewEnd=%ld })",
          this, pacpStart ? *pacpStart : 0, pacpEnd ? *pacpEnd : 0,
          pChange ? pChange->acpStart: 0, pChange ? pChange->acpOldEnd : 0,
          pChange ? pChange->acpNewEnd : 0));
  return S_OK;
}

bool
nsTextStore::InsertTextAtSelectionInternal(const nsAString &aInsertStr,
                                           TS_TEXTCHANGE* aTextChange)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal("
          "aInsertStr=\"%s\", aTextChange=0x%p), %s",
          this, NS_ConvertUTF16toUTF8(aInsertStr).get(), aTextChange,
          mCompositionView ? "there is composition view" :
                             "there is no composition view"));

  TS_SELECTION_ACP oldSelection;
  oldSelection.acpStart = 0;
  oldSelection.acpEnd = 0;

  if (mCompositionView) {
    oldSelection = mCompositionSelection;
    
    
    
    
    
    mCompositionString.Replace(
      static_cast<uint32_t>(oldSelection.acpStart) - mCompositionStart,
      static_cast<uint32_t>(oldSelection.acpEnd - oldSelection.acpStart),
      aInsertStr);

    mCompositionSelection.acpStart += aInsertStr.Length();
    mCompositionSelection.acpEnd = mCompositionSelection.acpStart;
    mCompositionSelection.style.ase = TS_AE_END;
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() replaced "
            "a part of (%lu-%lu) the composition string, waiting "
            "SetSelection() or OnUpdateComposition()...", this,
            oldSelection.acpStart - mCompositionStart,
            oldSelection.acpEnd - mCompositionStart));
  } else {
    
    if (aTextChange && !GetSelectionInternal(oldSelection)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() FAILED "
              "due to GetSelectionInternal() failure", this));
      return false;
    }

    
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() "
            "dispatching a compositionstart event...", this));
    nsCompositionEvent compStartEvent(true, NS_COMPOSITION_START,
                                      mWidget);
    mWidget->InitEvent(compStartEvent);
    mWidget->DispatchWindowEvent(&compStartEvent);
    if (!mWidget || mWidget->Destroyed()) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() FAILED "
              "due to the widget destroyed by compositionstart event", this));
      return false;
    }

    if (!aInsertStr.IsEmpty()) {
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() "
            "dispatching a compositionupdate event...", this));
      nsCompositionEvent compUpdateEvent(true, NS_COMPOSITION_UPDATE,
                                         mWidget);
      compUpdateEvent.data = aInsertStr;
      mWidget->DispatchWindowEvent(&compUpdateEvent);
      if (!mWidget || mWidget->Destroyed()) {
        PR_LOG(sTextStoreLog, PR_LOG_ERROR,
               ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() "
                "FAILED due to the widget destroyed by compositionupdate event",
                this));
        return false;
      }
    }

    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() "
            "dispatching a text event...", this));
    nsTextEvent textEvent(true, NS_TEXT_TEXT, mWidget);
    mWidget->InitEvent(textEvent);
    textEvent.theText = aInsertStr;
    textEvent.theText.ReplaceSubstring(NS_LITERAL_STRING("\r\n"),
                                       NS_LITERAL_STRING("\n"));
    mWidget->DispatchWindowEvent(&textEvent);
    if (!mWidget || mWidget->Destroyed()) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() FAILED "
              "due to the widget destroyed by text event", this));
      return false;
    }

    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() "
            "dispatching a compositionend event...", this));
    nsCompositionEvent compEndEvent(true, NS_COMPOSITION_END, mWidget);
    compEndEvent.data = aInsertStr;
    mWidget->DispatchWindowEvent(&compEndEvent);
    if (!mWidget || mWidget->Destroyed()) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() FAILED "
              "due to the widget destroyed by compositionend event", this));
      return false;
    }
  }

  if (aTextChange) {
    aTextChange->acpStart = oldSelection.acpStart;
    aTextChange->acpOldEnd = oldSelection.acpEnd;

    
    TS_SELECTION_ACP newSelection;
    if (!GetSelectionInternal(newSelection)) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
             ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() FAILED "
              "due to GetSelectionInternal() failure after inserted the text",
              this));
      return false;
    }
    aTextChange->acpNewEnd = newSelection.acpEnd;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::InsertTextAtSelectionInternal() succeeded: "
          "mWidget=0x%p, mWidget->Destroyed()=%s, aTextChange={ acpStart=%ld, "
          "acpOldEnd=%ld, acpNewEnd=%ld }",
          this, mWidget.get(),
          GetBoolName(mWidget ? mWidget->Destroyed() : true),
          aTextChange ? aTextChange->acpStart : 0,
          aTextChange ? aTextChange->acpOldEnd : 0,
          aTextChange ? aTextChange->acpNewEnd : 0));
  return true;
}

STDMETHODIMP
nsTextStore::InsertEmbeddedAtSelection(DWORD dwFlags,
                                       IDataObject *pDataObject,
                                       LONG *pacpStart,
                                       LONG *pacpEnd,
                                       TS_TEXTCHANGE *pChange)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::InsertEmbeddedAtSelection() called "
          "but not supported (E_NOTIMPL)", this));

  
  return E_NOTIMPL;
}

HRESULT
nsTextStore::OnStartCompositionInternal(ITfCompositionView* pComposition,
                                        ITfRange* aRange,
                                        bool aPreserveSelection)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal("
          "pComposition=0x%p, aRange=0x%p, aPreserveSelection=%s), "
          "mCompositionView=0x%p",
          this, pComposition, aRange, GetBoolName(aPreserveSelection),
          mCompositionView.get()));

  mCompositionView = pComposition;
  HRESULT hr = GetRangeExtent(aRange, &mCompositionStart, &mCompositionLength);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal() FAILED due "
            "to GetRangeExtent() failure", this));
    return hr;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal(), "
          "mCompositionStart=%ld, mCompositionLength=%ld",
          this, mCompositionStart, mCompositionLength));

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal(), "
          "dispatching selectionset event..."));

  
  nsSelectionEvent selEvent(true, NS_SELECTION_SET, mWidget);
  mWidget->InitEvent(selEvent);
  selEvent.mOffset = uint32_t(mCompositionStart);
  selEvent.mLength = uint32_t(mCompositionLength);
  selEvent.mReversed = false;
  mWidget->DispatchWindowEvent(&selEvent);
  if (!selEvent.mSucceeded) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal() FAILED due "
            "to NS_SELECTION_SET failure", this));
    return E_FAIL;
  }

  
  nsQueryContentEvent queryEvent(true, NS_QUERY_SELECTED_TEXT, mWidget);
  mWidget->InitEvent(queryEvent);
  mWidget->DispatchWindowEvent(&queryEvent);
  if (!queryEvent.mSucceeded) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal() FAILED due "
            "to NS_QUERY_SELECTED_TEXT failure", this));
    return E_FAIL;
  }

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal(), "
          "dispatching compositionstart event..."));

  mCompositionString = queryEvent.mReply.mString;
  if (!aPreserveSelection) {
    mCompositionSelection.acpStart = mCompositionStart;
    mCompositionSelection.acpEnd = mCompositionStart + mCompositionLength;
    mCompositionSelection.style.ase = TS_AE_END;
    mCompositionSelection.style.fInterimChar = FALSE;
  }
  nsCompositionEvent event(true, NS_COMPOSITION_START, mWidget);
  mWidget->InitEvent(event);
  mWidget->DispatchWindowEvent(&event);

  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p   nsTextStore::OnStartCompositionInternal() succeeded: "
          "mCompositionStart=%ld, mCompositionLength=%ld, "
          "mCompositionSelection={ acpStart=%ld, acpEnd=%ld, style.ase=%s, "
          "style.iInterimChar=%s }",
          this, mCompositionStart, mCompositionLength,
          mCompositionSelection.acpStart, mCompositionSelection.acpEnd,
          GetActiveSelEndName(mCompositionSelection.style.ase),
          GetBoolName(mCompositionSelection.style.fInterimChar)));
  return S_OK;
}

static uint32_t
GetLayoutChangeIntervalTime()
{
  static int32_t sTime = -1;
  if (sTime > 0)
    return uint32_t(sTime);

  sTime = std::max(10,
    Preferences::GetInt("intl.tsf.on_layout_change_interval", 100));
  return uint32_t(sTime);
}

STDMETHODIMP
nsTextStore::OnStartComposition(ITfCompositionView* pComposition,
                                BOOL* pfOk)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::OnStartComposition(pComposition=0x%p, "
          "pfOk=0x%p), mCompositionView=0x%p",
          this, pComposition, pfOk, mCompositionView.get()));

  *pfOk = FALSE;

  
  if (mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartComposition() FAILED due to "
            "there is another composition already (but returns S_OK)", this));
    return S_OK;
  }

  nsRefPtr<ITfRange> range;
  HRESULT hr = pComposition->GetRange(getter_AddRefs(range));
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartComposition() FAILED due to "
            "pComposition->GetRange() failure", this));
    return hr;
  }
  hr = OnStartCompositionInternal(pComposition, range, false);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnStartComposition() FAILED due to "
            "OnStartCompositionInternal() failure", this));
    return hr;
  }

  NS_ASSERTION(!mCompositionTimer, "The timer is alive!");
  mCompositionTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (mCompositionTimer) {
    mCompositionTimer->InitWithFuncCallback(CompositionTimerCallbackFunc, this,
                                            GetLayoutChangeIntervalTime(),
                                            nsITimer::TYPE_REPEATING_SLACK);
  }
  *pfOk = TRUE;
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnStartComposition() succeeded", this));
  return S_OK;
}

STDMETHODIMP
nsTextStore::OnUpdateComposition(ITfCompositionView* pComposition,
                                 ITfRange* pRangeNew)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::OnUpdateComposition(pComposition=0x%p, "
          "pRangeNew=0x%p), mCompositionView=0x%p",
          this, pComposition, pRangeNew, mCompositionView.get()));

  if (!mDocumentMgr || !mContext) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() FAILED due to "
            "not ready for the composition", this));
    return E_UNEXPECTED;
  }
  if (!mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() FAILED due to "
            "no active composition", this));
    return E_UNEXPECTED;
  }
  if (mCompositionView != pComposition) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() FAILED due to "
            "different composition view specified", this));
    return E_UNEXPECTED;
  }

  
  if (!pRangeNew) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() succeeded but "
            "not complete", this));
    return S_OK;
  }

  HRESULT hr = UpdateCompositionExtent(pRangeNew);
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() FAILED due to "
            "UpdateCompositionExtent() failure", this));
    return hr;
  }

  hr = SendTextEventForCompositionString();
  if (FAILED(hr)) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnUpdateComposition() FAILED due to "
            "SendTextEventForCompositionString() failure", this));
    return hr;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnUpdateComposition() succeeded: "
          "mCompositionStart=%ld, mCompositionLength=%ld, "
          "mCompositionSelection={ acpStart=%ld, acpEnd=%ld, style.ase=%s, "
          "style.iInterimChar=%s }, mCompositionString=\"%s\"",
          this, mCompositionStart, mCompositionLength,
          mCompositionSelection.acpStart, mCompositionSelection.acpEnd,
          GetActiveSelEndName(mCompositionSelection.style.ase),
          GetBoolName(mCompositionSelection.style.fInterimChar),
          NS_ConvertUTF16toUTF8(mCompositionString).get()));
  return S_OK;
}

STDMETHODIMP
nsTextStore::OnEndComposition(ITfCompositionView* pComposition)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p nsTextStore::OnEndComposition(pComposition=0x%p), "
          "mCompositionView=0x%p, mCompositionString=\"%s\"",
          this, pComposition, mCompositionView.get(),
          NS_ConvertUTF16toUTF8(mCompositionString).get()));

  if (!mCompositionView) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnEndComposition() FAILED due to "
            "no active composition", this));
    return E_UNEXPECTED;
  }

  if (mCompositionView != pComposition) {
    PR_LOG(sTextStoreLog, PR_LOG_ERROR,
           ("TSF: 0x%p   nsTextStore::OnEndComposition() FAILED due to "
            "different composition view specified", this));
    return E_UNEXPECTED;
  }

  
  SaveTextEvent(nullptr);

  if (mCompositionTimer) {
    mCompositionTimer->Cancel();
    mCompositionTimer = nullptr;
  }

  if (mCompositionString != mLastDispatchedCompositionString) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
            "dispatching compositionupdate event...", this));
    nsCompositionEvent compositionUpdate(true, NS_COMPOSITION_UPDATE,
                                         mWidget);
    mWidget->InitEvent(compositionUpdate);
    compositionUpdate.data = mCompositionString;
    mLastDispatchedCompositionString = mCompositionString;
    mWidget->DispatchWindowEvent(&compositionUpdate);
    if (!mWidget || mWidget->Destroyed()) {
      PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
             ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
              "succeeded, but the widget has gone", this));
      return S_OK;
    }
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
          "dispatching text event...", this));

  
  nsTextEvent textEvent(true, NS_TEXT_TEXT, mWidget);
  mWidget->InitEvent(textEvent);
  textEvent.theText = mCompositionString;
  textEvent.theText.ReplaceSubstring(NS_LITERAL_STRING("\r\n"),
                                     NS_LITERAL_STRING("\n"));
  mWidget->DispatchWindowEvent(&textEvent);

  if (!mWidget || mWidget->Destroyed()) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
            "succeeded, but the widget has gone", this));
    return S_OK;
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
          "dispatching compositionend event...", this));

  nsCompositionEvent event(true, NS_COMPOSITION_END, mWidget);
  event.data = mLastDispatchedCompositionString;
  mWidget->InitEvent(event);
  mWidget->DispatchWindowEvent(&event);

  if (!mWidget || mWidget->Destroyed()) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnEndComposition(), "
            "succeeded, but the widget has gone", this));
    return S_OK;
  }

  mCompositionView = NULL;
  mCompositionString.Truncate(0);
  mLastDispatchedCompositionString.Truncate();

  
  SetSelectionInternal(&mCompositionSelection);

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnEndComposition(), succeeded", this));
  return S_OK;
}


nsresult
nsTextStore::OnFocusChange(bool aGotFocus,
                           nsWindowBase* aFocusedWidget,
                           IMEState::Enabled aIMEEnabled)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: nsTextStore::OnFocusChange(aGotFocus=%s, "
          "aFocusedWidget=0x%p, aIMEEnabled=%s), sTsfThreadMgr=0x%p, "
          "sTsfTextStore=0x%p",
          GetBoolName(aGotFocus), aFocusedWidget,
          GetIMEEnabledName(aIMEEnabled), sTsfThreadMgr, sTsfTextStore));

  
  if (!sTsfThreadMgr || !sTsfTextStore) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (aGotFocus) {
    bool bRet = sTsfTextStore->Create(aFocusedWidget, aIMEEnabled);
    NS_ENSURE_TRUE(bRet, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(sTsfTextStore->mDocumentMgr, NS_ERROR_FAILURE);
    HRESULT hr = sTsfThreadMgr->SetFocus(sTsfTextStore->mDocumentMgr);
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
  } else {
    sTsfTextStore->Destroy();
  }
  return NS_OK;
}


nsIMEUpdatePreference
nsTextStore::GetIMEUpdatePreference()
{
  bool hasFocus = false;
  if (sTsfThreadMgr && sTsfTextStore && sTsfTextStore->mDocumentMgr) {
    nsRefPtr<ITfDocumentMgr> docMgr;
    sTsfThreadMgr->GetFocus(getter_AddRefs(docMgr));
    hasFocus = (docMgr == sTsfTextStore->mDocumentMgr);
  }
  return nsIMEUpdatePreference(hasFocus, false);
}

nsresult
nsTextStore::OnTextChangeInternal(uint32_t aStart,
                                  uint32_t aOldEnd,
                                  uint32_t aNewEnd)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p nsTextStore::OnTextChangeInternal(aStart=%lu, "
          "aOldEnd=%lu, aNewEnd=%lu), mLock=%s, mSink=0x%p, mSinkMask=%s, "
          "mTextChange={ acpStart=%ld, acpOldEnd=%ld, acpNewEnd=%ld }",
          this, aStart, aOldEnd, aNewEnd, GetLockFlagNameStr(mLock).get(),
          mSink.get(), GetSinkMaskNameStr(mSinkMask).get(),
          mTextChange.acpStart, mTextChange.acpOldEnd, mTextChange.acpNewEnd));

  if (!mLock && mSink && 0 != (mSinkMask & TS_AS_TEXT_CHANGE)) {
    mTextChange.acpStart = std::min(mTextChange.acpStart, LONG(aStart));
    mTextChange.acpOldEnd = std::max(mTextChange.acpOldEnd, LONG(aOldEnd));
    mTextChange.acpNewEnd = std::max(mTextChange.acpNewEnd, LONG(aNewEnd));
    ::PostMessageW(mWidget->GetWindowHandle(),
                   WM_USER_TSF_TEXTCHANGE, 0, 0);
  }
  return NS_OK;
}

void
nsTextStore::OnTextChangeMsgInternal(void)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p nsTextStore::OnTextChangeMsgInternal(), mLock=%s, "
          "mSink=0x%p, mSinkMask=%s, mTextChange={ acpStart=%ld, "
          "acpOldEnd=%ld, acpNewEnd=%ld }",
          this, GetLockFlagNameStr(mLock).get(), mSink.get(),
          GetSinkMaskNameStr(mSinkMask).get(), mTextChange.acpStart,
          mTextChange.acpOldEnd, mTextChange.acpNewEnd));

  if (!mLock && mSink && 0 != (mSinkMask & TS_AS_TEXT_CHANGE) &&
      INT32_MAX > mTextChange.acpStart) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnTextChangeMsgInternal(), calling"
            "mSink->OnTextChange(0, { acpStart=%ld, acpOldEnd=%ld, "
            "acpNewEnd=%ld })...", this, mTextChange.acpStart,
            mTextChange.acpOldEnd, mTextChange.acpNewEnd));
    mSink->OnTextChange(0, &mTextChange);
    mTextChange.acpStart = INT32_MAX;
    mTextChange.acpOldEnd = mTextChange.acpNewEnd = 0;
  }
}

nsresult
nsTextStore::OnSelectionChangeInternal(void)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p nsTextStore::OnSelectionChangeInternal(), mLock=%s, "
          "mSink=0x%p, mSinkMask=%s",
          this, GetLockFlagNameStr(mLock).get(), mSink.get(),
          GetSinkMaskNameStr(mSinkMask).get()));

  if (!mLock && mSink && 0 != (mSinkMask & TS_AS_SEL_CHANGE)) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
           ("TSF: 0x%p   nsTextStore::OnSelectionChangeInternal(), calling "
            "mSink->OnSelectionChange()...", this));
    mSink->OnSelectionChange();
  }
  return NS_OK;
}

nsresult
nsTextStore::OnCompositionTimer()
{
  NS_ENSURE_TRUE(mContext, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSink, NS_ERROR_FAILURE);

  
  
  
  
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
         ("TSF: 0x%p   nsTextStore::OnCompositionTimer(), calling "
          "mSink->OnLayoutChange()...", this));
  HRESULT hr = mSink->OnLayoutChange(TS_LC_CHANGE, TEXTSTORE_DEFAULT_VIEW);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  return NS_OK;
}

void
nsTextStore::CommitCompositionInternal(bool aDiscard)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p nsTextStore::CommitCompositionInternal(aDiscard=%s), "
          "mLock=%s, mSink=0x%p, mContext=0x%p, mCompositionView=0x%p, "
          "mCompositionString=\"%s\"",
          this, GetBoolName(aDiscard), GetLockFlagNameStr(mLock).get(),
          mSink.get(), mContext.get(), mCompositionView.get(),
          NS_ConvertUTF16toUTF8(mCompositionString).get()));

  if (mCompositionView && aDiscard) {
    mCompositionString.Truncate(0);
    if (mSink && !mLock) {
      TS_TEXTCHANGE textChange;
      textChange.acpStart = mCompositionStart;
      textChange.acpOldEnd = mCompositionStart + mCompositionLength;
      textChange.acpNewEnd = mCompositionStart;
      PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
             ("TSF: 0x%p   nsTextStore::CommitCompositionInternal(), calling"
              "mSink->OnTextChange(0, { acpStart=%ld, acpOldEnd=%ld, "
              "acpNewEnd=%ld })...", this, textChange.acpStart,
              textChange.acpOldEnd, textChange.acpNewEnd));
      mSink->OnTextChange(0, &textChange);
    }
  }
  
  
  nsRefPtr<ITfContext> context = mContext;
  do {
    if (context) {
      nsRefPtr<ITfContextOwnerCompositionServices> services;
      context->QueryInterface(IID_ITfContextOwnerCompositionServices,
                              getter_AddRefs(services));
      if (services) {
        PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
               ("TSF: 0x%p   nsTextStore::CommitCompositionInternal(), "
                "requesting TerminateComposition() for the context 0x%p...",
                this, context.get()));
        services->TerminateComposition(NULL);
      }
    }
    if (context != mContext)
      break;
    if (mDocumentMgr)
      mDocumentMgr->GetTop(getter_AddRefs(context));
  } while (context != mContext);
}

static
bool
GetCompartment(IUnknown* pUnk,
               const GUID& aID,
               ITfCompartment** aCompartment)
{
  if (!pUnk) return false;

  nsRefPtr<ITfCompartmentMgr> compMgr;
  pUnk->QueryInterface(IID_ITfCompartmentMgr, getter_AddRefs(compMgr));
  if (!compMgr) return false;

  return SUCCEEDED(compMgr->GetCompartment(aID, aCompartment)) &&
         (*aCompartment) != NULL;
}


void
nsTextStore::SetIMEOpenState(bool aState)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: nsTextStore::SetIMEOpenState(aState=%s)", GetBoolName(aState)));

  nsRefPtr<ITfCompartment> comp;
  if (!GetCompartment(sTsfThreadMgr,
                      GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                      getter_AddRefs(comp))) {
    PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
           ("TSF:   nsTextStore::SetIMEOpenState() FAILED due to"
            "no compartment available"));
    return;
  }

  VARIANT variant;
  variant.vt = VT_I4;
  variant.lVal = aState;
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF:   nsTextStore::SetIMEOpenState(), setting "
          "0x%04X to GUID_COMPARTMENT_KEYBOARD_OPENCLOSE...",
          variant.lVal));
  comp->SetValue(sTsfClientId, &variant);
}


bool
nsTextStore::GetIMEOpenState(void)
{
  nsRefPtr<ITfCompartment> comp;
  if (!GetCompartment(sTsfThreadMgr,
                      GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                      getter_AddRefs(comp)))
    return false;

  VARIANT variant;
  ::VariantInit(&variant);
  if (SUCCEEDED(comp->GetValue(&variant)) && variant.vt == VT_I4)
    return variant.lVal != 0;

  ::VariantClear(&variant); 
  return false;
}

void
nsTextStore::SetInputContextInternal(IMEState::Enabled aState)
{
  PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
         ("TSF: 0x%p nsTextStore::SetInputContextInternal(aState=%s), "
          "mContext=0x%p",
          this, GetIMEEnabledName(aState), mContext.get()));

  VARIANT variant;
  variant.vt = VT_I4;
  variant.lVal = (aState != IMEState::ENABLED);

  
  
  nsRefPtr<ITfContext> context = mContext;
  nsRefPtr<ITfCompartment> comp;
  do {
    if (GetCompartment(context, GUID_COMPARTMENT_KEYBOARD_DISABLED,
                       getter_AddRefs(comp))) {
      PR_LOG(sTextStoreLog, PR_LOG_DEBUG,
             ("TSF: 0x%p   nsTextStore::SetInputContextInternal(), setting "
              "0x%04X to GUID_COMPARTMENT_KEYBOARD_DISABLED of context 0x%p...",
              this, variant.lVal, context.get()));
      comp->SetValue(sTsfClientId, &variant);
    }

    if (context != mContext)
      break;
    if (mDocumentMgr)
      mDocumentMgr->GetTop(getter_AddRefs(context));
  } while (context != mContext);
}


void
nsTextStore::Initialize(void)
{
#ifdef PR_LOGGING
  if (!sTextStoreLog) {
    sTextStoreLog = PR_NewLogModule("nsTextStoreWidgets");
  }
#endif

  bool enableTsf = Preferences::GetBool("intl.enable_tsf_support", false);
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF: nsTextStore::Initialize(), TSF is %s",
     enableTsf ? "enabled" : "disabled"));
  if (!enableTsf) {
    return;
  }

  if (!sTsfThreadMgr) {
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, NULL,
          CLSCTX_INPROC_SERVER, IID_ITfThreadMgr,
          reinterpret_cast<void**>(&sTsfThreadMgr)))) {
      PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
        ("TSF:   nsTextStore::Initialize() succeeded to "
         "create the thread manager, activating..."));
      if (FAILED(sTsfThreadMgr->Activate(&sTsfClientId))) {
        PR_LOG(sTextStoreLog, PR_LOG_ERROR,
          ("TSF:   nsTextStore::Initialize() FAILED to activate, "
           "releasing the thread manager..."));
        NS_RELEASE(sTsfThreadMgr);
      }
    }
#ifdef PR_LOGGING
    else {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
        ("TSF:   nsTextStore::Initialize() FAILED to "
         "create the thread manager"));
    }
#endif 
  }
  if (sTsfThreadMgr && !sTsfTextStore) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF:   nsTextStore::Initialize() is creating "
       "an nsTextStore instance..."));
    sTsfTextStore = new nsTextStore();
  }
  if (sTsfThreadMgr && !sDisplayAttrMgr) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF:   nsTextStore::Initialize() is creating "
       "a display attribute manager instance..."));
    HRESULT hr =
      ::CoCreateInstance(CLSID_TF_DisplayAttributeMgr, NULL,
                         CLSCTX_INPROC_SERVER, IID_ITfDisplayAttributeMgr,
                         reinterpret_cast<void**>(&sDisplayAttrMgr));
    if (FAILED(hr) || !sDisplayAttrMgr) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
        ("TSF:   nsTextStore::Initialize() FAILED to create "
         "a display attribute manager instance"));
    }
  }
  if (sTsfThreadMgr && sDisplayAttrMgr && !sCategoryMgr) {
    PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
      ("TSF:   nsTextStore::Initialize() is creating "
       "a category manager instance..."));
    HRESULT hr =
      ::CoCreateInstance(CLSID_TF_CategoryMgr, NULL,
                         CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                         reinterpret_cast<void**>(&sCategoryMgr));
    if (FAILED(hr) || !sCategoryMgr) {
      PR_LOG(sTextStoreLog, PR_LOG_ERROR,
        ("TSF:   nsTextStore::Initialize() FAILED to create "
         "a category manager instance"));
      
      
      NS_RELEASE(sDisplayAttrMgr);
    }
  }
  if (sTsfThreadMgr && !sFlushTIPInputMessage) {
    sFlushTIPInputMessage = ::RegisterWindowMessageW(
        NS_LITERAL_STRING("Flush TIP Input Message").get());
  }

  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS,
    ("TSF:   nsTextStore::Initialize(), sTsfThreadMgr=0x%p, "
     "sTsfClientId=0x%08X, sTsfTextStore=0x%p, sDisplayAttrMgr=0x%p, "
     "sCategoryMgr=0x%p",
     sTsfThreadMgr, sTsfClientId, sTsfTextStore,
     sDisplayAttrMgr, sCategoryMgr));
}


void
nsTextStore::Terminate(void)
{
  PR_LOG(sTextStoreLog, PR_LOG_ALWAYS, ("TSF: nsTextStore::Terminate()"));

  NS_IF_RELEASE(sDisplayAttrMgr);
  NS_IF_RELEASE(sCategoryMgr);
  NS_IF_RELEASE(sTsfTextStore);
  sTsfClientId = 0;
  if (sTsfThreadMgr) {
    sTsfThreadMgr->Deactivate();
    NS_RELEASE(sTsfThreadMgr);
  }
}
