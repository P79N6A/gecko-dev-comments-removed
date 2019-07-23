

#ifndef __CBROWSERCTLSITE_H_
#define __CBROWSERCTLSITE_H_

#include "resource.h"       



class ATL_NO_VTABLE CBrowserCtlSite : 
	public CControlSite,
	public IDocHostUIHandler,
	public IDocHostShowUI
{
public:
	CBrowserCtlSite();

DECLARE_REGISTRY_RESOURCEID(IDR_CBROWSERCTLSITE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBrowserCtlSite)
	CCONTROLSITE_INTERFACES()
	COM_INTERFACE_ENTRY(IDocHostUIHandler)
	COM_INTERFACE_ENTRY(IDocHostShowUI)
END_COM_MAP()

	BOOL m_bUseCustomPopupMenu;
	BOOL m_bUseCustomDropTarget;

public:

	virtual HRESULT STDMETHODCALLTYPE ShowContextMenu( DWORD dwID,  POINT __RPC_FAR *ppt,  IUnknown __RPC_FAR *pcmdtReserved,  IDispatch __RPC_FAR *pdispReserved);
	virtual HRESULT STDMETHODCALLTYPE GetHostInfo( DOCHOSTUIINFO __RPC_FAR *pInfo);
	virtual HRESULT STDMETHODCALLTYPE ShowUI( DWORD dwID,  IOleInPlaceActiveObject __RPC_FAR *pActiveObject,  IOleCommandTarget __RPC_FAR *pCommandTarget,  IOleInPlaceFrame __RPC_FAR *pFrame,  IOleInPlaceUIWindow __RPC_FAR *pDoc);
	virtual HRESULT STDMETHODCALLTYPE HideUI(void);
	virtual HRESULT STDMETHODCALLTYPE UpdateUI(void);
	virtual HRESULT STDMETHODCALLTYPE EnableModeless( BOOL fEnable);
	virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate( BOOL fActivate);
	virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate( BOOL fActivate);
	virtual HRESULT STDMETHODCALLTYPE ResizeBorder( LPCRECT prcBorder,  IOleInPlaceUIWindow __RPC_FAR *pUIWindow,  BOOL fRameWindow);
	virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( LPMSG lpMsg,  const GUID __RPC_FAR *pguidCmdGroup,  DWORD nCmdID);
	virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath( LPOLESTR __RPC_FAR *pchKey,  DWORD dw);
	virtual HRESULT STDMETHODCALLTYPE GetDropTarget( IDropTarget __RPC_FAR *pDropTarget,  IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget);
	virtual HRESULT STDMETHODCALLTYPE GetExternal( IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);
	virtual HRESULT STDMETHODCALLTYPE TranslateUrl( DWORD dwTranslate,  OLECHAR __RPC_FAR *pchURLIn,  OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut);
	virtual HRESULT STDMETHODCALLTYPE FilterDataObject( IDataObject __RPC_FAR *pDO,  IDataObject __RPC_FAR *__RPC_FAR *ppDORet);


	virtual HRESULT STDMETHODCALLTYPE ShowMessage( HWND hwnd,  LPOLESTR lpstrText,  LPOLESTR lpstrCaption,  DWORD dwType,  LPOLESTR lpstrHelpFile,  DWORD dwHelpContext, LRESULT __RPC_FAR *plResult);
	virtual HRESULT STDMETHODCALLTYPE ShowHelp( HWND hwnd,  LPOLESTR pszHelpFile,  UINT uCommand,  DWORD dwData,  POINT ptMouse,  IDispatch __RPC_FAR *pDispatchObjectHit);
};

typedef CComObject<CBrowserCtlSite> CBrowserCtlSiteInstance;

#endif 
