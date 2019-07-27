




#include "mozilla/ArrayUtils.h"

#include <ole2.h>
#include <shlobj.h>

#include "nsDataObj.h"
#include "nsClipboard.h"
#include "nsReadableUtils.h"
#include "nsITransferable.h"
#include "nsISupportsPrimitives.h"
#include "IEnumFE.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPIDLString.h"
#include "nsImageClipboard.h"
#include "nsCRT.h"
#include "nsPrintfCString.h"
#include "nsIStringBundle.h"
#include "nsEscape.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "mozilla/Services.h"
#include "nsIOutputStream.h"
#include "nsXPCOMStrings.h"
#include "nscore.h"
#include "nsDirectoryServiceDefs.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "mozilla/Preferences.h"
#include "nsIContentPolicy.h"
#include "nsContentUtils.h"
#include "nsINode.h"

#include "WinUtils.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/WindowsVersion.h"
#include <algorithm>


using namespace mozilla;
using namespace mozilla::widget;

#define DEFAULT_THREAD_TIMEOUT_MS 30000

NS_IMPL_ISUPPORTS(nsDataObj::CStream, nsIStreamListener)



nsDataObj::CStream::CStream() :
  mChannelRead(false),
  mStreamRead(0)
{
}


nsDataObj::CStream::~CStream()
{
}



