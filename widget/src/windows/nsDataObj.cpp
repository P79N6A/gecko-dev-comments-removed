









































#include <ole2.h>
#include <shlobj.h>

#include "nsDataObj.h"
#include "nsClipboard.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsVoidArray.h"
#include "nsITransferable.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "IENUMFE.H"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPIDLString.h"
#include "nsIImage.h"
#include "nsImageClipboard.h"
#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsDirectoryServiceDefs.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsPrintfCString.h"
#include "nsIStringBundle.h"
#include "nsEscape.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsXPCOMStrings.h"




#ifndef __IAsyncOperation_INTERFACE_DEFINED__
  const IID IID_IAsyncOperation = {0x3D8B0590, 0xF691, 0x11d2, {0x8E, 0xA9, 0x00, 0x60, 0x97, 0xDF, 0x5B, 0xD4}};
#endif

#if 0
#define PRNTDEBUG(_x) printf(_x);
#define PRNTDEBUG2(_x1, _x2) printf(_x1, _x2);
#define PRNTDEBUG3(_x1, _x2, _x3) printf(_x1, _x2, _x3);
#else
#define PRNTDEBUG(_x)
#define PRNTDEBUG2(_x1, _x2)
#define PRNTDEBUG3(_x1, _x2, _x3)
#endif



nsDataObj::CStream::CStream() : mRefCount(1)
{
}


nsDataObj::CStream::~CStream()
{
}



