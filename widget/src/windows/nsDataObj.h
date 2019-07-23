




































#ifndef _NSDATAOBJ_H_
#define _NSDATAOBJ_H_

#ifdef __MINGW32__
#include <unknwn.h>
#include <basetyps.h>
#include <objidl.h>
#endif
#include <oleidl.h>

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsTPtrArray.h"



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

#ifdef __MINGW32__
# include <w32api.h>
# if __W32API_MAJOR_VERSION < 3 || (__W32API_MAJOR_VERSION == 3 && __W32API_MINOR_VERSION == 0)
#  ifndef FILEGROUPDESCRIPTORA
#   define FILEGROUPDESCRIPTORA    FILEGROUPDESCRIPTOR
#  endif
#  ifndef LPFILEGROUPDESCRIPTORA
#   define LPFILEGROUPDESCRIPTORA  LPFILEGROUPDESCRIPTOR
#  endif
typedef struct _FILEDESCRIPTORW {
   DWORD dwFlags;
   CLSID clsid;
   SIZEL sizel;
   POINTL pointl;
   DWORD dwFileAttributes;
   FILETIME ftCreationTime;
   FILETIME ftLastAccessTime;
   FILETIME ftLastWriteTime;
   DWORD nFileSizeHigh;
   DWORD nFileSizeLow;
   WCHAR cFileName[MAX_PATH];
} FILEDESCRIPTORW,*LPFILEDESCRIPTORW;
typedef struct _FILEGROUPDESCRIPTORW {
   UINT cItems;
   FILEDESCRIPTORW fgd[1];
} FILEGROUPDESCRIPTORW,*LPFILEGROUPDESCRIPTORW;
# endif 
#endif 

class CEnumFormatEtc;
class nsITransferable;






class nsDataObj : public IDataObject,
                  public IAsyncOperation
{
  public: 
    nsDataObj(nsIURI *uri = nsnull);
    ~nsDataObj();

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

    
    nsresult GetDownloadDetails(nsIURI **aSourceURI,
                                nsAString &aFilename);

	protected:
	  
    PRBool IsFlavourPresent(const char *inFlavour);

		virtual HRESULT AddSetFormat(FORMATETC&  FE);
		virtual HRESULT AddGetFormat(FORMATETC&  FE);

		virtual HRESULT GetFile ( FORMATETC& aFE, STGMEDIUM& aSTG );
		virtual HRESULT GetText ( const nsACString& aDF, FORMATETC& aFE, STGMEDIUM & aSTG );
		virtual HRESULT GetBitmap ( const nsACString& inFlavor, FORMATETC&  FE, STGMEDIUM&  STM);
		virtual HRESULT GetDib ( const nsACString& inFlavor, FORMATETC &, STGMEDIUM & aSTG );
		virtual HRESULT GetMetafilePict(FORMATETC&  FE, STGMEDIUM&  STM);
        
        virtual HRESULT DropImage( FORMATETC& aFE, STGMEDIUM& aSTG );
        virtual HRESULT DropFile( FORMATETC& aFE, STGMEDIUM& aSTG );

    virtual HRESULT GetUniformResourceLocator ( FORMATETC& aFE, STGMEDIUM& aSTG, PRBool aIsUnicode ) ;
    virtual HRESULT ExtractUniformResourceLocatorA ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT ExtractUniformResourceLocatorW ( FORMATETC& aFE, STGMEDIUM& aSTG ) ;
    virtual HRESULT GetFileDescriptor ( FORMATETC& aFE, STGMEDIUM& aSTG, PRBool aIsUnicode ) ;
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
    
    
    
    
    class CStream : public IStream
    {
      ULONG mRefCount;  
      nsCOMPtr<nsIInputStream> mInputStream;
      nsCOMPtr<nsIChannel> mChannel;

    protected:
      virtual ~CStream();
      

    public:
      CStream();
      nsresult Init(nsIURI *pSourceURI);

      
      STDMETHOD(QueryInterface)(REFIID refiid, void** ppvResult);
      STDMETHOD_(ULONG, AddRef)(void);
      STDMETHOD_(ULONG, Release)(void);

      
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

    HRESULT FindFORMATETC(FORMATETC *pfe, LPDATAENTRY *ppde, BOOL fAdd);
    HRESULT AddRefStgMedium(STGMEDIUM *pstgmIn, STGMEDIUM *pstgmOut,
                            BOOL fCopyIn);
    IUnknown* GetCanonicalIUnknown(IUnknown *punk);
    HGLOBAL GlobalClone(HGLOBAL hglobIn);
};


#endif  