nsresult nsDataObj::CStream::Init(nsIURI *pSourceURI,
                                  nsINode* aRequestingNode)
{
  
  if (!aRequestingNode) {
    return NS_ERROR_FAILURE;
  }
  nsresult rv;
  rv = NS_NewChannel(getter_AddRefs(mChannel),
                     pSourceURI,
                     aRequestingNode,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER,
                     nullptr,   
                     nullptr,   
                     nsIRequest::LOAD_FROM_CACHE);

  NS_ENSURE_SUCCESS(rv, rv);
  rv = mChannel->AsyncOpen(this, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}




STDMETHODIMP nsDataObj::CStream::QueryInterface(REFIID refiid, void** ppvResult)
{
  *ppvResult = nullptr;
  if (IID_IUnknown == refiid ||
      refiid == IID_IStream)

  {
    *ppvResult = this;
  }

  if (nullptr != *ppvResult)
  {
    ((LPUNKNOWN)*ppvResult)->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}


NS_IMETHODIMP
nsDataObj::CStream::OnDataAvailable(nsIRequest *aRequest,
                                    nsISupports *aContext,
                                    nsIInputStream *aInputStream,
                                    uint64_t aOffset, 
                                    uint32_t aCount) 
{
    
    uint8_t* buffer = mChannelData.AppendElements(aCount);
    if (buffer == nullptr)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ASSERTION((mChannelData.Length() == (aOffset + aCount)),
      "stream length mismatch w/write buffer");

    
    
    nsresult rv;
    uint32_t odaBytesReadTotal = 0;
    do {
      uint32_t bytesReadByCall = 0;
      rv = aInputStream->Read((char*)(buffer + odaBytesReadTotal),
                              aCount, &bytesReadByCall);
      odaBytesReadTotal += bytesReadByCall;
    } while (aCount < odaBytesReadTotal && NS_SUCCEEDED(rv));
    return rv;
}

NS_IMETHODIMP nsDataObj::CStream::OnStartRequest(nsIRequest *aRequest,
                                                 nsISupports *aContext)
{
    mChannelResult = NS_OK;
    return NS_OK;
}

NS_IMETHODIMP nsDataObj::CStream::OnStopRequest(nsIRequest *aRequest,
                                                nsISupports *aContext,
                                                nsresult aStatusCode)
{
    mChannelRead = true;
    mChannelResult = aStatusCode;
    return NS_OK;
}




nsresult nsDataObj::CStream::WaitForCompletion()
{
  
  while (!mChannelRead) {
    
    NS_ProcessNextEvent(nullptr, true);
  }

  if (!mChannelData.Length())
    mChannelResult = NS_ERROR_FAILURE;

  return mChannelResult;
}



STDMETHODIMP nsDataObj::CStream::Clone(IStream** ppStream)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Commit(DWORD dwFrags)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::CopyTo(IStream* pDestStream,
                                        ULARGE_INTEGER nBytesToCopy,
                                        ULARGE_INTEGER* nBytesRead,
                                        ULARGE_INTEGER* nBytesWritten)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::LockRegion(ULARGE_INTEGER nStart,
                                            ULARGE_INTEGER nBytes,
                                            DWORD dwFlags)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Read(void* pvBuffer,
                                      ULONG nBytesToRead,
                                      ULONG* nBytesRead)
{
  
  
  if (NS_FAILED(WaitForCompletion()))
    return E_FAIL;

  
  ULONG bytesLeft = mChannelData.Length() - mStreamRead;
  
  *nBytesRead = std::min(bytesLeft, nBytesToRead);
  
  memcpy(pvBuffer, ((char*)mChannelData.Elements() + mStreamRead), *nBytesRead);
  
  mStreamRead += *nBytesRead;
  return S_OK;
}


STDMETHODIMP nsDataObj::CStream::Revert(void)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Seek(LARGE_INTEGER nMove,
                                      DWORD dwOrigin,
                                      ULARGE_INTEGER* nNewPos)
{
  if (nNewPos == nullptr)
    return STG_E_INVALIDPOINTER;

  if (nMove.LowPart == 0 && nMove.HighPart == 0 &&
      (dwOrigin == STREAM_SEEK_SET || dwOrigin == STREAM_SEEK_CUR)) { 
    nNewPos->LowPart = 0;
    nNewPos->HighPart = 0;
    return S_OK;
  }

  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::SetSize(ULARGE_INTEGER nNewSize)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Stat(STATSTG* statstg, DWORD dwFlags)
{
  if (statstg == nullptr)
    return STG_E_INVALIDPOINTER;

  if (!mChannel || NS_FAILED(WaitForCompletion()))
    return E_FAIL;

  memset((void*)statstg, 0, sizeof(STATSTG));

  if (dwFlags != STATFLAG_NONAME) 
  {
    nsCOMPtr<nsIURI> sourceURI;
    if (NS_FAILED(mChannel->GetURI(getter_AddRefs(sourceURI)))) {
      return E_FAIL;
    }

    nsAutoCString strFileName;
    nsCOMPtr<nsIURL> sourceURL = do_QueryInterface(sourceURI);
    sourceURL->GetFileName(strFileName);

    if (strFileName.IsEmpty())
      return E_FAIL;

    NS_UnescapeURL(strFileName);
    NS_ConvertUTF8toUTF16 wideFileName(strFileName);

    uint32_t nMaxNameLength = (wideFileName.Length()*2) + 2;
    void * retBuf = CoTaskMemAlloc(nMaxNameLength); 
    if (!retBuf) 
      return STG_E_INSUFFICIENTMEMORY;

    ZeroMemory(retBuf, nMaxNameLength);
    memcpy(retBuf, wideFileName.get(), wideFileName.Length()*2);
    statstg->pwcsName = (LPOLESTR)retBuf;
  }

  SYSTEMTIME st;

  statstg->type = STGTY_STREAM;

  GetSystemTime(&st);
  SystemTimeToFileTime((const SYSTEMTIME*)&st, (LPFILETIME)&statstg->mtime);
  statstg->ctime = statstg->atime = statstg->mtime;

  statstg->cbSize.LowPart = (DWORD)mChannelData.Length();
  statstg->grfMode = STGM_READ;
  statstg->grfLocksSupported = LOCK_ONLYONCE;
  statstg->clsid = CLSID_NULL;

  return S_OK;
}


STDMETHODIMP nsDataObj::CStream::UnlockRegion(ULARGE_INTEGER nStart,
                                              ULARGE_INTEGER nBytes,
                                              DWORD dwFlags)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Write(const void* pvBuffer,
                                       ULONG nBytesToRead,
                                       ULONG* nBytesRead)
{
  return E_NOTIMPL;
}


HRESULT nsDataObj::CreateStream(IStream **outStream)
{
  NS_ENSURE_TRUE(outStream, E_INVALIDARG);

  nsresult rv = NS_ERROR_FAILURE;
  nsAutoString wideFileName;
  nsCOMPtr<nsIURI> sourceURI;
  HRESULT res;

  res = GetDownloadDetails(getter_AddRefs(sourceURI),
                           wideFileName);
  if(FAILED(res))
    return res;

  nsDataObj::CStream *pStream = new nsDataObj::CStream();
  NS_ENSURE_TRUE(pStream, E_OUTOFMEMORY);

  pStream->AddRef();

  
  nsCOMPtr<nsIDOMNode> requestingDomNode;
  mTransferable->GetRequestingNode(getter_AddRefs(requestingDomNode));
  nsCOMPtr<nsINode> requestingNode = do_QueryInterface(requestingDomNode);
  MOZ_ASSERT(requestingNode, "can not create channel without a node");

  rv = pStream->Init(sourceURI, requestingNode);
  if (NS_FAILED(rv))
  {
    pStream->Release();
    return E_FAIL;
  }
  *outStream = pStream;

  return S_OK;
}

static GUID CLSID_nsDataObj =
	{ 0x1bba7640, 0xdf52, 0x11cf, { 0x82, 0x7b, 0, 0xa0, 0x24, 0x3a, 0xe5, 0x05 } };







#define NS_MAX_FILEDESCRIPTOR 128 + 1








nsDataObj::nsDataObj(nsIURI * uri)
  : m_cRef(0), mTransferable(nullptr),
    mIsAsyncMode(FALSE), mIsInOperation(FALSE)
{
  mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS, 
                                 NS_LITERAL_CSTRING("nsDataObj"),
                                 LazyIdleThread::ManualShutdown);
  m_enumFE = new CEnumFormatEtc();
  m_enumFE->AddRef();

  if (uri) {
    
    
    uri->GetSpec(mSourceURL);
  }
}



nsDataObj::~nsDataObj()
{
  NS_IF_RELEASE(mTransferable);

  mDataFlavors.Clear();

  m_enumFE->Release();

  
  for (uint32_t idx = 0; idx < mDataEntryList.Length(); idx++) {
      CoTaskMemFree(mDataEntryList[idx]->fe.ptd);
      ReleaseStgMedium(&mDataEntryList[idx]->stgm);
      CoTaskMemFree(mDataEntryList[idx]);
  }
}





STDMETHODIMP nsDataObj::QueryInterface(REFIID riid, void** ppv)
{
	*ppv=nullptr;

	if ( (IID_IUnknown == riid) || (IID_IDataObject	== riid) ) {
		*ppv = this;
		AddRef();
		return S_OK;
  } else if (IID_IAsyncOperation == riid) {
    *ppv = static_cast<IAsyncOperation*>(this);
    AddRef();
    return S_OK;
  }

	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) nsDataObj::AddRef()
{
	++m_cRef;
	NS_LOG_ADDREF(this, m_cRef, "nsDataObj", sizeof(*this));
	return m_cRef;
}



STDMETHODIMP_(ULONG) nsDataObj::Release()
{
	--m_cRef;
	
	NS_LOG_RELEASE(this, m_cRef, "nsDataObj");
	if (0 != m_cRef)
		return m_cRef;

  
  
  
  
  if (mCachedTempFile) {
    nsresult rv;
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      mTimer->InitWithFuncCallback(nsDataObj::RemoveTempFile, this,
                                   500, nsITimer::TYPE_ONE_SHOT);
      return AddRef();
    }
    mCachedTempFile->Remove(false);
    mCachedTempFile = nullptr;
  }

	delete this;

	return 0;
}


BOOL nsDataObj::FormatsMatch(const FORMATETC& source, const FORMATETC& target) const
{
  if ((source.cfFormat == target.cfFormat) &&
      (source.dwAspect & target.dwAspect) &&
      (source.tymed & target.tymed)) {
    return TRUE;
  } else {
    return FALSE;
  }
}




STDMETHODIMP nsDataObj::GetData(LPFORMATETC aFormat, LPSTGMEDIUM pSTM)
{
  if (!mTransferable)
    return DV_E_FORMATETC;

  uint32_t dfInx = 0;

  static CLIPFORMAT fileDescriptorFlavorA = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORA ); 
  static CLIPFORMAT fileDescriptorFlavorW = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORW ); 
  static CLIPFORMAT uniformResourceLocatorA = ::RegisterClipboardFormat( CFSTR_INETURLA );
  static CLIPFORMAT uniformResourceLocatorW = ::RegisterClipboardFormat( CFSTR_INETURLW );
  static CLIPFORMAT fileFlavor = ::RegisterClipboardFormat( CFSTR_FILECONTENTS ); 
  static CLIPFORMAT PreferredDropEffect = ::RegisterClipboardFormat( CFSTR_PREFERREDDROPEFFECT );

  
  
  
  LPDATAENTRY pde;
  if (LookupArbitraryFormat(aFormat, &pde, FALSE)) {
    return CopyMediumData(pSTM, &pde->stgm, aFormat, FALSE)
           ? S_OK : E_UNEXPECTED;
  }

  
  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)
         && dfInx < mDataFlavors.Length()) {
    nsCString& df = mDataFlavors.ElementAt(dfInx);
    if (FormatsMatch(fe, *aFormat)) {
      pSTM->pUnkForRelease = nullptr;     
      CLIPFORMAT format = aFormat->cfFormat;
      switch(format) {

      
      case CF_TEXT:
      case CF_UNICODETEXT:
      return GetText(df, *aFormat, *pSTM);

      
      
      case CF_HDROP:
        return GetFile(*aFormat, *pSTM);

      
      case CF_DIBV5:
      case CF_DIB:
        return GetDib(df, *aFormat, *pSTM);

      default:
        if ( format == fileDescriptorFlavorA )
          return GetFileDescriptor ( *aFormat, *pSTM, false );
        if ( format == fileDescriptorFlavorW )
          return GetFileDescriptor ( *aFormat, *pSTM, true);
        if ( format == uniformResourceLocatorA )
          return GetUniformResourceLocator( *aFormat, *pSTM, false);
        if ( format == uniformResourceLocatorW )
          return GetUniformResourceLocator( *aFormat, *pSTM, true);
        if ( format == fileFlavor )
          return GetFileContents ( *aFormat, *pSTM );
        if ( format == PreferredDropEffect )
          return GetPreferredDropEffect( *aFormat, *pSTM );
        
        
        return GetText(df, *aFormat, *pSTM);
      } 
    } 
    dfInx++;
  } 

  return DATA_E_FORMATETC;
}


STDMETHODIMP nsDataObj::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  return E_FAIL;
}






STDMETHODIMP nsDataObj::QueryGetData(LPFORMATETC pFE)
{
  
  
  
  LPDATAENTRY pde;
  if (LookupArbitraryFormat(pFE, &pde, FALSE))
    return S_OK;

  
  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)) {
    if (fe.cfFormat == pFE->cfFormat) {
      return S_OK;
    }
  }
  return E_FAIL;
}


