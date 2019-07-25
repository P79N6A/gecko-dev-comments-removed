




































#ifndef DROPTARGET_H
#define DROPTARGET_H

#include "MozillaBrowser.h"



class CDropTarget : public CComObjectRootEx<CComSingleThreadModel>,
                    public IDropTarget
{
public:
    CDropTarget();
    virtual ~CDropTarget();

BEGIN_COM_MAP(CDropTarget)
    COM_INTERFACE_ENTRY(IDropTarget)
END_COM_MAP()

    
    CIPtr(IDataObject) m_spDataObject;

    
    CMozillaBrowser *m_pOwner;

    
    void SetOwner(CMozillaBrowser *pOwner);
    
    HRESULT CDropTarget::GetURLFromFile(const TCHAR *pszFile, tstring &szURL);


    virtual HRESULT STDMETHODCALLTYPE DragEnter( IDataObject __RPC_FAR *pDataObj,  DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver( DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave(void);
    virtual HRESULT STDMETHODCALLTYPE Drop( IDataObject __RPC_FAR *pDataObj,  DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect);
};

typedef CComObject<CDropTarget> CDropTargetInstance;

#endif