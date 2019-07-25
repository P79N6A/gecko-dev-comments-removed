




































#ifndef CONTROLSITE_H
#define CONTROLSITE_H

#include "IOleCommandTargetImpl.h"

#include "PropertyList.h"




#define CCONTROLSITE_INTERFACES() \
    COM_INTERFACE_ENTRY(IOleWindow) \
    COM_INTERFACE_ENTRY(IOleClientSite) \
    COM_INTERFACE_ENTRY(IOleInPlaceSite) \
    COM_INTERFACE_ENTRY_IID(IID_IOleInPlaceSite, IOleInPlaceSiteWindowless) \
    COM_INTERFACE_ENTRY_IID(IID_IOleInPlaceSiteEx, IOleInPlaceSiteWindowless) \
    COM_INTERFACE_ENTRY(IOleControlSite) \
    COM_INTERFACE_ENTRY(IDispatch) \
    COM_INTERFACE_ENTRY_IID(IID_IAdviseSink, IAdviseSinkEx) \
    COM_INTERFACE_ENTRY_IID(IID_IAdviseSink2, IAdviseSinkEx) \
    COM_INTERFACE_ENTRY_IID(IID_IAdviseSinkEx, IAdviseSinkEx) \
    COM_INTERFACE_ENTRY(IOleCommandTarget) \
    COM_INTERFACE_ENTRY(IServiceProvider) \
    COM_INTERFACE_ENTRY(IBindStatusCallback) \
    COM_INTERFACE_ENTRY(IWindowForBindingUI)









class CControlSiteSecurityPolicy
{
public:
    
    virtual BOOL IsClassSafeToHost(const CLSID & clsid) = 0;
    
    virtual BOOL IsClassMarkedSafeForScripting(const CLSID & clsid, BOOL &bClassExists) = 0;
    
    virtual BOOL IsObjectSafeForScripting(IUnknown *pObject, const IID &iid) = 0;
};
