nsresult nsDataObj::CStream::Init(nsIURI *pSourceURI)
{
  nsresult rv;
  rv = NS_NewChannel(getter_AddRefs(mChannel), pSourceURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mChannel->Open(getter_AddRefs(mInputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}



STDMETHODIMP nsDataObj::CStream::QueryInterface(REFIID refiid, void** ppvResult)
{
  *ppvResult = NULL;
  if (IID_IUnknown == refiid ||
      refiid == IID_IStream)

  {
    *ppvResult = this;
  }

  if (NULL != *ppvResult)
  {
    ((LPUNKNOWN)*ppvResult)->AddRef();
    return S_OK;
  }

  return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) nsDataObj::CStream::AddRef(void)
{
  return ++mRefCount;
}


STDMETHODIMP_(ULONG) nsDataObj::CStream::Release(void)
{
  ULONG nCount = --mRefCount;
  if (nCount == 0)
  {
    delete this;
    return (ULONG)0;
  }

  return mRefCount;
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
  NS_ENSURE_TRUE(mInputStream, E_FAIL);

  nsresult rv;
  PRUint32 read = 0;
  rv = mInputStream->Read((char*)pvBuffer, nBytesToRead, &read);
  *nBytesRead = read;
  NS_ENSURE_SUCCESS(rv, S_FALSE);

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
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::SetSize(ULARGE_INTEGER nNewSize)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsDataObj::CStream::Stat(STATSTG* statstg, DWORD dwFlags)
{
  return E_NOTIMPL;
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

  rv = GetDownloadDetails(getter_AddRefs(sourceURI),
                          wideFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsDataObj::CStream *pStream = new nsDataObj::CStream();
  NS_ENSURE_TRUE(pStream, E_OUTOFMEMORY);

  rv = pStream->Init(sourceURI);
  if (NS_FAILED(rv))
  {
    pStream->Release();
    return E_FAIL;
  }
  *outStream = pStream;

  return S_OK;
}

ULONG nsDataObj::g_cRef = 0;

EXTERN_C GUID CDECL CLSID_nsDataObj =
	{ 0x1bba7640, 0xdf52, 0x11cf, { 0x82, 0x7b, 0, 0xa0, 0x24, 0x3a, 0xe5, 0x05 } };







#define NS_MAX_FILEDESCRIPTOR 128 + 1








nsDataObj::nsDataObj(nsIURI * uri)
: m_cRef(0), mTransferable(nsnull)
{
  mDataFlavors    = new nsVoidArray();
  m_enumFE        = new CEnumFormatEtc(32);
 
  m_enumFE->AddRef();

  if (uri) {

    
    
    uri->GetSpec(mSourceURL);
  }

  mIsAsyncMode = FALSE;
  mIsInOperation = FALSE;
}



nsDataObj::~nsDataObj()
{
  NS_IF_RELEASE(mTransferable);
  PRInt32 i;
  for (i=0;i<mDataFlavors->Count();i++) {
    nsCAutoString* df = NS_REINTERPRET_CAST(nsCAutoString *, mDataFlavors->ElementAt(i));
    delete df;
  }

  delete mDataFlavors;

  m_cRef = 0;
  m_enumFE->Release();

}





STDMETHODIMP nsDataObj::QueryInterface(REFIID riid, void** ppv)
{
	*ppv=NULL;

	if ( (IID_IUnknown == riid) || (IID_IDataObject	== riid) ) {
		*ppv = this;
		AddRef();
		return S_OK;
  } else if (IID_IAsyncOperation == riid) {
    *ppv = static_cast<IAsyncOperation*>(this);
    AddRef();
    return S_OK;
  }

	return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) nsDataObj::AddRef()
{
	++g_cRef;
	++m_cRef;
	NS_LOG_ADDREF(this, m_cRef, "nsDataObj", sizeof(*this));
  
	return m_cRef;
}



STDMETHODIMP_(ULONG) nsDataObj::Release()
{
  
	if (0 < g_cRef)
		--g_cRef;

	NS_LOG_RELEASE(this, m_cRef, "nsDataObj");
	if (0 != --m_cRef)
		return m_cRef;

	delete this;

	return 0;
}


BOOL nsDataObj::FormatsMatch(const FORMATETC& source, const FORMATETC& target) const
{
	if ((source.cfFormat == target.cfFormat) &&
		 (source.dwAspect  & target.dwAspect)  &&
		 (source.tymed     & target.tymed))       {
		return TRUE;
	} else {
		return FALSE;
	}
}




STDMETHODIMP nsDataObj::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  PRNTDEBUG("nsDataObj::GetData\n");
  PRNTDEBUG3("  format: %d  Text: %d\n", pFE->cfFormat, CF_TEXT);
  if ( !mTransferable )
	  return ResultFromScode(DATA_E_FORMATETC);

  PRUint32 dfInx = 0;

  static CLIPFORMAT fileDescriptorFlavorA = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORA ); 
  static CLIPFORMAT fileDescriptorFlavorW = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORW ); 
  static CLIPFORMAT uniformResourceLocatorA = ::RegisterClipboardFormat( CFSTR_INETURLA );
  static CLIPFORMAT uniformResourceLocatorW = ::RegisterClipboardFormat( CFSTR_INETURLW );
#ifndef WINCE
  static CLIPFORMAT fileFlavor = ::RegisterClipboardFormat( CFSTR_FILECONTENTS ); 
  static CLIPFORMAT PreferredDropEffect = ::RegisterClipboardFormat( CFSTR_PREFERREDDROPEFFECT );
#endif

  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)) {
    nsCAutoString * df = NS_REINTERPRET_CAST(nsCAutoString*, mDataFlavors->SafeElementAt(dfInx));
    if ( df ) {
      if (FormatsMatch(fe, *pFE)) {
        pSTM->pUnkForRelease = NULL;        
        CLIPFORMAT format = pFE->cfFormat;
        switch(format) {

        
        case CF_TEXT:
        case CF_UNICODETEXT:
        return GetText(*df, *pFE, *pSTM);

        
        case CF_DIB:
          return GetDib(*df, *pFE, *pSTM);
                                              
        
        
        
        
        
            
        default:
          if ( format == fileDescriptorFlavorA )
            return GetFileDescriptor ( *pFE, *pSTM, PR_FALSE );
          if ( format == fileDescriptorFlavorW )
            return GetFileDescriptor ( *pFE, *pSTM, PR_TRUE);
          if ( format == uniformResourceLocatorA )
            return GetUniformResourceLocator( *pFE, *pSTM, PR_FALSE);
          if ( format == uniformResourceLocatorW )
            return GetUniformResourceLocator( *pFE, *pSTM, PR_TRUE);
#ifndef WINCE
          if ( format == fileFlavor )
            return GetFileContents ( *pFE, *pSTM );
          if ( format == PreferredDropEffect )
            return GetPreferredDropEffect( *pFE, *pSTM );
#endif
          PRNTDEBUG2("***** nsDataObj::GetData - Unknown format %u\n", format);
          return GetText(*df, *pFE, *pSTM);
        } 
      } 
    }
    dfInx++;
  } 

  return ResultFromScode(DATA_E_FORMATETC);
}