STDMETHODIMP nsDataObj::GetCanonicalFormatEtc
	 (LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::SetData(LPFORMATETC aFormat, LPSTGMEDIUM aMedium, BOOL shouldRel)
{
  
  
  
  LPDATAENTRY pde;
  if (LookupArbitraryFormat(aFormat, &pde, TRUE)) {
    
    
    
    if (pde->stgm.tymed) {
      ReleaseStgMedium(&pde->stgm);
      memset(&pde->stgm, 0, sizeof(STGMEDIUM));
    }

    bool result = true;
    if (shouldRel) {
      
      
      
      
      pde->stgm = *aMedium;
    } else {
      
      
      result = CopyMediumData(&pde->stgm, aMedium, aFormat, TRUE);
    }
    pde->fe.tymed = pde->stgm.tymed;

    return result ? S_OK : DV_E_TYMED;
  }

  if (shouldRel)
    ReleaseStgMedium(aMedium);

  return S_OK;
}

bool
nsDataObj::LookupArbitraryFormat(FORMATETC *aFormat, LPDATAENTRY *aDataEntry, BOOL aAddorUpdate)
{
  *aDataEntry = nullptr;

  if (aFormat->ptd != nullptr)
    return false;

  
  for (uint32_t idx = 0; idx < mDataEntryList.Length(); idx++) {
    if (mDataEntryList[idx]->fe.cfFormat == aFormat->cfFormat &&
        mDataEntryList[idx]->fe.dwAspect == aFormat->dwAspect &&
        mDataEntryList[idx]->fe.lindex == aFormat->lindex) {
      if (aAddorUpdate || (mDataEntryList[idx]->fe.tymed & aFormat->tymed)) {
        
        
        *aDataEntry = mDataEntryList[idx];
        return true;
      } else {
        
        return false;
      }
    }
  }

  if (!aAddorUpdate)
    return false;

  
  LPDATAENTRY dataEntry = (LPDATAENTRY)CoTaskMemAlloc(sizeof(DATAENTRY));
  if (!dataEntry)
    return false;
  
  dataEntry->fe = *aFormat;
  *aDataEntry = dataEntry;
  memset(&dataEntry->stgm, 0, sizeof(STGMEDIUM));

  
  
  m_enumFE->AddFormatEtc(aFormat);

  
  mDataEntryList.AppendElement(dataEntry);

  return true;
}

bool
nsDataObj::CopyMediumData(STGMEDIUM *aMediumDst, STGMEDIUM *aMediumSrc, LPFORMATETC aFormat, BOOL aSetData)
{
  STGMEDIUM stgmOut = *aMediumSrc;
  
  switch (stgmOut.tymed) {
    case TYMED_ISTREAM:
      stgmOut.pstm->AddRef();
    break;
    case TYMED_ISTORAGE:
      stgmOut.pstg->AddRef();
    break;
    case TYMED_HGLOBAL:
      if (!aMediumSrc->pUnkForRelease) {
        if (aSetData) {
          if (aMediumSrc->tymed != TYMED_HGLOBAL)
            return false;
          stgmOut.hGlobal = OleDuplicateData(aMediumSrc->hGlobal, aFormat->cfFormat, 0);
          if (!stgmOut.hGlobal)
            return false;
        } else {
          
          
          stgmOut.pUnkForRelease = static_cast<IDataObject*>(this);
        }
      }
    break;
    default:
      return false;
  }

  if (stgmOut.pUnkForRelease)
    stgmOut.pUnkForRelease->AddRef();

  *aMediumDst = stgmOut;

  return true;
}


STDMETHODIMP nsDataObj::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
  switch (dwDir) {
    case DATADIR_GET:
      m_enumFE->Clone(ppEnum);
      break;
    case DATADIR_SET:
      
    default:
      *ppEnum = nullptr;
  } 

  if (nullptr == *ppEnum)
    return E_FAIL;

  (*ppEnum)->Reset();
  
  return NOERROR;
}


STDMETHODIMP nsDataObj::DAdvise(LPFORMATETC pFE, DWORD dwFlags,
										            LPADVISESINK pIAdviseSink, DWORD* pdwConn)
{
  return OLE_E_ADVISENOTSUPPORTED;
}



STDMETHODIMP nsDataObj::DUnadvise(DWORD dwConn)
{
  return OLE_E_ADVISENOTSUPPORTED;
}


STDMETHODIMP nsDataObj::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
  return OLE_E_ADVISENOTSUPPORTED;
}


STDMETHODIMP nsDataObj::EndOperation(HRESULT hResult,
                                     IBindCtx *pbcReserved,
                                     DWORD dwEffects)
{
  mIsInOperation = FALSE;
  return S_OK;
}

STDMETHODIMP nsDataObj::GetAsyncMode(BOOL *pfIsOpAsync)
{
  *pfIsOpAsync = mIsAsyncMode;

  return S_OK;
}

STDMETHODIMP nsDataObj::InOperation(BOOL *pfInAsyncOp)
{
  *pfInAsyncOp = mIsInOperation;

  return S_OK;
}

STDMETHODIMP nsDataObj::SetAsyncMode(BOOL fDoOpAsync)
{
  mIsAsyncMode = fDoOpAsync;
  return S_OK;
}

STDMETHODIMP nsDataObj::StartOperation(IBindCtx *pbcReserved)
{
  mIsInOperation = TRUE;
  return S_OK;
}




HRESULT nsDataObj::AddSetFormat(FORMATETC& aFE)
{
  return S_OK;
}


HRESULT nsDataObj::AddGetFormat(FORMATETC& aFE)
{
  return S_OK;
}







HRESULT 
nsDataObj::GetDib(const nsACString& inFlavor,
                  FORMATETC &aFormat,
                  STGMEDIUM & aSTG)
{
  ULONG result = E_FAIL;
  uint32_t len = 0;
  nsCOMPtr<nsISupports> genericDataWrapper;
  mTransferable->GetTransferData(PromiseFlatCString(inFlavor).get(), getter_AddRefs(genericDataWrapper), &len);
  nsCOMPtr<imgIContainer> image ( do_QueryInterface(genericDataWrapper) );
  if ( !image ) {
    
    
    
    nsCOMPtr<nsISupportsInterfacePointer> ptr(do_QueryInterface(genericDataWrapper));
    if ( ptr ) {
      nsCOMPtr<nsISupports> supports;
      ptr->GetData(getter_AddRefs(supports));
      image = do_QueryInterface(supports);
    }
  }
  
  if ( image ) {
    
    
    nsImageToClipboard converter(image, aFormat.cfFormat == CF_DIBV5);
    HANDLE bits = nullptr;
    nsresult rv = converter.GetPicture ( &bits );
    if ( NS_SUCCEEDED(rv) && bits ) {
      aSTG.hGlobal = bits;
      aSTG.tymed = TYMED_HGLOBAL;
      result = S_OK;
    }
  } 
  else  
    NS_WARNING ( "Definitely not an image on clipboard" );
	return result;
}







HRESULT 
nsDataObj :: GetFileDescriptor ( FORMATETC& aFE, STGMEDIUM& aSTG, bool aIsUnicode )
{
  HRESULT res = S_OK;
  
  
  
  if (IsFlavourPresent(kFilePromiseMime) ||
      IsFlavourPresent(kFileMime))
  {
    if (aIsUnicode)
      return GetFileDescriptor_IStreamW(aFE, aSTG);
    else
      return GetFileDescriptor_IStreamA(aFE, aSTG);
  }
  else if (IsFlavourPresent(kURLMime))
  {
    if ( aIsUnicode )
      res = GetFileDescriptorInternetShortcutW ( aFE, aSTG );
    else
      res = GetFileDescriptorInternetShortcutA ( aFE, aSTG );
  }
  else
    NS_WARNING ( "Not yet implemented\n" );
  
	return res;
} 



