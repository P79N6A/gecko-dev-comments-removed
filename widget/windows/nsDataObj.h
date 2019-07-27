




#ifndef _NSDATAOBJ_H_
#define _NSDATAOBJ_H_

#include <oleidl.h>
#include <shldisp.h>

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsIURI.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsCOMArray.h"
#include "nsITimer.h"

class nsIThread;
class nsINode;




#ifdef __IDataObjectAsyncCapability_INTERFACE_DEFINED__
#define IAsyncOperation IDataObjectAsyncCapability
#define IID_IAsyncOperation IID_IDataObjectAsyncCapability
#else


#ifndef __IAsyncOperation_INTERFACE_DEFINED__

EXTERN_C const IID IID_IAsyncOperation;

MIDL_INTERFACE("3D8B0590-F691-11d2-8EA9-006097DF5BD4")
IAsyncOperation : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL *pfIsOpAsync) = 0;
  virtual HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx *pbcReserved) = 0;
  virtual HRESULT STDMETHODCALLTYPE InOperation(BOOL *pfInAsyncOp) = 0;
  virtual HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult,
                                                 IBindCtx *pbcReserved,
                                                 DWORD dwEffects) = 0;
};

#ifndef FD_PROGRESSUI
  #define FD_PROGRESSUI 0x4000
#endif

#endif 
#endif 






#ifndef CFSTR_INETURLA
#define CFSTR_INETURLA    L"UniformResourceLocator"
#endif
#ifndef CFSTR_INETURLW
#define CFSTR_INETURLW    L"UniformResourceLocatorW"
#endif





#ifndef CFSTR_FILEDESCRIPTORA
# define CFSTR_FILEDESCRIPTORA   L"FileGroupDescriptor"
#endif
#ifndef CFSTR_FILEDESCRIPTORW
# define CFSTR_FILEDESCRIPTORW   L"FileGroupDescriptorW"
#endif

class CEnumFormatEtc;
class nsITransferable;






