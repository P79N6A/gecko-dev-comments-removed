






































#ifndef __PLUGINHOSTCTRL_H_
#define __PLUGINHOSTCTRL_H_

#include "resource.h"       
#include <atlctl.h>

#include "nsPluginHostWnd.h"
#include "nsPluginWnd.h"



class ATL_NO_VTABLE nsPluginHostCtrl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<nsPluginHostCtrl, IMozPluginHostCtrl, &IID_IMozPluginHostCtrl, &LIBID_PLUGINHOSTCTRLLib>,
	public CComControl<nsPluginHostCtrl, nsPluginHostWnd>,
	public IPersistStreamInitImpl<nsPluginHostCtrl>,
	public IOleControlImpl<nsPluginHostCtrl>,
	public IOleObjectImpl<nsPluginHostCtrl>,
	public IOleInPlaceActiveObjectImpl<nsPluginHostCtrl>,
	public IViewObjectExImpl<nsPluginHostCtrl>,
	public IOleInPlaceObjectWindowlessImpl<nsPluginHostCtrl>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<nsPluginHostCtrl>,
	public IPersistStorageImpl<nsPluginHostCtrl>,
    public IPersistPropertyBagImpl<nsPluginHostCtrl>,
	public ISpecifyPropertyPagesImpl<nsPluginHostCtrl>,
	public IQuickActivateImpl<nsPluginHostCtrl>,
	public IDataObjectImpl<nsPluginHostCtrl>,
	public IProvideClassInfo2Impl<&CLSID_MozPluginHostCtrl, &DIID__IMozPluginHostCtrlEvents, &LIBID_PLUGINHOSTCTRLLib>,
	public IPropertyNotifySinkCP<nsPluginHostCtrl>,
	public CComCoClass<nsPluginHostCtrl, &CLSID_MozPluginHostCtrl>
{
protected:
    virtual ~nsPluginHostCtrl();

public:
	nsPluginHostCtrl();

DECLARE_REGISTRY_RESOURCEID(IDR_PLUGINHOSTCTRL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(nsPluginHostCtrl)
	COM_INTERFACE_ENTRY(IMozPluginHostCtrl)
	COM_INTERFACE_ENTRY2(IDispatch, IMozPluginHostCtrl)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)

	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(nsPluginHostCtrl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("HWND", DISPID_HWND, CLSID_NULL)
	PROP_ENTRY("Text", DISPID_TEXT, CLSID_NULL)
    
    PROP_ENTRY("type", 1, CLSID_NULL)
    PROP_ENTRY("src", 2, CLSID_NULL)
    PROP_ENTRY("pluginspage", 3, CLSID_NULL)
	
	
	
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(nsPluginHostCtrl)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(nsPluginHostWnd)
	CHAIN_MSG_MAP(nsPluginHostWnd)
END_MSG_MAP()








	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_IMozPluginHostCtrl,
		};
		for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i], riid))
				return S_OK;
		}
		return S_FALSE;
	}


    virtual HRESULT GetWebBrowserApp(IWebBrowserApp **pBrowser);


	DECLARE_VIEW_STATUS(0)


	STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);


public:
	STDMETHOD(get_PluginSource)( BSTR *pVal);
	STDMETHOD(put_PluginSource)( BSTR newVal);
	STDMETHOD(get_PluginContentType)( BSTR *pVal);
	STDMETHOD(put_PluginContentType)( BSTR newVal);
	STDMETHOD(get_PluginsPage)( BSTR *pVal);
	STDMETHOD(put_PluginsPage)( BSTR newVal);
};

#endif 
