









































#ifndef  __AIMM_DEFINED__
#define  __AIMM_DEFINED__

#include <unknwn.h>
#include <imm.h>

#if defined(__cplusplus) && !defined(CINTERFACE)

interface IEnumRegisterWordA;
interface IEnumRegisterWordW;
interface IEnumInputContext;
struct NS_IMEMENUITEMINFOA;
struct NS_IMEMENUITEMINFOW;
    
    interface
    IActiveIMMApp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AssociateContext( 
             HWND hWnd,
             HIMC hIME,
             HIMC __RPC_FAR *phPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEA( 
             HKL hKL,
             HWND hWnd,
             DWORD dwMode,
             REGISTERWORDA __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureIMEW( 
             HKL hKL,
             HWND hWnd,
             DWORD dwMode,
             REGISTERWORDW __RPC_FAR *pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateContext( 
             HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyContext( 
             HIMC hIME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szRegister,
             LPVOID pData,
             IEnumRegisterWordA __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumRegisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szRegister,
             LPVOID pData,
             IEnumRegisterWordW __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeA( 
             HKL hKL,
             HIMC hIMC,
             UINT uEscape,
             LPVOID pData,
             LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EscapeW( 
             HKL hKL,
             HIMC hIMC,
             UINT uEscape,
             LPVOID pData,
             LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListA( 
             HIMC hIMC,
             DWORD dwIndex,
             UINT uBufLen,
             CANDIDATELIST __RPC_FAR *pCandList,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListW( 
             HIMC hIMC,
             DWORD dwIndex,
             UINT uBufLen,
             CANDIDATELIST __RPC_FAR *pCandList,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountA( 
             HIMC hIMC,
             DWORD __RPC_FAR *pdwListSize,
             DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateListCountW( 
             HIMC hIMC,
             DWORD __RPC_FAR *pdwListSize,
             DWORD __RPC_FAR *pdwBufLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCandidateWindow( 
             HIMC hIMC,
             DWORD dwIndex,
             CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontA( 
             HIMC hIMC,
             LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionFontW( 
             HIMC hIMC,
             LOGFONTW __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringA( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LONG __RPC_FAR *plCopied,
             LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionStringW( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LONG __RPC_FAR *plCopied,
             LPVOID pBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositionWindow( 
             HIMC hIMC,
             COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
             HWND hWnd,
             HIMC __RPC_FAR *phIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListA( 
             HKL hKL,
             HIMC hIMC,
             LPSTR pSrc,
             UINT uBufLen,
             UINT uFlag,
             CANDIDATELIST __RPC_FAR *pDst,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionListW( 
             HKL hKL,
             HIMC hIMC,
             LPWSTR pSrc,
             UINT uBufLen,
             UINT uFlag,
             CANDIDATELIST __RPC_FAR *pDst,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversionStatus( 
             HIMC hIMC,
             DWORD __RPC_FAR *pfdwConversion,
             DWORD __RPC_FAR *pfdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultIMEWnd( 
             HWND hWnd,
             HWND __RPC_FAR *phDefWnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionA( 
             HKL hKL,
             UINT uBufLen,
             LPSTR szDescription,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescriptionW( 
             HKL hKL,
             UINT uBufLen,
             LPWSTR szDescription,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineA( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LPSTR pBuf,
             DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGuideLineW( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LPWSTR pBuf,
             DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameA( 
             HKL hKL,
             UINT uBufLen,
             LPSTR szFileName,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIMEFileNameW( 
             HKL hKL,
             UINT uBufLen,
             LPWSTR szFileName,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpenStatus( 
             HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
             HKL hKL,
             DWORD fdwIndex,
             DWORD __RPC_FAR *pdwProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleA( 
             HKL hKL,
             UINT nItem,
             STYLEBUFA __RPC_FAR *pStyleBuf,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegisterWordStyleW( 
             HKL hKL,
             UINT nItem,
             STYLEBUFW __RPC_FAR *pStyleBuf,
             UINT __RPC_FAR *puCopied) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusWindowPos( 
             HIMC hIMC,
             POINT __RPC_FAR *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVirtualKey( 
             HWND hWnd,
             UINT __RPC_FAR *puVirtualKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEA( 
             LPSTR szIMEFileName,
             LPSTR szLayoutText,
             HKL __RPC_FAR *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InstallIMEW( 
             LPWSTR szIMEFileName,
             LPWSTR szLayoutText,
             HKL __RPC_FAR *phKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsIME( 
             HKL hKL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUIMessageA( 
             HWND hWndIME,
             UINT msg,
             WPARAM wParam,
             LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUIMessageW( 
             HWND hWndIME,
             UINT msg,
             WPARAM wParam,
             LPARAM lParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyIME( 
             HIMC hIMC,
             DWORD dwAction,
             DWORD dwIndex,
             DWORD dwValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szRegister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szRegister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseContext( 
             HWND hWnd,
             HIMC hIMC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCandidateWindow( 
             HIMC hIMC,
             CANDIDATEFORM __RPC_FAR *pCandidate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontA( 
             HIMC hIMC,
             LOGFONTA __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionFontW( 
             HIMC hIMC,
             LOGFONTW __RPC_FAR *plf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionStringA( 
             HIMC hIMC,
             DWORD dwIndex,
             LPVOID pComp,
             DWORD dwCompLen,
             LPVOID pRead,
             DWORD dwReadLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionStringW( 
             HIMC hIMC,
             DWORD dwIndex,
             LPVOID pComp,
             DWORD dwCompLen,
             LPVOID pRead,
             DWORD dwReadLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositionWindow( 
             HIMC hIMC,
             COMPOSITIONFORM __RPC_FAR *pCompForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConversionStatus( 
             HIMC hIMC,
             DWORD fdwConversion,
             DWORD fdwSentence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpenStatus( 
             HIMC hIMC,
             BOOL fOpen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusWindowPos( 
             HIMC hIMC,
             POINT __RPC_FAR *pptPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SimulateHotKey( 
             HWND hWnd,
             DWORD dwHotKeyID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szUnregister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szUnregister) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Activate( 
             BOOL fRestoreLayout) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDefWindowProc( 
             HWND hWnd,
             UINT Msg,
             WPARAM wParam,
             LPARAM lParam,
             LRESULT __RPC_FAR *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterClientWindows( 
             ATOM __RPC_FAR *aaClassList,
             UINT uSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePageA( 
             HKL hKL,
             UINT __RPC_FAR *uCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLangId( 
             HKL hKL,
             LANGID __RPC_FAR *plid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AssociateContextEx( 
             HWND hWnd,
             HIMC hIMC,
             DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableIME( 
             DWORD idThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImeMenuItemsA( 
             HIMC hIMC,
             DWORD dwFlags,
             DWORD dwType,
             NS_IMEMENUITEMINFOA __RPC_FAR *pImeParentMenu,
             NS_IMEMENUITEMINFOA __RPC_FAR *pImeMenu,
             DWORD dwSize,
             DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImeMenuItemsW( 
             HIMC hIMC,
             DWORD dwFlags,
             DWORD dwType,
             NS_IMEMENUITEMINFOW __RPC_FAR *pImeParentMenu,
             NS_IMEMENUITEMINFOW __RPC_FAR *pImeMenu,
             DWORD dwSize,
             DWORD __RPC_FAR *pdwResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumInputContext( 
             DWORD idThread,
             IEnumInputContext __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
    interface
    IActiveIMMMessagePumpOwner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE End( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTranslateMessage( 
             const MSG __RPC_FAR *pMsg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( 
             DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( 
             DWORD dwCookie) = 0;
        
    };


#endif 	

#endif 
