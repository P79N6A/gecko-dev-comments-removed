




































#include "stdafx.h"

#include "ControlSiteIPFrame.h"

CControlSiteIPFrame::CControlSiteIPFrame()
{
    m_hwndFrame = NULL;
}


CControlSiteIPFrame::~CControlSiteIPFrame()
{
}




HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::GetWindow( HWND __RPC_FAR *phwnd)
{
    if (phwnd == NULL)
    {
        return E_INVALIDARG;
    }
    *phwnd = m_hwndFrame;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::ContextSensitiveHelp( BOOL fEnterMode)
{
    return S_OK;
}





HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::GetBorder( LPRECT lprectBorder)
{
    return INPLACE_E_NOTOOLSPACE;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::RequestBorderSpace( LPCBORDERWIDTHS pborderwidths)
{
    return INPLACE_E_NOTOOLSPACE;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::SetBorderSpace( LPCBORDERWIDTHS pborderwidths)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::SetActiveObject( IOleInPlaceActiveObject __RPC_FAR *pActiveObject,  LPCOLESTR pszObjName)
{
    return S_OK;
}





HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::InsertMenus( HMENU hmenuShared,  LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::SetMenu( HMENU hmenuShared,  HOLEMENU holemenu,  HWND hwndActiveObject)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::RemoveMenus( HMENU hmenuShared)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::SetStatusText( LPCOLESTR pszStatusText)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::EnableModeless( BOOL fEnable)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSiteIPFrame::TranslateAccelerator( LPMSG lpmsg,  WORD wID)
{
    return E_NOTIMPL;
}

