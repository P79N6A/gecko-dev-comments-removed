




































#ifndef CONTROLSITEIPFRAME_H
#define CONTROLSITEIPFRAME_H

class CControlSiteIPFrame :    public CComObjectRootEx<CComSingleThreadModel>,
                            public IOleInPlaceFrame
{
public:
    CControlSiteIPFrame();
    virtual ~CControlSiteIPFrame();

    HWND m_hwndFrame;

BEGIN_COM_MAP(CControlSiteIPFrame)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleInPlaceFrame)
    COM_INTERFACE_ENTRY_IID(IID_IOleInPlaceUIWindow, IOleInPlaceFrame)
    COM_INTERFACE_ENTRY(IOleInPlaceFrame)
END_COM_MAP()

    
    virtual  HRESULT STDMETHODCALLTYPE GetWindow( HWND __RPC_FAR *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp( BOOL fEnterMode);

    
    virtual  HRESULT STDMETHODCALLTYPE GetBorder( LPRECT lprectBorder);
    virtual  HRESULT STDMETHODCALLTYPE RequestBorderSpace( LPCBORDERWIDTHS pborderwidths);
    virtual  HRESULT STDMETHODCALLTYPE SetBorderSpace( LPCBORDERWIDTHS pborderwidths);
    virtual HRESULT STDMETHODCALLTYPE SetActiveObject( IOleInPlaceActiveObject __RPC_FAR *pActiveObject,  LPCOLESTR pszObjName);

    
    virtual HRESULT STDMETHODCALLTYPE InsertMenus( HMENU hmenuShared,  LPOLEMENUGROUPWIDTHS lpMenuWidths);
    virtual  HRESULT STDMETHODCALLTYPE SetMenu( HMENU hmenuShared,  HOLEMENU holemenu,  HWND hwndActiveObject);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenus( HMENU hmenuShared);
    virtual  HRESULT STDMETHODCALLTYPE SetStatusText( LPCOLESTR pszStatusText);
    virtual HRESULT STDMETHODCALLTYPE EnableModeless( BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( LPMSG lpmsg,  WORD wID);
};

typedef CComObject<CControlSiteIPFrame> CControlSiteIPFrameInstance;

#endif