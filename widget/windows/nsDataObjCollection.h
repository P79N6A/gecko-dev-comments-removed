




#ifndef _NSDATAOBJCOLLECTION_H_
#define _NSDATAOBJCOLLECTION_H_

#include <oleidl.h>

#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsDataObj.h"
#include "mozilla/Attributes.h"

#define MULTI_MIME "Mozilla/IDataObjectCollectionFormat"

EXTERN_C const IID IID_IDataObjCollection;



class nsIDataObjCollection : public IUnknown {
public:
  
};






 
class nsDataObjCollection final : public nsIDataObjCollection, public nsDataObj
{
  public:
    nsDataObjCollection();
    ~nsDataObjCollection();

  public: 
    STDMETHODIMP_(ULONG) AddRef        ();
    STDMETHODIMP       QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) Release       ();

  public: 
    virtual HRESULT GetFile(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetText(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFileDescriptors(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    virtual HRESULT GetFirstSupporting(LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    using nsDataObj::GetFile;
    using nsDataObj::GetFileContents;
    using nsDataObj::GetText;

    
    void AddDataFlavor(const char * aDataFlavor, LPFORMATETC aFE);

    
    void AddDataObject(IDataObject * aDataObj);
    int32_t GetNumDataObjects() { return mDataObjects.Length(); }
    nsDataObj* GetDataObjectAt(uint32_t aItem)
            { return mDataObjects.SafeElementAt(aItem, nsRefPtr<nsDataObj>()); }

    
    CLSID GetClassID() const;

  public:
    
    
    
    
    STDMETHODIMP GetData  (LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    
    
    STDMETHODIMP GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pSTM);

    
    
    STDMETHODIMP QueryGetData (LPFORMATETC pFE);

    
    
    
    
    STDMETHODIMP SetData  (LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL release);

  protected:
    ULONG m_cRef;              

    nsTArray<nsRefPtr<nsDataObj> > mDataObjects;
};

#endif