HRESULT 
nsDataObj :: GetFileContents ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
  HRESULT res = S_OK;
  
  
  
  if (IsFlavourPresent(kFilePromiseMime) ||
      IsFlavourPresent(kFileMime))
    return GetFileContents_IStream(aFE, aSTG);
  else if (IsFlavourPresent(kURLMime))
    return GetFileContentsInternetShortcut ( aFE, aSTG );
  else
    NS_WARNING ( "Not yet implemented\n" );

	return res;
	
} 









static void
MangleTextToValidFilename(nsString & aText)
{
  static const char* forbiddenNames[] = {
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", 
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    "CON", "PRN", "AUX", "NUL", "CLOCK$"
  };

  aText.StripChars(FILE_PATH_SEPARATOR  FILE_ILLEGAL_CHARACTERS);
  aText.CompressWhitespace(true, true);
  uint32_t nameLen;
  for (size_t n = 0; n < ArrayLength(forbiddenNames); ++n) {
    nameLen = (uint32_t) strlen(forbiddenNames[n]);
    if (aText.EqualsIgnoreCase(forbiddenNames[n], nameLen)) {
      
      if (aText.Length() == nameLen || aText.CharAt(nameLen) == char16_t('.')) {
        aText.Truncate();
        break;
      }
    }
  }
}








static bool
CreateFilenameFromTextA(nsString & aText, const char * aExtension, 
                         char * aFilename, uint32_t aFilenameLen)
{
  
  
  
  MangleTextToValidFilename(aText);
  if (aText.IsEmpty())
    return false;

  
  
  
  
  
  int maxUsableFilenameLen = aFilenameLen - strlen(aExtension) - 1; 
  int currLen, textLen = (int) std::min(aText.Length(), aFilenameLen);
  char defaultChar = '_';
  do {
    currLen = WideCharToMultiByte(CP_ACP, 
      WC_COMPOSITECHECK|WC_DEFAULTCHAR,
      aText.get(), textLen--, aFilename, maxUsableFilenameLen, &defaultChar, nullptr);
  }
  while (currLen == 0 && textLen > 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  if (currLen > 0 && textLen > 0) {
    strcpy(&aFilename[currLen], aExtension);
    return true;
  }
  else {
    
    return false;
  }
}

static bool
CreateFilenameFromTextW(nsString & aText, const wchar_t * aExtension, 
                         wchar_t * aFilename, uint32_t aFilenameLen)
{
  
  
  
  MangleTextToValidFilename(aText);
  if (aText.IsEmpty())
    return false;

  const int extensionLen = wcslen(aExtension);
  if (aText.Length() + extensionLen + 1 > aFilenameLen)
    aText.Truncate(aFilenameLen - extensionLen - 1);
  wcscpy(&aFilename[0], aText.get());
  wcscpy(&aFilename[aText.Length()], aExtension);
  return true;
}

#define PAGEINFO_PROPERTIES "chrome://navigator/locale/pageInfo.properties"

static bool
GetLocalizedString(const char16_t * aName, nsXPIDLString & aString)
{
  nsCOMPtr<nsIStringBundleService> stringService =
    mozilla::services::GetStringBundleService();
  if (!stringService)
    return false;

  nsCOMPtr<nsIStringBundle> stringBundle;
  nsresult rv = stringService->CreateBundle(PAGEINFO_PROPERTIES,
                                            getter_AddRefs(stringBundle));
  if (NS_FAILED(rv))
    return false;

  rv = stringBundle->GetStringFromName(aName, getter_Copies(aString));
  return NS_SUCCEEDED(rv);
}







HRESULT
nsDataObj :: GetFileDescriptorInternetShortcutA ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
  
  nsAutoString title;
  if ( NS_FAILED(ExtractShortcutTitle(title)) )
    return E_OUTOFMEMORY;

  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORA));
  if (!fileGroupDescHandle)
    return E_OUTOFMEMORY;

  LPFILEGROUPDESCRIPTORA fileGroupDescA = reinterpret_cast<LPFILEGROUPDESCRIPTORA>(::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescA) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  
  
  if (!CreateFilenameFromTextA(title, ".URL", 
                               fileGroupDescA->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
    nsXPIDLString untitled;
    if (!GetLocalizedString(MOZ_UTF16("noPageTitle"), untitled) ||
        !CreateFilenameFromTextA(untitled, ".URL", 
                                 fileGroupDescA->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
      strcpy(fileGroupDescA->fgd[0].cFileName, "Untitled.URL");
    }
  }

  
  fileGroupDescA->cItems = 1;
  fileGroupDescA->fgd[0].dwFlags = FD_LINKUI;

  ::GlobalUnlock( fileGroupDescHandle );
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
} 

HRESULT
nsDataObj :: GetFileDescriptorInternetShortcutW ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
  
  nsAutoString title;
  if ( NS_FAILED(ExtractShortcutTitle(title)) )
    return E_OUTOFMEMORY;

  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  if (!fileGroupDescHandle)
    return E_OUTOFMEMORY;

  LPFILEGROUPDESCRIPTORW fileGroupDescW = reinterpret_cast<LPFILEGROUPDESCRIPTORW>(::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescW) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  
  
  if (!CreateFilenameFromTextW(title, L".URL",
                               fileGroupDescW->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
    nsXPIDLString untitled;
    if (!GetLocalizedString(MOZ_UTF16("noPageTitle"), untitled) ||
        !CreateFilenameFromTextW(untitled, L".URL",
                                 fileGroupDescW->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
      wcscpy(fileGroupDescW->fgd[0].cFileName, L"Untitled.URL");
    }
  }

  
  fileGroupDescW->cItems = 1;
  fileGroupDescW->fgd[0].dwFlags = FD_LINKUI;

  ::GlobalUnlock( fileGroupDescHandle );
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
} 








HRESULT
nsDataObj :: GetFileContentsInternetShortcut ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
  static const char * kShellIconPref = "browser.shell.shortcutFavicons";
  nsAutoString url;
  if ( NS_FAILED(ExtractShortcutURL(url)) )
    return E_OUTOFMEMORY;

  
  nsAutoCString asciiUrl;
  LossyCopyUTF16toASCII(url, asciiUrl);

  nsCOMPtr<nsIFile> icoFile;
  nsCOMPtr<nsIURI> aUri;
  NS_NewURI(getter_AddRefs(aUri), url);

  const char *shortcutFormatStr;
  int totalLen;
  nsCString path;
  if (!Preferences::GetBool(kShellIconPref, true) ||
      !IsVistaOrLater()) {
    shortcutFormatStr = "[InternetShortcut]\r\nURL=%s\r\n";
    const int formatLen = strlen(shortcutFormatStr) - 2;  
    totalLen = formatLen + asciiUrl.Length();  
  } else {
    nsCOMPtr<nsIFile> icoFile;
    nsCOMPtr<nsIURI> aUri;
    NS_NewURI(getter_AddRefs(aUri), url);

    nsAutoString aUriHash;

    mozilla::widget::FaviconHelper::ObtainCachedIconFile(aUri, aUriHash, mIOThread, true);

    nsresult rv = mozilla::widget::FaviconHelper::GetOutputIconPath(aUri, icoFile, true);
    NS_ENSURE_SUCCESS(rv, E_FAIL);
    rv = icoFile->GetNativePath(path);
    NS_ENSURE_SUCCESS(rv, E_FAIL);

    shortcutFormatStr = "[InternetShortcut]\r\nURL=%s\r\n"
                        "IDList=\r\nHotKey=0\r\nIconFile=%s\r\n"
                        "IconIndex=0\r\n";
    const int formatLen = strlen(shortcutFormatStr) - 2 * 2; 
    totalLen = formatLen + asciiUrl.Length() +
               path.Length(); 
  }

  
  HGLOBAL hGlobalMemory = ::GlobalAlloc(GMEM_SHARE, totalLen);
  if ( !hGlobalMemory )
    return E_OUTOFMEMORY;

  char* contents = reinterpret_cast<char*>(::GlobalLock(hGlobalMemory));
  if ( !contents ) {
    ::GlobalFree( hGlobalMemory );
    return E_OUTOFMEMORY;
  }
    
  
  
  
  

  if (!Preferences::GetBool(kShellIconPref, true)) {
    _snprintf(contents, totalLen, shortcutFormatStr, asciiUrl.get());
  } else {
    _snprintf(contents, totalLen, shortcutFormatStr, asciiUrl.get(), path.get());
  }

  ::GlobalUnlock(hGlobalMemory);
  aSTG.hGlobal = hGlobalMemory;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
} 


bool nsDataObj :: IsFlavourPresent(const char *inFlavour)
{
  bool retval = false;
  NS_ENSURE_TRUE(mTransferable, false);
  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  mTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
  NS_ENSURE_TRUE(flavorList, false);

  
  uint32_t cnt;
  flavorList->Count(&cnt);
  for (uint32_t i = 0; i < cnt; ++i) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt (i, getter_AddRefs(genericFlavor));
    nsCOMPtr<nsISupportsCString> currentFlavor (do_QueryInterface(genericFlavor));
    if (currentFlavor) {
      nsAutoCString flavorStr;
      currentFlavor->GetData(flavorStr);
      if (flavorStr.Equals(inFlavour)) {
        retval = true;         
        break;
      }
    }
  } 

  return retval;
}