STDMETHODIMP nsDataObj::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  PRNTDEBUG("nsDataObj::GetDataHere\n");
		return ResultFromScode(E_FAIL);
}






STDMETHODIMP nsDataObj::QueryGetData(LPFORMATETC pFE)
{
  PRNTDEBUG("nsDataObj::QueryGetData  ");
  PRNTDEBUG3("format: %d  Text: %d\n", pFE->cfFormat, CF_TEXT);

  ULONG count;
  FORMATETC fe;
  m_enumFE->Reset();
  while (NOERROR == m_enumFE->Next(1, &fe, &count)) {
    if (fe.cfFormat == pFE->cfFormat) {
      return S_OK;
    }
  }
  
  PRNTDEBUG2("***** nsDataObj::QueryGetData - Unknown format %d\n", pFE->cfFormat);
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObj::GetCanonicalFormatEtc
	 (LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
  PRNTDEBUG("nsDataObj::GetCanonicalFormatEtc\n");
		return ResultFromScode(E_FAIL);
}



STDMETHODIMP nsDataObj::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
  PRNTDEBUG("nsDataObj::SetData\n");
#ifndef WINCE
  static CLIPFORMAT PerformedDropEffect = ::RegisterClipboardFormat( CFSTR_PERFORMEDDROPEFFECT );  

  if (pFE && pFE->cfFormat == PerformedDropEffect) {
    
    if (mCachedTempFile) {
      mCachedTempFile->Remove(PR_FALSE);
      mCachedTempFile = NULL;
    }
  }
#endif

  if (fRelease) {
    ReleaseStgMedium(pSTM);
  }

  return ResultFromScode(S_OK);
}



STDMETHODIMP nsDataObj::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
  PRNTDEBUG("nsDataObj::EnumFormatEtc\n");

  switch (dwDir) {
    case DATADIR_GET: {
       m_enumFE->Clone(ppEnum);
    } break;
    case DATADIR_SET:
        *ppEnum=NULL;
        break;
    default:
        *ppEnum=NULL;
        break;
  } 

  
  
  
  if (NULL == *ppEnum)
    return ResultFromScode(E_FAIL);
  else
    (*ppEnum)->AddRef();

  return NOERROR;

}


STDMETHODIMP nsDataObj::DAdvise(LPFORMATETC pFE, DWORD dwFlags,
										            LPADVISESINK pIAdviseSink, DWORD* pdwConn)
{
  PRNTDEBUG("nsDataObj::DAdvise\n");
	return ResultFromScode(E_FAIL);
}



