





































#ifndef ITEMCONTAINER_H
#define ITEMCONTAINER_H





class CItemContainer :  public CComObjectRootEx<CComSingleThreadModel>,
                        public IOleItemContainer
{

public:

    CItemContainer();
    virtual ~CItemContainer();

BEGIN_COM_MAP(CItemContainer)
    COM_INTERFACE_ENTRY_IID(IID_IParseDisplayName, IOleItemContainer)
    COM_INTERFACE_ENTRY_IID(IID_IOleContainer, IOleItemContainer)
    COM_INTERFACE_ENTRY_IID(IID_IOleItemContainer, IOleItemContainer)
END_COM_MAP()

    
    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName( IBindCtx __RPC_FAR *pbc,  LPOLESTR pszDisplayName,  ULONG __RPC_FAR *pchEaten,  IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);

    
    virtual HRESULT STDMETHODCALLTYPE EnumObjects( DWORD grfFlags,  IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum);
    virtual HRESULT STDMETHODCALLTYPE LockContainer( BOOL fLock);

    
    virtual  HRESULT STDMETHODCALLTYPE GetObject( LPOLESTR pszItem,  DWORD dwSpeedNeeded,  IBindCtx __RPC_FAR *pbc,  REFIID riid,  void __RPC_FAR *__RPC_FAR *ppvObject);
    virtual  HRESULT STDMETHODCALLTYPE GetObjectStorage( LPOLESTR pszItem,  IBindCtx __RPC_FAR *pbc,  REFIID riid,  void __RPC_FAR *__RPC_FAR *ppvStorage);
    virtual HRESULT STDMETHODCALLTYPE IsRunning( LPOLESTR pszItem);  
};

typedef CComObject<CItemContainer> CItemContainerInstance;


#endif