HRESULT nsDataObj::GetPreferredDropEffect ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
  HRESULT res = S_OK;
  aSTG.tymed = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = nullptr;    
  HGLOBAL hGlobalMemory = nullptr;
  hGlobalMemory = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
  if (hGlobalMemory) {
    DWORD* pdw = (DWORD*) GlobalLock(hGlobalMemory);
    
    
    
    
    
    
    
    *pdw = (DWORD) DROPEFFECT_MOVE;
    GlobalUnlock(hGlobalMemory);
  }
  else {
    res = E_OUTOFMEMORY;
  }
  aSTG.hGlobal = hGlobalMemory;
  return res;
}


HRESULT nsDataObj::GetText(const nsACString & aDataFlavor, FORMATETC& aFE, STGMEDIUM& aSTG)
{
  void* data = nullptr;
  uint32_t   len;
  
  
  const char* flavorStr;
  const nsPromiseFlatCString& flat = PromiseFlatCString(aDataFlavor);
  if (aDataFlavor.EqualsLiteral("text/plain"))
    flavorStr = kUnicodeMime;
  else
    flavorStr = flat.get();

  
  nsCOMPtr<nsISupports> genericDataWrapper;
  mTransferable->GetTransferData(flavorStr, getter_AddRefs(genericDataWrapper), &len);
  if ( !len )
    return E_FAIL;
  nsPrimitiveHelpers::CreateDataFromPrimitive ( flavorStr, genericDataWrapper, &data, len );
  if ( !data )
    return E_FAIL;

  HGLOBAL     hGlobalMemory = nullptr;

  aSTG.tymed          = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = nullptr;

  
  
  
  
  
  
  
  
  DWORD allocLen = (DWORD)len;
  if ( aFE.cfFormat == CF_TEXT ) {
    
    
    size_t bufferSize = sizeof(char)*(len + 2);
    char* plainTextData = static_cast<char*>(moz_xmalloc(bufferSize));
    char16_t* castedUnicode = reinterpret_cast<char16_t*>(data);
    int32_t plainTextLen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)castedUnicode, len / 2 + 1, plainTextData, bufferSize, NULL, NULL);
    
    
    free(data);
    if ( plainTextLen ) {
      data = plainTextData;
      allocLen = plainTextLen;
    }
    else {
      free(plainTextData);
      NS_WARNING ( "Oh no, couldn't convert unicode to plain text" );
      return S_OK;
    }
  }
  else if ( aFE.cfFormat == nsClipboard::CF_HTML ) {
    
    
    NS_ConvertUTF16toUTF8 converter ( reinterpret_cast<char16_t*>(data) );
    char* utf8HTML = nullptr;
    nsresult rv = BuildPlatformHTML ( converter.get(), &utf8HTML );      
    
    free(data);
    if ( NS_SUCCEEDED(rv) && utf8HTML ) {
      
      data = utf8HTML;
      allocLen = strlen(utf8HTML) + sizeof(char);
    }
    else {
      NS_WARNING ( "Oh no, couldn't convert to HTML" );
      return S_OK;
    }
  }
  else {
    
    
    allocLen += sizeof(char16_t);
  }

  hGlobalMemory = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, allocLen);

  
  if ( hGlobalMemory ) {
    char* dest = reinterpret_cast<char*>(GlobalLock(hGlobalMemory));
    char* source = reinterpret_cast<char*>(data);
    memcpy ( dest, source, allocLen );                         
    GlobalUnlock(hGlobalMemory);
  }
  aSTG.hGlobal = hGlobalMemory;

  
  free(data);

  return S_OK;
}


HRESULT nsDataObj::GetFile(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  uint32_t dfInx = 0;
  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)
    && dfInx < mDataFlavors.Length()) {
      if (mDataFlavors[dfInx].EqualsLiteral(kNativeImageMime))
        return DropImage(aFE, aSTG);
      if (mDataFlavors[dfInx].EqualsLiteral(kFileMime))
        return DropFile(aFE, aSTG);
      if (mDataFlavors[dfInx].EqualsLiteral(kFilePromiseMime))
        return DropTempFile(aFE, aSTG);
      dfInx++;
  }
  return E_FAIL;
}

HRESULT nsDataObj::DropFile(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  nsresult rv;
  uint32_t len = 0;
  nsCOMPtr<nsISupports> genericDataWrapper;

  mTransferable->GetTransferData(kFileMime, getter_AddRefs(genericDataWrapper),
                                 &len);
  nsCOMPtr<nsIFile> file ( do_QueryInterface(genericDataWrapper) );

  if (!file)
  {
    nsCOMPtr<nsISupportsInterfacePointer> ptr(do_QueryInterface(genericDataWrapper));
    if (ptr) {
      nsCOMPtr<nsISupports> supports;
      ptr->GetData(getter_AddRefs(supports));
      file = do_QueryInterface(supports);
    }
  }

  if (!file)
    return E_FAIL;

  aSTG.tymed = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = nullptr;

  nsAutoString path;
  rv = file->GetPath(path);
  if (NS_FAILED(rv))
    return E_FAIL;

  uint32_t allocLen = path.Length() + 2;
  HGLOBAL hGlobalMemory = nullptr;
  char16_t *dest;

  hGlobalMemory = GlobalAlloc(GMEM_MOVEABLE, sizeof(DROPFILES) +
                                             allocLen * sizeof(char16_t));
  if (!hGlobalMemory)
    return E_FAIL;

  DROPFILES* pDropFile = (DROPFILES*)GlobalLock(hGlobalMemory);

  
  pDropFile->pFiles = sizeof(DROPFILES); 
  pDropFile->fNC    = 0;
  pDropFile->pt.x   = 0;
  pDropFile->pt.y   = 0;
  pDropFile->fWide  = TRUE;

  
  dest = (char16_t*)(((char*)pDropFile) + pDropFile->pFiles);
  memcpy(dest, path.get(), (allocLen - 1) * sizeof(char16_t));

  
  
  
  dest[allocLen - 1] = L'\0';

  GlobalUnlock(hGlobalMemory);

  aSTG.hGlobal = hGlobalMemory;

  return S_OK;
}