STDMETHODIMP nsDataObj::DUnadvise(DWORD dwConn)
{
  PRNTDEBUG("nsDataObj::DUnadvise\n");
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObj::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
  PRNTDEBUG("nsDataObj::EnumDAdvise\n");
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObj::EndOperation(HRESULT hResult,
                                     IBindCtx *pbcReserved,
                                     DWORD dwEffects)
{
  mIsInOperation = FALSE;
  Release();
  return S_OK;
}

STDMETHODIMP nsDataObj::GetAsyncMode(BOOL *pfIsOpAsync)
{
  if (!pfIsOpAsync)
    return E_FAIL;

  *pfIsOpAsync = mIsAsyncMode;

  return S_OK;
}

STDMETHODIMP nsDataObj::InOperation(BOOL *pfInAsyncOp)
{
  if (!pfInAsyncOp)
    return E_FAIL;

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




ULONG nsDataObj::GetCumRefCount()
{
	return g_cRef;
}


ULONG nsDataObj::GetRefCount() const
{
	return m_cRef;
}




HRESULT nsDataObj::AddSetFormat(FORMATETC& aFE)
{
  PRNTDEBUG("nsDataObj::AddSetFormat\n");
	return ResultFromScode(S_OK);
}


HRESULT nsDataObj::AddGetFormat(FORMATETC& aFE)
{
  PRNTDEBUG("nsDataObj::AddGetFormat\n");
	return ResultFromScode(S_OK);
}


HRESULT 
nsDataObj::GetBitmap ( const nsACString& , FORMATETC&, STGMEDIUM& )
{
  PRNTDEBUG("nsDataObj::GetBitmap\n");
	return ResultFromScode(E_NOTIMPL);
}








HRESULT 
nsDataObj :: GetDib ( const nsACString& inFlavor, FORMATETC &, STGMEDIUM & aSTG )
{
  PRNTDEBUG("nsDataObj::GetDib\n");
  ULONG result = E_FAIL;
#ifndef WINCE  
  
  PRUint32 len = 0;
  nsCOMPtr<nsISupports> genericDataWrapper;
  mTransferable->GetTransferData(PromiseFlatCString(inFlavor).get(), getter_AddRefs(genericDataWrapper), &len);
  nsCOMPtr<nsIImage> image ( do_QueryInterface(genericDataWrapper) );
  if ( !image ) {
    
    
    
    
    nsCOMPtr<nsISupportsInterfacePointer> ptr(do_QueryInterface(genericDataWrapper));
    if ( ptr )
      ptr->GetData(getter_AddRefs(image));
  }
  
  if ( image ) {
    
    
    nsImageToClipboard converter ( image );
    HANDLE bits = nsnull;
    nsresult rv = converter.GetPicture ( &bits );
    if ( NS_SUCCEEDED(rv) && bits ) {
      aSTG.hGlobal = bits;
      aSTG.tymed = TYMED_HGLOBAL;
      result = S_OK;
    }
  } 
  else  
    NS_WARNING ( "Definitely not an image on clipboard" );

#endif
	return ResultFromScode(result);
}







HRESULT 
nsDataObj :: GetFileDescriptor ( FORMATETC& aFE, STGMEDIUM& aSTG, PRBool aIsUnicode )
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
  aText.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRUint32 nameLen;
  for (size_t n = 0; n < NS_ARRAY_LENGTH(forbiddenNames); ++n) {
    nameLen = (PRUint32) strlen(forbiddenNames[n]);
    if (aText.EqualsIgnoreCase(forbiddenNames[n], nameLen)) {
      
      if (aText.Length() == nameLen || aText.CharAt(nameLen) == PRUnichar('.')) {
        aText.Truncate();
        break;
      }
    }
  }
}








static PRBool
CreateFilenameFromTextA(nsString & aText, const char * aExtension, 
                         char * aFilename, PRUint32 aFilenameLen)
{
  
  
  
  MangleTextToValidFilename(aText);
  if (aText.IsEmpty())
    return PR_FALSE;

  
  
  
  
  
  int maxUsableFilenameLen = aFilenameLen - strlen(aExtension) - 1; 
  int currLen, textLen = (int) NS_MIN(aText.Length(), aFilenameLen);
  char defaultChar = '_';
  do {
    currLen = WideCharToMultiByte(CP_ACP, 
      WC_COMPOSITECHECK|WC_DEFAULTCHAR,
      aText.get(), textLen--, aFilename, maxUsableFilenameLen, &defaultChar, NULL);
  }
  while (currLen == 0 && textLen > 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  if (currLen > 0 && textLen > 0) {
    strcpy(&aFilename[currLen], aExtension);
    return PR_TRUE;
  }
  else {
    
    return PR_FALSE;
  }
}

static PRBool
CreateFilenameFromTextW(nsString & aText, const wchar_t * aExtension, 
                         wchar_t * aFilename, PRUint32 aFilenameLen)
{
  
  
  
  MangleTextToValidFilename(aText);
  if (aText.IsEmpty())
    return PR_FALSE;

  const int extensionLen = wcslen(aExtension);
  if (aText.Length() + extensionLen + 1 > aFilenameLen)
    aText.Truncate(aFilenameLen - extensionLen - 1);
  wcscpy(&aFilename[0], aText.get());
  wcscpy(&aFilename[aText.Length()], aExtension);
  return PR_TRUE;
}

#define PAGEINFO_PROPERTIES "chrome://navigator/locale/pageInfo.properties"

static PRBool
GetLocalizedString(const PRUnichar * aName, nsXPIDLString & aString)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) 
    return PR_FALSE;

  nsCOMPtr<nsIStringBundle> stringBundle;
  rv = stringService->CreateBundle(PAGEINFO_PROPERTIES, getter_AddRefs(stringBundle));
  if (NS_FAILED(rv))
    return PR_FALSE;

  rv = stringBundle->GetStringFromName(aName, getter_Copies(aString));
  return NS_SUCCEEDED(rv);
}







