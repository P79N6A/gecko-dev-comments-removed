





































#ifndef _NSDATAOBJCOLLECTION_H_
#define _NSDATAOBJCOLLECTION_H_

#ifdef __MINGW32__
#include <unknwn.h>
#include <basetyps.h>
#include <objidl.h>
#endif
#include <oleidl.h>

#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsDataObj.h"

class CEnumFormatEtc;

#define MULTI_MIME "Mozilla/IDataObjectCollectionFormat"

EXTERN_C const IID IID_IDataObjCollection;



class nsIDataObjCollection : public IUnknown {
public:
  
};






 
class nsDataObjCollection : public nsIDataObjCollection, public nsDataObj
{
  public:
    nsDataObjCollection();
    ~nsDataObjCollection();

  public: 
    STDMETHODIMP_(ULONG) AddRef        ();
    STDMETHODIMP       QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) Release       ();

  public: 
    virtual HRESULT AddSetFormat(FORMATETC&  FE);
    virtual HRESULT AddGetFormat(FORMATETC&  FE);

    virtual HRESULT GetFile(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetText(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFileDescriptors(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFirstSupporting(LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    
    void AddDataFlavor(const char * aDataFlavor, LPFORMATETC aFE);

    
    void AddDataObject(IDataObject * aDataObj);
    PRInt32 GetNumDataObjects() { return mDataObjects.Length(); }
    nsDataObj* GetDataObjectAt(PRUint32 aItem)
            { return mDataObjects.SafeElementAt(aItem, nsRefPtr<nsDataObj>()); }

    
    CLSID GetClassID() const;

  public:
    
    
    
    
    STDMETHODIMP GetData  (LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    
    
    STDMETHODIMP GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    
    
    STDMETHODIMP QueryGetData (LPFORMATETC pFE);

    
    
    
    STDMETHODIMP GetCanonicalFormatEtc (LPFORMATETC pFE, LPFORMATETC pCanonFE);

    
    
    
    
    STDMETHODIMP SetData  (LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL release);

    
    
    
    STDMETHODIMP EnumFormatEtc  (DWORD direction, LPENUMFORMATETC* ppEnum);

    
    
    
    STDMETHODIMP DAdvise  (LPFORMATETC pFE, DWORD flags, LPADVISESINK pAdvise,
                   DWORD* pConn);

    
    STDMETHODIMP DUnadvise (DWORD pConn);

    
    
      
    STDMETHODIMP EnumDAdvise (LPENUMSTATDATA *ppEnum);

  public:
    
    

    
    

  protected:
    BOOL FormatsMatch(const FORMATETC& source, const FORMATETC& target) const;

    ULONG m_cRef;              

    
    CEnumFormatEtc   * m_enumFE;

    nsTArray<nsRefPtr<nsDataObj> > mDataObjects;
    
    BOOL mIsAsyncMode;
    BOOL mIsInOperation;
};

#endif