HRESULT nsDataObj::DropImage(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  nsresult rv;
  if (!mCachedTempFile) {
    uint32_t len = 0;
    nsCOMPtr<nsISupports> genericDataWrapper;

    mTransferable->GetTransferData(kNativeImageMime, getter_AddRefs(genericDataWrapper), &len);
    nsCOMPtr<imgIContainer> image(do_QueryInterface(genericDataWrapper));

    if (!image) {
      
      
      
      nsCOMPtr<nsISupportsInterfacePointer> ptr(do_QueryInterface(genericDataWrapper));
      if (ptr) {
        nsCOMPtr<nsISupports> supports;
        ptr->GetData(getter_AddRefs(supports));
        image = do_QueryInterface(supports);
      }
    }

    if (!image) 
      return E_FAIL;

    
    nsImageToClipboard converter(image);
    HANDLE bits = nullptr;
    rv = converter.GetPicture(&bits); 

    if (NS_FAILED(rv) || !bits)
      return E_FAIL;

    
    uint32_t bitmapSize = GlobalSize(bits);
    if (!bitmapSize) {
      GlobalFree(bits);
      return E_FAIL;
    }

    
    nsCOMPtr<nsIFile> dropFile;
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dropFile));
    if (!dropFile) {
      GlobalFree(bits);
      return E_FAIL;
    }

    
    
    char buf[13];
    nsCString filename;
    NS_MakeRandomString(buf, 8);
    memcpy(buf+8, ".bmp", 5);
    filename.Append(nsDependentCString(buf, 12));
    dropFile->AppendNative(filename);
    rv = dropFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0660);
    if (NS_FAILED(rv)) { 
      GlobalFree(bits);
      return E_FAIL;
    }

    
    
    
    dropFile->Clone(getter_AddRefs(mCachedTempFile));

    
    nsCOMPtr<nsIOutputStream> outStream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream), dropFile);
    if (NS_FAILED(rv)) { 
      GlobalFree(bits);
      return E_FAIL;
    }

    char * bm = (char *)GlobalLock(bits);

    BITMAPFILEHEADER	fileHdr;
    BITMAPINFOHEADER *bmpHdr = (BITMAPINFOHEADER*)bm;

    fileHdr.bfType        = ((WORD) ('M' << 8) | 'B');
    fileHdr.bfSize        = GlobalSize (bits) + sizeof(fileHdr);
    fileHdr.bfReserved1   = 0;
    fileHdr.bfReserved2   = 0;
    fileHdr.bfOffBits     = (DWORD) (sizeof(fileHdr) + bmpHdr->biSize);

    uint32_t writeCount = 0;
    if (NS_FAILED(outStream->Write((const char *)&fileHdr, sizeof(fileHdr), &writeCount)) ||
        NS_FAILED(outStream->Write((const char *)bm, bitmapSize, &writeCount)))
      rv = NS_ERROR_FAILURE;

    outStream->Close();

    GlobalUnlock(bits);
    GlobalFree(bits);

    if (NS_FAILED(rv))
      return E_FAIL;
  }
  
  
  nsAutoString path;
  rv = mCachedTempFile->GetPath(path);
  if (NS_FAILED(rv))
    return E_FAIL;

  
  HGLOBAL hGlobalMemory = nullptr;

  uint32_t allocLen = path.Length() + 2;

  aSTG.tymed = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = nullptr;

  hGlobalMemory = GlobalAlloc(GMEM_MOVEABLE, sizeof(DROPFILES) + allocLen * sizeof(char16_t));
  if (!hGlobalMemory)
    return E_FAIL;

  DROPFILES* pDropFile = (DROPFILES*)GlobalLock(hGlobalMemory);

  
  pDropFile->pFiles = sizeof(DROPFILES); 
  pDropFile->fNC    = 0;
  pDropFile->pt.x   = 0;
  pDropFile->pt.y   = 0;
  pDropFile->fWide  = TRUE;

  
  char16_t* dest = (char16_t*)(((char*)pDropFile) + pDropFile->pFiles);
  memcpy(dest, path.get(), (allocLen - 1) * sizeof(char16_t)); 

  
  
  
  dest[allocLen - 1] = L'\0';

  GlobalUnlock(hGlobalMemory);

  aSTG.hGlobal = hGlobalMemory;

  return S_OK;
}

HRESULT nsDataObj::DropTempFile(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  nsresult rv;
  if (!mCachedTempFile) {
    
    nsCOMPtr<nsIFile> dropFile;
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dropFile));
    if (!dropFile)
      return E_FAIL;

    
    nsCString filename;
    nsAutoString wideFileName;
    nsCOMPtr<nsIURI> sourceURI;
    HRESULT res;
    res = GetDownloadDetails(getter_AddRefs(sourceURI),
      wideFileName);
    if (FAILED(res))
      return res;
    NS_UTF16ToCString(wideFileName, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, filename);

    dropFile->AppendNative(filename);
    rv = dropFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0660);
    if (NS_FAILED(rv))
      return E_FAIL;

    
    
    
    dropFile->Clone(getter_AddRefs(mCachedTempFile));

    
    nsCOMPtr<nsIOutputStream> outStream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream), dropFile);
    if (NS_FAILED(rv))
      return E_FAIL;

    IStream *pStream = nullptr;
    nsDataObj::CreateStream(&pStream);
    NS_ENSURE_TRUE(pStream, E_FAIL);

    char buffer[512];
    ULONG readCount = 0;
    uint32_t writeCount = 0;
    while (1) {
      HRESULT hres = pStream->Read(buffer, sizeof(buffer), &readCount);
      if (FAILED(hres))
        return E_FAIL;
      if (readCount == 0)
        break;
      rv = outStream->Write(buffer, readCount, &writeCount);
      if (NS_FAILED(rv))
        return E_FAIL;
    }
    outStream->Close();
    pStream->Release();
  }

  
  nsAutoString path;
  rv = mCachedTempFile->GetPath(path);
  if (NS_FAILED(rv))
    return E_FAIL;

  uint32_t allocLen = path.Length() + 2;

  
  HGLOBAL hGlobalMemory = nullptr;

  aSTG.tymed = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = nullptr;

  hGlobalMemory = GlobalAlloc(GMEM_MOVEABLE, sizeof(DROPFILES) + allocLen * sizeof(char16_t));
  if (!hGlobalMemory)
    return E_FAIL;

  DROPFILES* pDropFile = (DROPFILES*)GlobalLock(hGlobalMemory);

  
  pDropFile->pFiles = sizeof(DROPFILES); 
  pDropFile->fNC    = 0;
  pDropFile->pt.x   = 0;
  pDropFile->pt.y   = 0;
  pDropFile->fWide  = TRUE;

  
  char16_t* dest = (char16_t*)(((char*)pDropFile) + pDropFile->pFiles);
  memcpy(dest, path.get(), (allocLen - 1) * sizeof(char16_t)); 

  
  
  
  dest[allocLen - 1] = L'\0';

  GlobalUnlock(hGlobalMemory);

  aSTG.hGlobal = hGlobalMemory;

  return S_OK;
}


