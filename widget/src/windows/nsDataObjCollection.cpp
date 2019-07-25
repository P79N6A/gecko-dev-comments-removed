





































#include <shlobj.h>

#include "nsDataObjCollection.h"
#include "nsClipboard.h"
#include "IEnumFE.h"

#include <ole2.h>


const IID IID_IDataObjCollection =
  {0x25589c3e, 0x1fac, 0x47b9, {0xbf, 0x43, 0xca, 0xea, 0x89, 0xb7, 0x95, 0x33}};





nsDataObjCollection::nsDataObjCollection()
  : m_cRef(0), mIsAsyncMode(FALSE), mIsInOperation(FALSE)
{
  m_enumFE = new CEnumFormatEtc();
  m_enumFE->AddRef();
}

nsDataObjCollection::~nsDataObjCollection()
{
  mDataFlavors.Clear();
  mDataObjects.Clear();

  m_enumFE->Release();
}



STDMETHODIMP nsDataObjCollection::QueryInterface(REFIID riid, void** ppv)
{
  *ppv=NULL;

  if ( (IID_IUnknown == riid) || (IID_IDataObject  == riid) ) {
    *ppv = static_cast<IDataObject*>(this); 
    AddRef();
    return NOERROR;
  }

  if ( IID_IDataObjCollection  == riid ) {
    *ppv = static_cast<nsIDataObjCollection*>(this); 
    AddRef();
    return NOERROR;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) nsDataObjCollection::AddRef()
{
  return ++m_cRef;
}

STDMETHODIMP_(ULONG) nsDataObjCollection::Release()
{
  if (0 != --m_cRef)
    return m_cRef;

  delete this;

  return 0;
}

BOOL nsDataObjCollection::FormatsMatch(const FORMATETC& source,
                                       const FORMATETC& target) const
{
  if ((source.cfFormat == target.cfFormat) &&
      (source.dwAspect & target.dwAspect)  &&
      (source.tymed    & target.tymed)) {
    return TRUE;
  } else {
    return FALSE;
  }
}


STDMETHODIMP nsDataObjCollection::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  static CLIPFORMAT fileDescriptorFlavorA =
                               ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
  static CLIPFORMAT fileDescriptorFlavorW =
                               ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
  static CLIPFORMAT fileFlavor = ::RegisterClipboardFormat(CFSTR_FILECONTENTS);

  switch (pFE->cfFormat) {
  case CF_TEXT:
  case CF_UNICODETEXT:
    return GetText(pFE, pSTM);
  case CF_HDROP:
    return GetFile(pFE, pSTM);
  default:
    if (pFE->cfFormat == fileDescriptorFlavorA ||
        pFE->cfFormat == fileDescriptorFlavorW) {
      return GetFileDescriptors(pFE, pSTM);
    }
    if (pFE->cfFormat == fileFlavor) {
      return GetFileContents(pFE, pSTM);
    }
  }
  return GetFirstSupporting(pFE, pSTM);
}

STDMETHODIMP nsDataObjCollection::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  return E_FAIL;
}


STDMETHODIMP nsDataObjCollection::QueryGetData(LPFORMATETC pFE)
{
  UINT format = nsClipboard::GetFormat(MULTI_MIME);

  if (format == pFE->cfFormat) {
    return S_OK;
  }

  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    IDataObject * dataObj = mDataObjects.ElementAt(i);
    if (S_OK == dataObj->QueryGetData(pFE)) {
      return S_OK;
    }
  }

  return DV_E_FORMATETC;
}

STDMETHODIMP nsDataObjCollection::GetCanonicalFormatEtc(LPFORMATETC pFEIn,
                                                        LPFORMATETC pFEOut)
{
  return E_NOTIMPL;
}

STDMETHODIMP nsDataObjCollection::SetData(LPFORMATETC pFE,
                                          LPSTGMEDIUM pSTM,
                                          BOOL fRelease)
{
  
  
  if (mDataObjects.Length() == 0)
    return E_FAIL;
  return mDataObjects.ElementAt(0)->SetData(pFE, pSTM, fRelease);
}

STDMETHODIMP nsDataObjCollection::EnumFormatEtc(DWORD dwDir,
                                                LPENUMFORMATETC *ppEnum)
{
  if (dwDir == DATADIR_GET) {
    
    m_enumFE->Clone(ppEnum);
    if (!(*ppEnum))
      return E_FAIL;
    (*ppEnum)->Reset();
    return S_OK;
  }

  return E_NOTIMPL;
}

