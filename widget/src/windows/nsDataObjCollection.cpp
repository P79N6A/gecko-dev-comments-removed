




































#include "nsDataObjCollection.h"
#include "nsITransferable.h"
#include "nsClipboard.h"
#include "IEnumFE.h"

#include <ole2.h>

#if 0
#define PRNTDEBUG(_x) printf(_x);
#define PRNTDEBUG2(_x1, _x2) printf(_x1, _x2);
#define PRNTDEBUG3(_x1, _x2, _x3) printf(_x1, _x2, _x3);
#else
#define PRNTDEBUG(_x)
#define PRNTDEBUG2(_x1, _x2)
#define PRNTDEBUG3(_x1, _x2, _x3)
#endif

EXTERN_C GUID CDECL CLSID_nsDataObjCollection =
{ 0x2d851b91, 0xd4c, 0x11d3, { 0x96, 0xd4, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 } };








nsDataObjCollection::nsDataObjCollection()
  : m_cRef(0), mTransferable(nsnull)
{
  m_enumFE = new CEnumFormatEtc();
  m_enumFE->AddRef();
}




nsDataObjCollection::~nsDataObjCollection()
{
  NS_IF_RELEASE(mTransferable);

  mDataFlavors.Clear();
  mDataObjects.Clear();

  m_enumFE->Release();
}





STDMETHODIMP nsDataObjCollection::QueryInterface(REFIID riid, void** ppv)
{
	*ppv=NULL;

	if ( (IID_IUnknown == riid) || (IID_IDataObject	== riid) ) {
		*ppv = static_cast<IDataObject*>(this); 
		AddRef();
		return NOERROR;
	}

	if ( IID_IDataObjCollection	== riid ) {
		*ppv = static_cast<nsIDataObjCollection*>(this); 
		AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_NOINTERFACE);
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


BOOL nsDataObjCollection::FormatsMatch(const FORMATETC& source, const FORMATETC& target) const
{
	if ((source.cfFormat == target.cfFormat) &&
		 (source.dwAspect  & target.dwAspect)  &&
		 (source.tymed     & target.tymed))       {
		return TRUE;
	} else {
		return FALSE;
	}
}




STDMETHODIMP nsDataObjCollection::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  PRNTDEBUG("nsDataObjCollection::GetData\n");
  PRNTDEBUG3("  format: %d  Text: %d\n", pFE->cfFormat, CF_TEXT);

  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    IDataObject * dataObj = mDataObjects.ElementAt(i);
    if (S_OK == dataObj->GetData(pFE, pSTM)) {
      return S_OK;
    }
  }

	return ResultFromScode(DATA_E_FORMATETC);
}



STDMETHODIMP nsDataObjCollection::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
  PRNTDEBUG("nsDataObjCollection::GetDataHere\n");
		return ResultFromScode(E_FAIL);
}






STDMETHODIMP nsDataObjCollection::QueryGetData(LPFORMATETC pFE)
{
  UINT format = nsClipboard::GetFormat(MULTI_MIME);
  PRNTDEBUG("nsDataObjCollection::QueryGetData  ");

  PRNTDEBUG3("format: %d  Mulitple: %d\n", pFE->cfFormat, format);

  if (format == pFE->cfFormat) {
    return S_OK;
  }


  for (PRUint32 i = 0; i < mDataObjects.Length(); ++i) {
    IDataObject * dataObj = mDataObjects.ElementAt(i);
    if (S_OK == dataObj->QueryGetData(pFE)) {
      return S_OK;
    }
  }

  PRNTDEBUG2("***** nsDataObjCollection::QueryGetData - Unknown format %d\n", pFE->cfFormat);
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObjCollection::GetCanonicalFormatEtc
	 (LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
  PRNTDEBUG("nsDataObjCollection::GetCanonicalFormatEtc\n");
		return ResultFromScode(E_FAIL);
}



STDMETHODIMP nsDataObjCollection::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
  PRNTDEBUG("nsDataObjCollection::SetData\n");

  return ResultFromScode(E_FAIL);
}



STDMETHODIMP nsDataObjCollection::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
  PRNTDEBUG("nsDataObjCollection::EnumFormatEtc\n");

  if (dwDir == DATADIR_GET) {
    
    return m_enumFE->Clone(ppEnum);
  }

  return E_NOTIMPL;
}


STDMETHODIMP nsDataObjCollection::DAdvise(LPFORMATETC pFE, DWORD dwFlags,
										                      LPADVISESINK pIAdviseSink, DWORD* pdwConn)
{
  PRNTDEBUG("nsDataObjCollection::DAdvise\n");
	return ResultFromScode(E_FAIL);
}



STDMETHODIMP nsDataObjCollection::DUnadvise(DWORD dwConn)
{
  PRNTDEBUG("nsDataObjCollection::DUnadvise\n");
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP nsDataObjCollection::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
  PRNTDEBUG("nsDataObjCollection::EnumDAdvise\n");
	return ResultFromScode(E_FAIL);
}




HRESULT nsDataObjCollection::AddSetFormat(FORMATETC& aFE)
{
  PRNTDEBUG("nsDataObjCollection::AddSetFormat\n");
	return ResultFromScode(S_OK);
}


HRESULT nsDataObjCollection::AddGetFormat(FORMATETC& aFE)
{
  PRNTDEBUG("nsDataObjCollection::AddGetFormat\n");
	return ResultFromScode(S_OK);
}


HRESULT nsDataObjCollection::GetBitmap(FORMATETC&, STGMEDIUM&)
{
  PRNTDEBUG("nsDataObjCollection::GetBitmap\n");
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObjCollection::GetDib(FORMATETC&, STGMEDIUM&)
{
  PRNTDEBUG("nsDataObjCollection::GetDib\n");
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObjCollection::GetMetafilePict(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObjCollection::SetBitmap(FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_NOTIMPL);
}


HRESULT nsDataObjCollection::SetDib   (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}


HRESULT nsDataObjCollection::SetMetafilePict (FORMATETC&, STGMEDIUM&)
{
	return ResultFromScode(E_FAIL);
}



CLSID nsDataObjCollection::GetClassID() const
{
	return CLSID_nsDataObjCollection;
}




void nsDataObjCollection::AddDataFlavor(nsString * aDataFlavor, LPFORMATETC aFE)
{
  
  
  
  
  mDataFlavors.AppendElement(*aDataFlavor);
  m_enumFE->AddFormatEtc(aFE);
}




void nsDataObjCollection::AddDataObject(IDataObject * aDataObj)
{
  mDataObjects.AppendElement(aDataObj);
}