HRESULT nsDataObj::GetMetafilePict(FORMATETC&, STGMEDIUM&)
{
	return E_NOTIMPL;
}


HRESULT nsDataObj::SetBitmap(FORMATETC&, STGMEDIUM&)
{
	return E_NOTIMPL;
}


HRESULT nsDataObj::SetDib(FORMATETC&, STGMEDIUM&)
{
	return E_FAIL;
}


HRESULT nsDataObj::SetText  (FORMATETC& aFE, STGMEDIUM& aSTG)
{
	return E_FAIL;
}


HRESULT nsDataObj::SetMetafilePict(FORMATETC&, STGMEDIUM&)
{
	return E_FAIL;
}





CLSID nsDataObj::GetClassID() const
{
	return CLSID_nsDataObj;
}




void nsDataObj::AddDataFlavor(const char* aDataFlavor, LPFORMATETC aFE)
{
  
  
  
  
  mDataFlavors.AppendElement(aDataFlavor);
  m_enumFE->AddFormatEtc(aFE);
}




void nsDataObj::SetTransferable(nsITransferable * aTransferable)
{
    NS_IF_RELEASE(mTransferable);

  mTransferable = aTransferable;
  if (nullptr == mTransferable) {
    return;
  }

  NS_ADDREF(mTransferable);

  return;
}











nsresult
nsDataObj :: ExtractShortcutURL ( nsString & outURL )
{
  NS_ASSERTION ( mTransferable, "We don't have a good transferable" );
  nsresult rv = NS_ERROR_FAILURE;
  
  uint32_t len = 0;
  nsCOMPtr<nsISupports> genericURL;
  if ( NS_SUCCEEDED(mTransferable->GetTransferData(kURLMime, getter_AddRefs(genericURL), &len)) ) {
    nsCOMPtr<nsISupportsString> urlObject ( do_QueryInterface(genericURL) );
    if ( urlObject ) {
      nsAutoString url;
      urlObject->GetData ( url );
      outURL = url;

      
      
      int32_t lineIndex = outURL.FindChar ( '\n' );
      NS_ASSERTION ( lineIndex > 0, "Format for url flavor is <url> <linefeed> <page title>" );
      if ( lineIndex > 0 ) {
        outURL.Truncate ( lineIndex );
        rv = NS_OK;    
      }
    }
  } else if ( NS_SUCCEEDED(mTransferable->GetTransferData(kURLDataMime, getter_AddRefs(genericURL), &len)) ||
              NS_SUCCEEDED(mTransferable->GetTransferData(kURLPrivateMime, getter_AddRefs(genericURL), &len)) ) {
    nsCOMPtr<nsISupportsString> urlObject ( do_QueryInterface(genericURL) );
    if ( urlObject ) {
      nsAutoString url;
      urlObject->GetData ( url );
      outURL = url;

      rv = NS_OK;    
    }

  }  
  
  return rv;

} 











nsresult
nsDataObj :: ExtractShortcutTitle ( nsString & outTitle )
{
  NS_ASSERTION ( mTransferable, "We'd don't have a good transferable" );
  nsresult rv = NS_ERROR_FAILURE;
  
  uint32_t len = 0;
  nsCOMPtr<nsISupports> genericURL;
  if ( NS_SUCCEEDED(mTransferable->GetTransferData(kURLMime, getter_AddRefs(genericURL), &len)) ) {
    nsCOMPtr<nsISupportsString> urlObject ( do_QueryInterface(genericURL) );
    if ( urlObject ) {
      nsAutoString url;
      urlObject->GetData ( url );

      
      
      int32_t lineIndex = url.FindChar ( '\n' );
      NS_ASSERTION ( lineIndex != -1, "Format for url flavor is <url> <linefeed> <page title>" );
      if ( lineIndex != -1 ) {
        url.Mid ( outTitle, lineIndex + 1, (len/2) - (lineIndex + 1) );
        rv = NS_OK;    
      }
    }
  } 
  
  return rv;

} 














nsresult 
nsDataObj :: BuildPlatformHTML ( const char* inOurHTML, char** outPlatformHTML ) 
{
  *outPlatformHTML = nullptr;

  nsDependentCString inHTMLString(inOurHTML);
  const char* const numPlaceholder  = "00000000";
  const char* const startHTMLPrefix = "Version:0.9\r\nStartHTML:";
  const char* const endHTMLPrefix   = "\r\nEndHTML:";
  const char* const startFragPrefix = "\r\nStartFragment:";
  const char* const endFragPrefix   = "\r\nEndFragment:";
  const char* const startSourceURLPrefix = "\r\nSourceURL:";
  const char* const endFragTrailer  = "\r\n";

  
  if (mSourceURL.IsEmpty()) {
    nsAutoString url;
    ExtractShortcutURL(url);

    AppendUTF16toUTF8(url, mSourceURL);
  }

  const int32_t kSourceURLLength    = mSourceURL.Length();
  const int32_t kNumberLength       = strlen(numPlaceholder);

  const int32_t kTotalHeaderLen     = strlen(startHTMLPrefix) +
                                      strlen(endHTMLPrefix) +
                                      strlen(startFragPrefix) + 
                                      strlen(endFragPrefix) + 
                                      strlen(endFragTrailer) +
                                      (kSourceURLLength > 0 ? strlen(startSourceURLPrefix) : 0) +
                                      kSourceURLLength +
                                      (4 * kNumberLength);

  NS_NAMED_LITERAL_CSTRING(htmlHeaderString, "<html><body>\r\n");

  NS_NAMED_LITERAL_CSTRING(fragmentHeaderString, "<!--StartFragment-->");

  nsDependentCString trailingString(
      "<!--EndFragment-->\r\n"
      "</body>\r\n"
      "</html>");

  
  int32_t startHTMLOffset = kTotalHeaderLen;
  int32_t startFragOffset = startHTMLOffset
                              + htmlHeaderString.Length()
			      + fragmentHeaderString.Length();

  int32_t endFragOffset   = startFragOffset
                              + inHTMLString.Length();

  int32_t endHTMLOffset   = endFragOffset
                              + trailingString.Length();

  
  nsCString clipboardString;
  clipboardString.SetCapacity(endHTMLOffset);

  clipboardString.Append(startHTMLPrefix);
  clipboardString.Append(nsPrintfCString("%08u", startHTMLOffset));

  clipboardString.Append(endHTMLPrefix);  
  clipboardString.Append(nsPrintfCString("%08u", endHTMLOffset));

  clipboardString.Append(startFragPrefix);
  clipboardString.Append(nsPrintfCString("%08u", startFragOffset));

  clipboardString.Append(endFragPrefix);
  clipboardString.Append(nsPrintfCString("%08u", endFragOffset));

  if (kSourceURLLength > 0) {
    clipboardString.Append(startSourceURLPrefix);
    clipboardString.Append(mSourceURL);
  }

  clipboardString.Append(endFragTrailer);

  clipboardString.Append(htmlHeaderString);
  clipboardString.Append(fragmentHeaderString);
  clipboardString.Append(inHTMLString);
  clipboardString.Append(trailingString);

  *outPlatformHTML = ToNewCString(clipboardString);
  if (!*outPlatformHTML)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

HRESULT 
nsDataObj :: GetUniformResourceLocator( FORMATETC& aFE, STGMEDIUM& aSTG, bool aIsUnicode )
{
  HRESULT res = S_OK;
  if (IsFlavourPresent(kURLMime)) {
    if ( aIsUnicode )
      res = ExtractUniformResourceLocatorW( aFE, aSTG );
    else
      res = ExtractUniformResourceLocatorA( aFE, aSTG );
  }
  else
    NS_WARNING ("Not yet implemented\n");
  return res;
}

HRESULT
nsDataObj::ExtractUniformResourceLocatorA(FORMATETC& aFE, STGMEDIUM& aSTG )
{
  HRESULT result = S_OK;

  nsAutoString url;
  if (NS_FAILED(ExtractShortcutURL(url)))
    return E_OUTOFMEMORY;

  NS_LossyConvertUTF16toASCII asciiUrl(url);
  const int totalLen = asciiUrl.Length() + 1;
  HGLOBAL hGlobalMemory = GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE, totalLen);
  if (!hGlobalMemory)
    return E_OUTOFMEMORY;

  char* contents = reinterpret_cast<char*>(GlobalLock(hGlobalMemory));
  if (!contents) {
    GlobalFree(hGlobalMemory);
    return E_OUTOFMEMORY;
  }

  strcpy(contents, asciiUrl.get());
  GlobalUnlock(hGlobalMemory);
  aSTG.hGlobal = hGlobalMemory;
  aSTG.tymed = TYMED_HGLOBAL;

  return result;
}