STDMETHODIMP nsDataObjCollection::DAdvise(LPFORMATETC pFE,
                                          DWORD dwFlags,
                                          LPADVISESINK pIAdviseSink,
                                          DWORD* pdwConn)
{
  return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP nsDataObjCollection::DUnadvise(DWORD dwConn)
{
  return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP nsDataObjCollection::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
  return OLE_E_ADVISENOTSUPPORTED;
}


HRESULT nsDataObjCollection::AddSetFormat(FORMATETC& aFE)
{
  return S_OK;
}

HRESULT nsDataObjCollection::AddGetFormat(FORMATETC& aFE)
{
  return S_OK;
}


void nsDataObjCollection::AddDataFlavor(const char * aDataFlavor,
                                        LPFORMATETC aFE)
{
  
  
  IEnumFORMATETC * ifEtc;
  FORMATETC fEtc;
  ULONG num;
  if (S_OK != this->EnumFormatEtc(DATADIR_GET, &ifEtc))
    return;
  while (S_OK == ifEtc->Next(1, &fEtc, &num)) {
    NS_ASSERTION(1 == num,
         "Bit off more than we can chew in nsDataObjCollection::AddDataFlavor");
    if (FormatsMatch(fEtc, *aFE)) {
      ifEtc->Release();
      return;
    }
  } 
  ifEtc->Release();
  m_enumFE->AddFormatEtc(aFE);
}


void nsDataObjCollection::AddDataObject(IDataObject * aDataObj)
{
  nsDataObj* dataObj = reinterpret_cast<nsDataObj*>(aDataObj);
  mDataObjects.AppendElement(dataObj);
}


HRESULT nsDataObjCollection::GetFile(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  STGMEDIUM workingmedium;
  FORMATETC fe = *pFE;
  HGLOBAL hGlobalMemory;
  HRESULT hr;
  
  PRUint32 buffersize = sizeof(DROPFILES) + sizeof(PRUnichar);
  PRUint32 alloclen = 0;
  PRUnichar* realbuffer;
  nsAutoString filename;
  
  hGlobalMemory = GlobalAlloc(GHND, buffersize);

  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    nsDataObj* dataObj = mDataObjects.ElementAt(i);
    hr = dataObj->GetData(&fe, &workingmedium);
    if (hr != S_OK) {
      switch (hr) {
      case DV_E_FORMATETC:
        continue;
      default:
        return hr;
      }
    }
    
    PRUnichar* buffer = (PRUnichar*)GlobalLock(workingmedium.hGlobal);
    if (buffer == NULL)
      return E_FAIL;
    buffer += sizeof(DROPFILES)/sizeof(PRUnichar);
    filename = buffer;
    GlobalUnlock(workingmedium.hGlobal);
    ReleaseStgMedium(&workingmedium);
    
    alloclen = (filename.Length() + 1) * sizeof(PRUnichar);
    hGlobalMemory = ::GlobalReAlloc(hGlobalMemory, buffersize + alloclen, GHND);
    if (hGlobalMemory == NULL)
      return E_FAIL;
    realbuffer = (PRUnichar*)((char*)GlobalLock(hGlobalMemory) + buffersize);
    if (!realbuffer)
      return E_FAIL;
    realbuffer--; 
    memcpy(realbuffer, filename.get(), alloclen);
    GlobalUnlock(hGlobalMemory);
    buffersize += alloclen;
  }
  
  
  
  DROPFILES* df = (DROPFILES*)GlobalLock(hGlobalMemory);
  if (!df)
    return E_FAIL;
  df->pFiles = sizeof(DROPFILES); 
  df->fNC    = 0;
  df->pt.x   = 0;
  df->pt.y   = 0;
  df->fWide  = TRUE; 
  GlobalUnlock(hGlobalMemory);
  
  pSTM->tymed = TYMED_HGLOBAL;
  pSTM->pUnkForRelease = NULL; 
  pSTM->hGlobal = hGlobalMemory;
  return S_OK;
}

