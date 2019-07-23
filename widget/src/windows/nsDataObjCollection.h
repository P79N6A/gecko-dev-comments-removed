




































#ifndef _NSDATAOBJCOLLECTION_H_
#define _NSDATAOBJCOLLECTION_H_

#define INITGUID

#ifdef __MINGW32__
#include <unknwn.h>
#include <basetyps.h>
#include <objidl.h>
#endif
#include <oleidl.h>

#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class CEnumFormatEtc;
class nsITransferable;

#define MULTI_MIME "Mozilla/IDataObjectCollectionFormat"

#if NOT_YET

EXTERN_C GUID CDECL IID_DATA_OBJ_COLLECTION = 
    { 0x6e99c280, 0xd820, 0x11d3, { 0xbb, 0x6f, 0xbb, 0xf2, 0x6b, 0xfe, 0x62, 0x3c } };


class nsPIDataObjCollection : IUnknown
{
public:

  STDMETHODIMP AddDataObject(IDataObject * aDataObj) = 0;
  STDMETHODIMP GetNumDataObjects(PRInt32* outNum) = 0;
  STDMETHODIMP GetDataObjectAt(PRUint32 aItem, IDataObject** outItem) = 0;

};
#endif


DEFINE_GUID(IID_IDataObjCollection, 
0x25589c3e, 0x1fac, 0x47b9, 0xbf, 0x43, 0xca, 0xea, 0x89, 0xb7, 0x95, 0x33);



class nsIDataObjCollection : public IUnknown {
public:
  
};






 
class nsDataObjCollection : public IDataObject, public nsIDataObjCollection 
{
	public: 
		nsDataObjCollection();
		~nsDataObjCollection();

	public: 
		STDMETHODIMP_(ULONG) AddRef        ();
		STDMETHODIMP 			QueryInterface(REFIID, void**);
		STDMETHODIMP_(ULONG) Release       ();

	public: 
		virtual HRESULT AddSetFormat(FORMATETC&  FE);
		virtual HRESULT AddGetFormat(FORMATETC&  FE);

		virtual HRESULT GetBitmap(FORMATETC&  FE, STGMEDIUM&  STM);
		virtual HRESULT GetDib   (FORMATETC&  FE, STGMEDIUM&  STM);
		virtual HRESULT GetMetafilePict(FORMATETC&  FE, STGMEDIUM&  STM);

		virtual HRESULT SetBitmap(FORMATETC&  FE, STGMEDIUM&  STM);
		virtual HRESULT SetDib   (FORMATETC&  FE, STGMEDIUM&  STM);
		virtual HRESULT SetMetafilePict(FORMATETC&  FE, STGMEDIUM&  STM);

    
    void AddDataFlavor(nsString * aDataFlavor, LPFORMATETC aFE);
    void SetTransferable(nsITransferable * aTransferable);

#if NOT_YET
    
    STDMETHODIMP AddDataObject(IDataObject * aDataObj);
    STDMETHODIMP GetNumDataObjects(PRInt32* outNum) { *outNum = mDataObjects.Length(); }
    STDMETHODIMP GetDataObjectAt(PRUint32 aItem, IDataObject** outItem) { *outItem = mDataObjects.SafeElementAt(aItem, nsRefPtr<IDataObject>()); }
#endif

    
    void AddDataObject(IDataObject * aDataObj);
    PRInt32 GetNumDataObjects() { return mDataObjects.Length(); }
    IDataObject* GetDataObjectAt(PRUint32 aItem) { return mDataObjects.SafeElementAt(aItem, nsRefPtr<IDataObject>()); }

		
		CLSID GetClassID() const;

	public: 
			  

		
		
		
		
		STDMETHODIMP GetData	(LPFORMATETC pFE, LPSTGMEDIUM pSTM);

		
		
		STDMETHODIMP GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pSTM);

		
		
		STDMETHODIMP QueryGetData (LPFORMATETC pFE);

		
		
		
		STDMETHODIMP GetCanonicalFormatEtc (LPFORMATETC pFE, LPFORMATETC pCanonFE);

		
		
		
		
		STDMETHODIMP SetData	(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL release);

		
		
		
		STDMETHODIMP EnumFormatEtc	(DWORD direction, LPENUMFORMATETC* ppEnum);

		
		
		
		STDMETHODIMP DAdvise	(LPFORMATETC pFE, DWORD flags, LPADVISESINK pAdvise,
									 DWORD* pConn);

		
		STDMETHODIMP DUnadvise (DWORD pConn);

		
		
      
		STDMETHODIMP EnumDAdvise (LPENUMSTATDATA *ppEnum);

	public: 

		
		

		
		

	protected:
    BOOL FormatsMatch(const FORMATETC& source, const FORMATETC& target) const;

		ULONG        m_cRef;              

    nsTArray<nsString> mDataFlavors;

    nsITransferable  * mTransferable; 
                                      

    CEnumFormatEtc   * m_enumFE;      
                                      

    nsTArray<nsRefPtr<IDataObject> > mDataObjects;
};


#endif