HRESULT
nsDataObj :: GetFileDescriptorInternetShortcutA ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
#ifdef WINCE
  return E_FAIL;
#else
  
  nsAutoString title;
  if ( NS_FAILED(ExtractShortcutTitle(title)) )
    return E_OUTOFMEMORY;

  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORA));
  if (!fileGroupDescHandle)
    return E_OUTOFMEMORY;

  LPFILEGROUPDESCRIPTORA fileGroupDescA = NS_REINTERPRET_CAST(LPFILEGROUPDESCRIPTORA, 
      ::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescA) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  
  
  if (!CreateFilenameFromTextA(title, ".URL", 
                               fileGroupDescA->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
    nsXPIDLString untitled;
    if (!GetLocalizedString(NS_LITERAL_STRING("noPageTitle").get(), untitled) ||
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
#endif
} 

HRESULT
nsDataObj :: GetFileDescriptorInternetShortcutW ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
#ifdef WINCE
  return E_FAIL;
#else
  
  nsAutoString title;
  if ( NS_FAILED(ExtractShortcutTitle(title)) )
    return E_OUTOFMEMORY;

  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  if (!fileGroupDescHandle)
    return E_OUTOFMEMORY;

  LPFILEGROUPDESCRIPTORW fileGroupDescW = NS_REINTERPRET_CAST(LPFILEGROUPDESCRIPTORW, 
      ::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescW) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  
  
  if (!CreateFilenameFromTextW(title, NS_LITERAL_STRING(".URL").get(), 
                               fileGroupDescW->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
    nsXPIDLString untitled;
    if (!GetLocalizedString(NS_LITERAL_STRING("noPageTitle").get(), untitled) ||
        !CreateFilenameFromTextW(untitled, NS_LITERAL_STRING(".URL").get(), 
                                 fileGroupDescW->fgd[0].cFileName, NS_MAX_FILEDESCRIPTOR)) {
      wcscpy(fileGroupDescW->fgd[0].cFileName, NS_LITERAL_STRING("Untitled.URL").get());
    }
  }

  
  fileGroupDescW->cItems = 1;
  fileGroupDescW->fgd[0].dwFlags = FD_LINKUI;

  ::GlobalUnlock( fileGroupDescHandle );
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
#endif
} 








HRESULT
nsDataObj :: GetFileContentsInternetShortcut ( FORMATETC& aFE, STGMEDIUM& aSTG )
{
#ifdef WINCE
  return E_FAIL;
#else
  nsAutoString url;
  if ( NS_FAILED(ExtractShortcutURL(url)) )
    return E_OUTOFMEMORY;

  
  nsCAutoString asciiUrl;
  LossyCopyUTF16toASCII(url, asciiUrl);
    
  static const char* shortcutFormatStr = "[InternetShortcut]\r\nURL=%s\r\n";
  static const int formatLen = strlen(shortcutFormatStr) - 2; 
  const int totalLen = formatLen + asciiUrl.Length(); 

  
  HGLOBAL hGlobalMemory = ::GlobalAlloc(GMEM_SHARE, totalLen);
  if ( !hGlobalMemory )
    return E_OUTOFMEMORY;

  char* contents = NS_REINTERPRET_CAST(char*, ::GlobalLock(hGlobalMemory));
  if ( !contents ) {
    ::GlobalFree( hGlobalMemory );
    return E_OUTOFMEMORY;
  }
    
  
  
  
  
  _snprintf( contents, totalLen, shortcutFormatStr, asciiUrl.get() );
    
  ::GlobalUnlock(hGlobalMemory);
  aSTG.hGlobal = hGlobalMemory;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
#endif  
} 


PRBool nsDataObj :: IsFlavourPresent(const char *inFlavour)
{
  PRBool retval = PR_FALSE;
  NS_ENSURE_TRUE(mTransferable, PR_FALSE);
  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  mTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
  NS_ENSURE_TRUE(flavorList, PR_FALSE);

  
  PRUint32 cnt;
  flavorList->Count(&cnt);
  for (PRUint32 i = 0; i < cnt; ++i) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt (i, getter_AddRefs(genericFlavor));
    nsCOMPtr<nsISupportsCString> currentFlavor (do_QueryInterface(genericFlavor));
    if (currentFlavor) {
      nsCAutoString flavorStr;
      currentFlavor->GetData(flavorStr);
      if (flavorStr.Equals(inFlavour)) {
        retval = PR_TRUE;         
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
  aSTG.pUnkForRelease = NULL;    
  HGLOBAL hGlobalMemory = NULL;
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
  void* data = NULL;
  PRUint32   len;
  
  
  const char* flavorStr;
  const nsPromiseFlatCString& flat = PromiseFlatCString(aDataFlavor);
  if ( aDataFlavor.Equals("text/plain") )
    flavorStr = kUnicodeMime;
  else
    flavorStr = flat.get();

  
  nsCOMPtr<nsISupports> genericDataWrapper;
  mTransferable->GetTransferData(flavorStr, getter_AddRefs(genericDataWrapper), &len);
  if ( !len )
    return ResultFromScode(E_FAIL);
  nsPrimitiveHelpers::CreateDataFromPrimitive ( flavorStr, genericDataWrapper, &data, len );
  if ( !data )
    return ResultFromScode(E_FAIL);

  HGLOBAL     hGlobalMemory = NULL;

  aSTG.tymed          = TYMED_HGLOBAL;
  aSTG.pUnkForRelease = NULL;

  
  
  
  
  
  
  
  
  DWORD allocLen = (DWORD)len;
  if ( aFE.cfFormat == CF_TEXT ) {
    
    
    char* plainTextData = nsnull;
    PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, data);
    PRInt32 plainTextLen = 0;
    nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText ( castedUnicode, len / 2, &plainTextData, &plainTextLen );
   
    
    
    nsMemory::Free(data);
    if ( plainTextData ) {
      data = plainTextData;
      allocLen = plainTextLen + sizeof(char);
    }
    else {
      NS_WARNING ( "Oh no, couldn't convert unicode to plain text" );
      return ResultFromScode(S_OK);
    }
  }
  else if ( aFE.cfFormat == nsClipboard::CF_HTML ) {
    
    
    NS_ConvertUTF16toUTF8 converter ( NS_REINTERPRET_CAST(PRUnichar*, data) );
    char* utf8HTML = nsnull;
    nsresult rv = BuildPlatformHTML ( converter.get(), &utf8HTML );      
    
    nsMemory::Free(data);
    if ( NS_SUCCEEDED(rv) && utf8HTML ) {
      
      data = utf8HTML;
      allocLen = strlen(utf8HTML) + sizeof(char);
    }
    else {
      NS_WARNING ( "Oh no, couldn't convert to HTML" );
      return ResultFromScode(S_OK);
    }
  }
  else {
    
    
    allocLen += sizeof(PRUnichar);
  }

  hGlobalMemory = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, allocLen);

  
  if ( hGlobalMemory ) {
    char* dest = NS_REINTERPRET_CAST(char*, GlobalLock(hGlobalMemory));
    char* source = NS_REINTERPRET_CAST(char*, data);
    memcpy ( dest, source, allocLen );                         
    GlobalUnlock(hGlobalMemory);
  }
  aSTG.hGlobal = hGlobalMemory;

  
  nsMemory::Free(data);

  return ResultFromScode(S_OK);
}


HRESULT nsDataObj::GetMetafilePict(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObj::SetBitmap(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObj::SetDib   (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}


HRESULT nsDataObj::SetText  (FORMATETC& aFE, STGMEDIUM& aSTG)
{
  if (aFE.cfFormat == CF_TEXT && aFE.tymed ==  TYMED_HGLOBAL) {
		HGLOBAL hString = (HGLOBAL)aSTG.hGlobal;
		if(hString == NULL)
			return(FALSE);

		
		char *  pString = (char *) GlobalLock(hString);    
		if(!pString)
			return(FALSE);

		GlobalUnlock(hString);
    nsAutoString str; str.AssignWithConversion(pString);

  }
	return ResultFromScode(E_FAIL);
}


HRESULT nsDataObj::SetMetafilePict (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}





CLSID nsDataObj::GetClassID() const
{
	return CLSID_nsDataObj;
}




void nsDataObj::AddDataFlavor(const char* aDataFlavor, LPFORMATETC aFE)
{
  
  
  
  

  
  
#ifndef WINCE
  if (aFE->cfFormat == CF_HDROP) {
    return;
  }  
  else 
#endif
  {
    mDataFlavors->AppendElement(new nsCAutoString(aDataFlavor));
    m_enumFE->AddFE(aFE);
  }
}




void nsDataObj::SetTransferable(nsITransferable * aTransferable)
{
    NS_IF_RELEASE(mTransferable);

  mTransferable = aTransferable;
  if (nsnull == mTransferable) {
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
  
  PRUint32 len = 0;
  nsCOMPtr<nsISupports> genericURL;
  if ( NS_SUCCEEDED(mTransferable->GetTransferData(kURLMime, getter_AddRefs(genericURL), &len)) ) {
    nsCOMPtr<nsISupportsString> urlObject ( do_QueryInterface(genericURL) );
    if ( urlObject ) {
      nsAutoString url;
      urlObject->GetData ( url );
      outURL = url;

      
      
      PRInt32 lineIndex = outURL.FindChar ( '\n' );
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
  
  PRUint32 len = 0;
  nsCOMPtr<nsISupports> genericURL;
  if ( NS_SUCCEEDED(mTransferable->GetTransferData(kURLMime, getter_AddRefs(genericURL), &len)) ) {
    nsCOMPtr<nsISupportsString> urlObject ( do_QueryInterface(genericURL) );
    if ( urlObject ) {
      nsAutoString url;
      urlObject->GetData ( url );

      
      
      PRInt32 lineIndex = url.FindChar ( '\n' );
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
  *outPlatformHTML = nsnull;

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

  const PRInt32 kSourceURLLength    = mSourceURL.Length();
  const PRInt32 kNumberLength       = strlen(numPlaceholder);

  const PRInt32 kTotalHeaderLen     = strlen(startHTMLPrefix) +
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

  
  PRInt32 startHTMLOffset = kTotalHeaderLen;
  PRInt32 startFragOffset = startHTMLOffset
                              + htmlHeaderString.Length()
			      + fragmentHeaderString.Length();

  PRInt32 endFragOffset   = startFragOffset
                              + inHTMLString.Length();

  PRInt32 endHTMLOffset   = endFragOffset
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
nsDataObj :: GetUniformResourceLocator( FORMATETC& aFE, STGMEDIUM& aSTG, PRBool aIsUnicode )
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

  char* contents = NS_REINTERPRET_CAST(char*, GlobalLock(hGlobalMemory));
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

  const int totalLen = (url.Length() + 1) * sizeof(PRUnichar);
  HGLOBAL hGlobalMemory = GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE, totalLen);
  if (!hGlobalMemory)
    return E_OUTOFMEMORY;

  wchar_t* contents = NS_REINTERPRET_CAST(wchar_t*, GlobalLock(hGlobalMemory));
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



nsresult nsDataObj::GetDownloadDetails(nsIURI **aSourceURI,
                                       nsAString &aFilename)
{
  NS_ENSURE_TRUE(mTransferable, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsISupports> urlPrimitive;
  PRUint32 dataSize = 0;
  mTransferable->GetTransferData(kFilePromiseURLMime, getter_AddRefs(urlPrimitive), &dataSize);
  nsCOMPtr<nsISupportsString> srcUrlPrimitive = do_QueryInterface(urlPrimitive);
  NS_ENSURE_TRUE(srcUrlPrimitive, NS_ERROR_FAILURE);
  
  
  
  nsAutoString strData;
  srcUrlPrimitive->GetData(strData);
  if (strData.IsEmpty())
    return NS_ERROR_FAILURE;

  
  
  
  nsCAutoString strFileName;
  nsCOMPtr<nsIURI> sourceURI;
  
  PRInt32 nPos = strData.FindChar('\n');
  
  NS_NewURI(aSourceURI, Substring(strData, 0, nPos));
  if (nPos != -1) {
    
    CopyUTF16toUTF8(Substring(strData, nPos + 1, strData.Length()), strFileName);
  } else {
    
    nsCOMPtr<nsIURL> sourceURL = do_QueryInterface(*aSourceURI);
    sourceURL->GetFileName(strFileName);
  }
  
  if (strFileName.IsEmpty())
    return NS_ERROR_FAILURE;

  NS_UnescapeURL(strFileName);
  NS_ConvertUTF8toUTF16 wideFileName(strFileName);

  
  MangleTextToValidFilename(wideFileName);

  aFilename = wideFileName;

  return NS_OK;
}

HRESULT nsDataObj::GetFileDescriptor_IStreamA(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  NS_ENSURE_TRUE(fileGroupDescHandle, E_OUTOFMEMORY);

  LPFILEGROUPDESCRIPTORA fileGroupDescA = NS_REINTERPRET_CAST(LPFILEGROUPDESCRIPTORA, 
                                                              ::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescA) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  nsAutoString wideFileName;
  nsresult rv;
  nsCOMPtr<nsIURI> sourceURI;
  rv = GetDownloadDetails(getter_AddRefs(sourceURI),
                          wideFileName);
  if (NS_FAILED(rv))
  {
    ::GlobalFree(fileGroupDescHandle);
    return E_FAIL;
  }

  nsCAutoString nativeFileName;
  NS_UTF16ToCString(wideFileName, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, nativeFileName);
  
  strncpy(fileGroupDescA->fgd[0].cFileName, nativeFileName.get(), NS_MAX_FILEDESCRIPTOR - 1);
  fileGroupDescA->fgd[0].cFileName[NS_MAX_FILEDESCRIPTOR - 1] = '\0';

  
  fileGroupDescA->cItems = 1;
  fileGroupDescA->fgd[0].dwFlags = FD_PROGRESSUI;

  ::GlobalUnlock( fileGroupDescHandle );
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
}

HRESULT nsDataObj::GetFileDescriptor_IStreamW(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  HGLOBAL fileGroupDescHandle = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_SHARE,sizeof(FILEGROUPDESCRIPTORW));
  NS_ENSURE_TRUE(fileGroupDescHandle, E_OUTOFMEMORY);

  LPFILEGROUPDESCRIPTORW fileGroupDescW = NS_REINTERPRET_CAST(LPFILEGROUPDESCRIPTORW, 
                                                              ::GlobalLock(fileGroupDescHandle));
  if (!fileGroupDescW) {
    ::GlobalFree(fileGroupDescHandle);
    return E_OUTOFMEMORY;
  }

  nsAutoString wideFileName;
  nsresult rv;
  nsCOMPtr<nsIURI> sourceURI;
  rv = GetDownloadDetails(getter_AddRefs(sourceURI),
                          wideFileName);
  if (NS_FAILED(rv))
  {
    ::GlobalFree(fileGroupDescHandle);
    return E_FAIL;
  }

  wcsncpy(fileGroupDescW->fgd[0].cFileName, wideFileName.get(), NS_MAX_FILEDESCRIPTOR - 1);
  fileGroupDescW->fgd[0].cFileName[NS_MAX_FILEDESCRIPTOR - 1] = '\0';
  
  fileGroupDescW->cItems = 1;
  fileGroupDescW->fgd[0].dwFlags = FD_PROGRESSUI;

  ::GlobalUnlock(fileGroupDescHandle);
  aSTG.hGlobal = fileGroupDescHandle;
  aSTG.tymed = TYMED_HGLOBAL;

  return S_OK;
}

HRESULT nsDataObj::GetFileContents_IStream(FORMATETC& aFE, STGMEDIUM& aSTG)
{
  IStream *pStream = NULL;

  nsDataObj::CreateStream(&pStream);
  NS_ENSURE_TRUE(pStream, E_FAIL);

  aSTG.tymed = TYMED_ISTREAM;
  aSTG.pstm = pStream;
  aSTG.pUnkForRelease = pStream;

  return S_OK;
}