HRESULT nsDataObjCollection::GetText(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  STGMEDIUM workingmedium;
  FORMATETC fe = *pFE;
  HGLOBAL hGlobalMemory;
  HRESULT hr;
  PRUint32 buffersize = 1;
  PRUint32 alloclen = 0;

  hGlobalMemory = GlobalAlloc(GHND, buffersize);

  if (pFE->cfFormat == CF_TEXT) {
    nsCAutoString text;
    for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
      nsDataObj* dataObj = mDataObjects.ElementAt(i);
      hr = dataObj->GetData(&fe, &workingmedium);
      if (hr != S_OK) {
        switch (hr) {
        case DV_E_FORMATETC:
          continue;
        default:
          return hr;
        }
      }
      
      char* buffer = (char*)GlobalLock(workingmedium.hGlobal);
      if (buffer == NULL)
        return E_FAIL;
      text = buffer;
      GlobalUnlock(workingmedium.hGlobal);
      ReleaseStgMedium(&workingmedium);
      
      alloclen = text.Length();
      hGlobalMemory = ::GlobalReAlloc(hGlobalMemory, buffersize + alloclen,
                                      GHND);
      if (hGlobalMemory == NULL)
        return E_FAIL;
      buffer = ((char*)GlobalLock(hGlobalMemory) + buffersize);
      if (!buffer)
        return E_FAIL;
      buffer--; 
      memcpy(buffer, text.get(), alloclen);
      GlobalUnlock(hGlobalMemory);
      buffersize += alloclen;
    }
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL; 
    pSTM->hGlobal = hGlobalMemory;
    return S_OK;
  }
  if (pFE->cfFormat == CF_UNICODETEXT) {
    buffersize = sizeof(PRUnichar);
    nsAutoString text;
    for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
      nsDataObj* dataObj = mDataObjects.ElementAt(i);
      hr = dataObj->GetData(&fe, &workingmedium);
      if (hr != S_OK) {
        switch (hr) {
        case DV_E_FORMATETC:
          continue;
        default:
          return hr;
        }
      }
      
      PRUnichar* buffer = (PRUnichar*)GlobalLock(workingmedium.hGlobal);
      if (buffer == NULL)
        return E_FAIL;
      text = buffer;
      GlobalUnlock(workingmedium.hGlobal);
      ReleaseStgMedium(&workingmedium);
      
      alloclen = text.Length() * sizeof(PRUnichar);
      hGlobalMemory = ::GlobalReAlloc(hGlobalMemory, buffersize + alloclen,
                                      GHND);
      if (hGlobalMemory == NULL)
        return E_FAIL;
      buffer = (PRUnichar*)((char*)GlobalLock(hGlobalMemory) + buffersize);
      if (!buffer)
        return E_FAIL;
      buffer--; 
      memcpy(buffer, text.get(), alloclen);
      GlobalUnlock(hGlobalMemory);
      buffersize += alloclen;
    }
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL; 
    pSTM->hGlobal = hGlobalMemory;
    return S_OK;
  }

  return E_FAIL;
}

HRESULT nsDataObjCollection::GetFileDescriptors(LPFORMATETC pFE,
                                                LPSTGMEDIUM pSTM)
{
  STGMEDIUM workingmedium;
  FORMATETC fe = *pFE;
  HGLOBAL hGlobalMemory;
  HRESULT hr;
  PRUint32 buffersize = sizeof(FILEGROUPDESCRIPTOR);
  PRUint32 alloclen = sizeof(FILEDESCRIPTOR);

  hGlobalMemory = GlobalAlloc(GHND, buffersize);

  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    nsDataObj* dataObj = mDataObjects.ElementAt(i);
    hr = dataObj->GetData(&fe, &workingmedium);
    if (hr != S_OK) {
      switch (hr) {
      case DV_E_FORMATETC:
        continue;
      default:
        return hr;
      }
    }
    
    FILEDESCRIPTOR* buffer =
     (FILEDESCRIPTOR*)((char*)GlobalLock(workingmedium.hGlobal) + sizeof(UINT));
    if (buffer == NULL)
      return E_FAIL;
    hGlobalMemory = ::GlobalReAlloc(hGlobalMemory, buffersize + alloclen, GHND);
    if (hGlobalMemory == NULL)
      return E_FAIL;
    FILEGROUPDESCRIPTOR* realbuffer =
                                (FILEGROUPDESCRIPTOR*)GlobalLock(hGlobalMemory);
    if (!realbuffer)
      return E_FAIL;
    FILEDESCRIPTOR* copyloc = (FILEDESCRIPTOR*)((char*)realbuffer + buffersize);
    memcpy(copyloc, buffer, sizeof(FILEDESCRIPTOR));
    realbuffer->cItems++;
    GlobalUnlock(hGlobalMemory);
    GlobalUnlock(workingmedium.hGlobal);
    ReleaseStgMedium(&workingmedium);
    buffersize += alloclen;
  }
  pSTM->tymed = TYMED_HGLOBAL;
  pSTM->pUnkForRelease = NULL; 
  pSTM->hGlobal = hGlobalMemory;
  return S_OK;
}

HRESULT nsDataObjCollection::GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  ULONG num = 0;
  ULONG numwanted = (pFE->lindex == -1) ? 0 : pFE->lindex;
  FORMATETC fEtc = *pFE;
  fEtc.lindex = -1;  

  
  
  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    nsDataObj* dataObj = mDataObjects.ElementAt(i);
    if (dataObj->QueryGetData(&fEtc) != S_OK)
      continue;
    if (num == numwanted)
      return dataObj->GetData(pFE, pSTM);
    numwanted++;
  }
  return DV_E_LINDEX;
}

HRESULT nsDataObjCollection::GetFirstSupporting(LPFORMATETC pFE,
                                                LPSTGMEDIUM pSTM)
{
  
  
  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    if (mDataObjects.ElementAt(i)->QueryGetData(pFE) == S_OK)
      return mDataObjects.ElementAt(i)->GetData(pFE, pSTM);
  }
  return DV_E_FORMATETC;
}