class nsDataObj : public IDataObject,
                  public IAsyncOperation
{

protected:
  nsCOMPtr<nsIThread> mIOThread;

  public: 
    nsDataObj(nsIURI *uri = nullptr);
    virtual ~nsDataObj();

	public: 
		STDMETHODIMP_(ULONG) AddRef        ();
		STDMETHODIMP 			QueryInterface(REFIID, void**);
		STDMETHODIMP_(ULONG) Release       ();

    
    void AddDataFlavor(const char* aDataFlavor, LPFORMATETC aFE);
    void SetTransferable(nsITransferable * aTransferable);

		
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

    
    STDMETHOD(EndOperation)(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);
    STDMETHOD(GetAsyncMode)(BOOL *pfIsOpAsync);
    STDMETHOD(InOperation)(BOOL *pfInAsyncOp);
    STDMETHOD(SetAsyncMode)(BOOL fDoOpAsync);
    STDMETHOD(StartOperation)(IBindCtx *pbcReserved);

	public: 

    
    HRESULT GetDownloadDetails(nsIURI **aSourceURI,
                               nsAString &aFilename);

	protected:
    
    bool IsFlavourPresent(const char *inFlavour);

    virtual HRESULT AddSetFormat(FORMATETC&  FE);
    virtual HRESULT AddGetFormat(FORMATETC&  FE);

    virtual HRESULT GetFile ( FORMATETC& aFE, STGMEDIUM& aSTG );
    virtual HRESULT GetText ( const nsACString& aDF, FORMATETC& aFE, STGMEDIUM & aSTG );
    virtual HRESULT GetDib ( const nsACString& inFlavor, FORMATETC &, STGMEDIUM & aSTG );
    virtual HRESULT GetMetafilePict(FORMATETC&  FE, STGMEDIUM&  STM);
        
    virtual HRESULT DropImage( FORMATETC& aFE, STGMEDIUM& aSTG );
    virtual HRESULT DropFile( FORMATETC& aFE, STGMEDIUM& aSTG );
    virtual HRESULT DropTempFile( FORMATETC& aFE, STGMEDIUM& aSTG );

    virtual HRESULT GetUniformResourceLocator ( FORMATETC& aFE, STGMEDIUM& aSTG, bool aIsUnicode ) ;
    virtual HRESULT ExtractUniformResourceLocatorA ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT ExtractUniformResourceLocatorW ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileDescriptor ( FORMATETC& aFE, STGMEDIUM& aSTG, bool aIsUnicode ) ;
    virtual HRESULT GetFileContents ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetPreferredDropEffect ( FORMATETC& aFE, STGMEDIUM& aSTG );

    virtual HRESULT SetBitmap(FORMATETC&  FE, STGMEDIUM&  STM);
    virtual HRESULT SetDib   (FORMATETC&  FE, STGMEDIUM&  STM);
    virtual HRESULT SetText  (FORMATETC&  FE, STGMEDIUM&  STM);
    virtual HRESULT SetMetafilePict(FORMATETC&  FE, STGMEDIUM&  STM);

      
    virtual HRESULT GetFileDescriptorInternetShortcutA ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileDescriptorInternetShortcutW ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileContentsInternetShortcut ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;

    
    virtual HRESULT GetFileDescriptor_IStreamA ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileDescriptor_IStreamW ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileContents_IStream ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;

    nsresult ExtractShortcutURL ( nsString & outURL ) ;
    nsresult ExtractShortcutTitle ( nsString & outTitle ) ;

      
    nsresult BuildPlatformHTML ( const char* inOurHTML, char** outPlatformHTML ) ;

    
    nsCString mSourceURL;

    BOOL FormatsMatch(const FORMATETC& source, const FORMATETC& target) const;

		ULONG        m_cRef;              

    nsTArray<nsCString> mDataFlavors;

    nsITransferable  * mTransferable; 
                                      

    CEnumFormatEtc   * m_enumFE;      
                                      

    nsCOMPtr<nsIFile> mCachedTempFile;

    BOOL mIsAsyncMode;
    BOOL mIsInOperation;
    
    
    
    
    class CStream : public IStream, public nsIStreamListener
    {
      nsCOMPtr<nsIChannel> mChannel;
      nsTArray<uint8_t> mChannelData;
      bool mChannelRead;
      nsresult mChannelResult;
      uint32_t mStreamRead;

    protected:
      virtual ~CStream();
      nsresult WaitForCompletion();

    public:
      CStream();
      nsresult Init(nsIURI *pSourceURI, nsINode* aRequestingNode);

      NS_DECL_ISUPPORTS
      NS_DECL_NSIREQUESTOBSERVER
      NS_DECL_NSISTREAMLISTENER

      
      STDMETHOD(QueryInterface)(REFIID refiid, void** ppvResult);

      
      STDMETHOD(Clone)(IStream** ppStream);
      STDMETHOD(Commit)(DWORD dwFrags);
      STDMETHOD(CopyTo)(IStream* pDestStream, ULARGE_INTEGER nBytesToCopy, ULARGE_INTEGER* nBytesRead, ULARGE_INTEGER* nBytesWritten);
      STDMETHOD(LockRegion)(ULARGE_INTEGER nStart, ULARGE_INTEGER nBytes, DWORD dwFlags);
      STDMETHOD(Read)(void* pvBuffer, ULONG nBytesToRead, ULONG* nBytesRead);
      STDMETHOD(Revert)(void);
      STDMETHOD(Seek)(LARGE_INTEGER nMove, DWORD dwOrigin, ULARGE_INTEGER* nNewPos);
      STDMETHOD(SetSize)(ULARGE_INTEGER nNewSize);
      STDMETHOD(Stat)(STATSTG* statstg, DWORD dwFlags);
      STDMETHOD(UnlockRegion)(ULARGE_INTEGER nStart, ULARGE_INTEGER nBytes, DWORD dwFlags);
      STDMETHOD(Write)(const void* pvBuffer, ULONG nBytesToRead, ULONG* nBytesRead);
    };

    HRESULT CreateStream(IStream **outStream);

  private:

    
    typedef struct {
      FORMATETC   fe;
      STGMEDIUM   stgm;
    } DATAENTRY, *LPDATAENTRY;

    nsTArray <LPDATAENTRY> mDataEntryList;
    nsCOMPtr<nsITimer> mTimer;

    bool LookupArbitraryFormat(FORMATETC *aFormat, LPDATAENTRY *aDataEntry, BOOL aAddorUpdate);
    bool CopyMediumData(STGMEDIUM *aMediumDst, STGMEDIUM *aMediumSrc, LPFORMATETC aFormat, BOOL aSetData);
    static void RemoveTempFile(nsITimer* aTimer, void* aClosure);
};


#endif  