class CControlSite :    public CComObjectRootEx<CComSingleThreadModel>,
                        public CControlSiteSecurityPolicy,
                        public IOleClientSite,
                        public IOleInPlaceSiteWindowless,
                        public IOleControlSite,
                        public IAdviseSinkEx,
                        public IDispatch,
                        public IServiceProvider,
                        public IOleCommandTargetImpl<CControlSite>,
                        public IBindStatusCallback,
                        public IWindowForBindingUI
{
public:

    
    HWND m_hWndParent;
    
    RECT m_rcObjectPos;
    
    unsigned m_bSetClientSiteFirst:1;
    
    unsigned m_bVisibleAtRuntime:1;
    
    unsigned m_bInPlaceActive:1;
    
    unsigned m_bUIActive:1;
    
    unsigned m_bInPlaceLocked:1;
    
    unsigned m_bSupportWindowlessActivation:1;
    
    unsigned m_bWindowless:1;
    
    unsigned m_bSafeForScriptingObjectsOnly:1;
    
    CComPtr<IServiceProvider> m_spServiceProvider;
    
    CComPtr<IOleContainer> m_spContainer;
    
    static CControlSiteSecurityPolicy *GetDefaultControlSecurityPolicy();

protected:

    
    CComPtr<IUnknown> m_spObject;
    
    CComQIPtr<IViewObject, &IID_IViewObject> m_spIViewObject;
    
    CComQIPtr<IOleObject, &IID_IOleObject> m_spIOleObject;
    
    CComQIPtr<IOleInPlaceObject, &IID_IOleInPlaceObject> m_spIOleInPlaceObject;
    
    CComQIPtr<IOleInPlaceObjectWindowless, &IID_IOleInPlaceObjectWindowless> m_spIOleInPlaceObjectWindowless;
    
    CLSID m_CLSID;
    
    PropertyList m_ParameterList;
    
    CControlSiteSecurityPolicy *m_pSecurityPolicy;


    
    unsigned m_bBindingInProgress;
    
    HRESULT m_hrBindResult;


    
    RECT m_rcBuffer;
    
    HBITMAP m_hBMBuffer;
    
    HBITMAP m_hBMBufferOld;
    
    HDC m_hDCBuffer;
    
    HRGN m_hRgnBuffer;
    
    DWORD m_dwBufferFlags;


    
    LCID m_nAmbientLocale;
    
    COLORREF m_clrAmbientForeColor;
    
    COLORREF m_clrAmbientBackColor;
    
    bool m_bAmbientShowHatching:1;
    
    bool m_bAmbientShowGrabHandles:1;
    
    bool m_bAmbientUserMode:1;
    
    bool m_bAmbientAppearance:1;

protected:
    
    virtual void FireAmbientPropertyChange(DISPID id);

public:

    
    CControlSite();
    
    virtual ~CControlSite();

BEGIN_COM_MAP(CControlSite)
    CCONTROLSITE_INTERFACES()
END_COM_MAP()

BEGIN_OLECOMMAND_TABLE()
END_OLECOMMAND_TABLE()

    
    HWND GetCommandTargetWindow()
    {
        return NULL; 
    }


    
    virtual HRESULT Create(REFCLSID clsid, PropertyList &pl = PropertyList(),
        LPCWSTR szCodebase = NULL, IBindCtx *pBindContext = NULL);
    
    virtual HRESULT Attach(HWND hwndParent, const RECT &rcPos, IUnknown *pInitStream = NULL);
    
    virtual HRESULT Detach();
    
    virtual HRESULT GetControlUnknown(IUnknown **ppObject);
    
    virtual HRESULT SetPosition(const RECT &rcPos);
    
    virtual HRESULT Draw(HDC hdc);
    
    virtual HRESULT DoVerb(LONG nVerb, LPMSG lpMsg = NULL);
    
    virtual HRESULT Advise(IUnknown *pIUnkSink, const IID &iid, DWORD *pdwCookie);
    
    virtual HRESULT Unadvise(const IID &iid, DWORD dwCookie);
    
    virtual void SetServiceProvider(IServiceProvider *pSP)
    {
        m_spServiceProvider = pSP;
    }
    virtual void SetContainer(IOleContainer *pContainer)
    {
        m_spContainer = pContainer;
    }
    
    
    virtual void SetSecurityPolicy(CControlSiteSecurityPolicy *pSecurityPolicy)
    {
        m_pSecurityPolicy = pSecurityPolicy;
    }
    virtual CControlSiteSecurityPolicy *GetSecurityPolicy() const
    {
        return m_pSecurityPolicy;
    }


    virtual void SetAmbientUserMode(BOOL bUser);


    
    virtual const CLSID &GetObjectCLSID() const
    {
        return m_CLSID;
    }
    
    virtual BOOL IsObjectValid() const
    {
        return (m_spObject) ? TRUE : FALSE;
    }
    
    virtual HWND GetParentWindow() const
    {
        return m_hWndParent;
    }
    
    virtual BOOL IsInPlaceActive() const
    {
        return m_bInPlaceActive;
    }


    
    virtual BOOL IsClassSafeToHost(const CLSID & clsid);
    
    virtual BOOL IsClassMarkedSafeForScripting(const CLSID & clsid, BOOL &bClassExists);
    
    virtual BOOL IsObjectSafeForScripting(IUnknown *pObject, const IID &iid);
    
    virtual BOOL IsObjectSafeForScripting(const IID &iid);


    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppv);


    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( UINT __RPC_FAR *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( UINT iTInfo,  LCID lcid,  ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( REFIID riid,  LPOLESTR __RPC_FAR *rgszNames,  UINT cNames,  LCID lcid,  DISPID __RPC_FAR *rgDispId);
    virtual  HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember,  REFIID riid,  LCID lcid,  WORD wFlags,  DISPPARAMS __RPC_FAR *pDispParams,  VARIANT __RPC_FAR *pVarResult,  EXCEPINFO __RPC_FAR *pExcepInfo,  UINT __RPC_FAR *puArgErr);


    virtual  void STDMETHODCALLTYPE OnDataChange( FORMATETC __RPC_FAR *pFormatetc,  STGMEDIUM __RPC_FAR *pStgmed);
    virtual  void STDMETHODCALLTYPE OnViewChange( DWORD dwAspect,  LONG lindex);
    virtual  void STDMETHODCALLTYPE OnRename( IMoniker __RPC_FAR *pmk);
    virtual  void STDMETHODCALLTYPE OnSave(void);
    virtual  void STDMETHODCALLTYPE OnClose(void);


    virtual  void STDMETHODCALLTYPE OnLinkSrcChange( IMoniker __RPC_FAR *pmk);


    virtual  void STDMETHODCALLTYPE OnViewStatusChange( DWORD dwViewStatus);


    virtual  HRESULT STDMETHODCALLTYPE GetWindow( HWND __RPC_FAR *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp( BOOL fEnterMode);


    virtual HRESULT STDMETHODCALLTYPE SaveObject(void);
    virtual HRESULT STDMETHODCALLTYPE GetMoniker( DWORD dwAssign,  DWORD dwWhichMoniker,  IMoniker __RPC_FAR *__RPC_FAR *ppmk);
    virtual HRESULT STDMETHODCALLTYPE GetContainer( IOleContainer __RPC_FAR *__RPC_FAR *ppContainer);
    virtual HRESULT STDMETHODCALLTYPE ShowObject(void);
    virtual HRESULT STDMETHODCALLTYPE OnShowWindow( BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout(void);


    virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void);
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate(void);
    virtual HRESULT STDMETHODCALLTYPE GetWindowContext( IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame,  IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc,  LPRECT lprcPosRect,  LPRECT lprcClipRect,  LPOLEINPLACEFRAMEINFO lpFrameInfo);
    virtual HRESULT STDMETHODCALLTYPE Scroll( SIZE scrollExtant);
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate( BOOL fUndoable);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void);
    virtual HRESULT STDMETHODCALLTYPE DiscardUndoState(void);
    virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChange( LPCRECT lprcPosRect);


    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx( BOOL __RPC_FAR *pfNoRedraw,  DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx( BOOL fNoRedraw);
    virtual HRESULT STDMETHODCALLTYPE RequestUIActivate(void);


    virtual HRESULT STDMETHODCALLTYPE CanWindowlessActivate(void);
    virtual HRESULT STDMETHODCALLTYPE GetCapture(void);
    virtual HRESULT STDMETHODCALLTYPE SetCapture( BOOL fCapture);
    virtual HRESULT STDMETHODCALLTYPE GetFocus(void);
    virtual HRESULT STDMETHODCALLTYPE SetFocus( BOOL fFocus);
    virtual HRESULT STDMETHODCALLTYPE GetDC( LPCRECT pRect,  DWORD grfFlags,  HDC __RPC_FAR *phDC);
    virtual HRESULT STDMETHODCALLTYPE ReleaseDC( HDC hDC);
    virtual HRESULT STDMETHODCALLTYPE InvalidateRect( LPCRECT pRect,  BOOL fErase);
    virtual HRESULT STDMETHODCALLTYPE InvalidateRgn( HRGN hRGN,  BOOL fErase);
    virtual HRESULT STDMETHODCALLTYPE ScrollRect( INT dx,  INT dy,  LPCRECT pRectScroll,  LPCRECT pRectClip);
    virtual HRESULT STDMETHODCALLTYPE AdjustRect( LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE OnDefWindowMessage( UINT msg,  WPARAM wParam,  LPARAM lParam,  LRESULT __RPC_FAR *plResult);


    virtual HRESULT STDMETHODCALLTYPE OnControlInfoChanged(void);
    virtual HRESULT STDMETHODCALLTYPE LockInPlaceActive( BOOL fLock);
    virtual HRESULT STDMETHODCALLTYPE GetExtendedControl( IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE TransformCoords( POINTL __RPC_FAR *pPtlHimetric,  POINTF __RPC_FAR *pPtfContainer,  DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( MSG __RPC_FAR *pMsg,  DWORD grfModifiers);
    virtual HRESULT STDMETHODCALLTYPE OnFocus( BOOL fGotFocus);
    virtual HRESULT STDMETHODCALLTYPE ShowPropertyFrame( void);


    virtual HRESULT STDMETHODCALLTYPE OnStartBinding( DWORD dwReserved,  IBinding __RPC_FAR *pib);
    virtual HRESULT STDMETHODCALLTYPE GetPriority( LONG __RPC_FAR *pnPriority);
    virtual HRESULT STDMETHODCALLTYPE OnLowResource( DWORD reserved);
    virtual HRESULT STDMETHODCALLTYPE OnProgress( ULONG ulProgress,  ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding( HRESULT hresult,  LPCWSTR szError);
    virtual  HRESULT STDMETHODCALLTYPE GetBindInfo(  DWORD __RPC_FAR *grfBINDF,  BINDINFO __RPC_FAR *pbindinfo);
    virtual  HRESULT STDMETHODCALLTYPE OnDataAvailable( DWORD grfBSCF,  DWORD dwSize,  FORMATETC __RPC_FAR *pformatetc,  STGMEDIUM __RPC_FAR *pstgmed);
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable( REFIID riid,  IUnknown __RPC_FAR *punk);


    virtual HRESULT STDMETHODCALLTYPE GetWindow( REFGUID rguidReason,  HWND *phwnd);
};

typedef CComObject<CControlSite> CControlSiteInstance;



#endif