HRESULT
nsDataObj::ExtractUniformResourceLocatorW(FORMATETC& aFE, STGMEDIUM& aSTG )
{
  HRESULT result = S_OK;

  nsAutoString url;
  if (NS_FAILED(ExtractShortcutURL(url)))
    return E_OUTOFMEMORY;

  const int totalLen = (url.Length() + 1) * sizeof(char16_t);
  HGLOBAL hGlobalMemory = GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE, totalLen);
  if (!hGlobalMemory)
    return E_OUTOFMEMORY;

  wchar_t* contents = reinterpret_cast<wchar_t*>(GlobalLock(hGlobalMemory));
  if (!contents) {
    GlobalFree(hGlobalMemory);
    return E_OUTOFMEMORY;
  }

  wcscpy(contents, url.get());
  GlobalUnlock(hGlobalMemory);
  aSTG.hGlobal = hGlobalMemory;
  aSTG.tymed = TYMED_HGLOBAL;

  return result;
}



HRESULT nsDataObj::GetDownloadDetails(nsIURI **aSourceURI,
                                      nsAString &aFilename)
{
  *aSourceURI = nullptr;

  NS_ENSURE_TRUE(mTransferable, E_FAIL);

  
  nsCOMPtr<nsISupports> urlPrimitive;
  uint32_t dataSize = 0;
  mTransferable->GetTransferData(kFilePromiseURLMime, getter_AddRefs(urlPrimitive), &dataSize);
  nsCOMPtr<nsISupportsString> srcUrlPrimitive = do_QueryInterface(urlPrimitive);
  NS_ENSURE_TRUE(srcUrlPrimitive, E_FAIL);
  
  nsAutoString srcUri;
  srcUrlPrimitive->GetData(srcUri);
  if (srcUri.IsEmpty())
    return E_FAIL;
  nsCOMPtr<nsIURI> sourceURI;
  NS_NewURI(getter_AddRefs(sourceURI), srcUri);

  nsAutoString srcFileName;
  nsCOMPtr<nsISupports> fileNamePrimitive;
  mTransferable->GetTransferData(kFilePromiseDestFilename, getter_AddRefs(fileNamePrimitive), &dataSize);
  nsCOMPtr<nsISupportsString> srcFileNamePrimitive = do_QueryInterface(fileNamePrimitive);
  if (srcFileNamePrimitive) {
    srcFileNamePrimitive->GetData(srcFileName);
  } else {
    nsCOMPtr<nsIURL> sourceURL = do_QueryInterface(sourceURI);
    if (!sourceURL)
      return E_FAIL;
    
    nsAutoCString urlFileName;
    sourceURL->GetFileName(urlFileName);
    NS_UnescapeURL(urlFileName);
    CopyUTF8toUTF16(urlFileName, srcFileName);
  }
  if (srcFileName.IsEmpty())
    return E_FAIL;

  
  MangleTextToValidFilename(srcFileName);

  sourceURI.swap(*aSourceURI);
  aFilename = srcFileName;
  return S_OK;
}

HRESULT nsDataObj::GetFileDescriptor_IStreamA(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  NS_ENSURE_TRUE(fileGroupDescHandle, E_OUTOFMEMORY);

  LPFILEGROUPDESCRIPTORA fileGroupDescA = reinterpret_cast<LPFILEGROUPDESCRIPTORA>(GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescA) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  nsAutoString wideFileName;
  HRESULT res;
  nsCOMPtr<nsIURI> sourceURI;
  res = GetDownloadDetails(getter_AddRefs(sourceURI), wideFileName);
  if (FAILED(res))
  {
    ::GlobalFree(fileGroupDescHandle);
    return res;
  }

  nsAutoCString nativeFileName;
  NS_UTF16ToCString(wideFileName, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, nativeFileName);
  
  strncpy(fileGroupDescA->fgd[0].cFileName, nativeFileName.get(), NS_MAX_FILEDESCRIPTOR - 1);
  fileGroupDescA->fgd[0].cFileName[NS_MAX_FILEDESCRIPTOR - 1] = '\0';

  
  fileGroupDescA->cItems = 1;
  fileGroupDescA->fgd[0].dwFlags = FD_PROGRESSUI;

  GlobalUnlock( fileGroupDescHandle );
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
}

HRESULT nsDataObj::GetFileDescriptor_IStreamW(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  NS_ENSURE_TRUE(fileGroupDescHandle, E_OUTOFMEMORY);

  LPFILEGROUPDESCRIPTORW fileGroupDescW = reinterpret_cast<LPFILEGROUPDESCRIPTORW>(GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescW) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  nsAutoString wideFileName;
  HRESULT res;
  nsCOMPtr<nsIURI> sourceURI;
  res = GetDownloadDetails(getter_AddRefs(sourceURI),
                           wideFileName);
  if (FAILED(res))
  {
    ::GlobalFree(fileGroupDescHandle);
    return res;
  }

  wcsncpy(fileGroupDescW->fgd[0].cFileName, wideFileName.get(), NS_MAX_FILEDESCRIPTOR - 1);
  fileGroupDescW->fgd[0].cFileName[NS_MAX_FILEDESCRIPTOR - 1] = '\0';
  
  fileGroupDescW->cItems = 1;
  fileGroupDescW->fgd[0].dwFlags = FD_PROGRESSUI;

  GlobalUnlock(fileGroupDescHandle);
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
}

HRESULT nsDataObj::GetFileContents_IStream(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  IStream *pStream = nullptr;

  nsDataObj::CreateStream(&pStream);
  NS_ENSURE_TRUE(pStream, E_FAIL);

  aSTG.tymed = TYMED_ISTREAM;
  aSTG.pstm = pStream;
  aSTG.pUnkForRelease = nullptr;

  return S_OK;
}

void nsDataObj::RemoveTempFile(nsITimer* aTimer, void* aClosure)
{
  nsDataObj *timedDataObj = static_cast<nsDataObj *>(aClosure);
  if (timedDataObj->mCachedTempFile) {
    timedDataObj->mCachedTempFile->Remove(false);
    timedDataObj->mCachedTempFile = nullptr;
  }
  timedDataObj->Release();
}
