




































#ifndef XPCDispPrivate_h__
#define XPCDispPrivate_h__






#ifndef xpcprivate_h___
#error "DispPrivate.h should not be included directly, please use XPCPrivate.h"
#endif




#include <atlbase.h>



#include <comdef.h>



#include "objsafe.h"




#undef GetClassInfo
#undef GetClassName
#undef GetMessage


#include "nsIDispatchSupport.h"


#define NS_DECL_IUNKNOWN                                                      \
public:                                                                       \
  STDMETHOD(QueryInterface)(REFIID aIID,                                      \
                            void** aInstancePtr);                             \
  STDMETHODIMP_(ULONG) AddRef(void);                                          \
  STDMETHODIMP_(ULONG) Release(void);                                         \
protected:                                                                    \
  ULONG mRefCnt;                                                       

#define NS_IMPL_COM_QUERY_HEAD(_class)                                        \
STDMETHODIMP _class::QueryInterface(REFIID aIID, void** aInstancePtr)         \
{                                                                             \
  NS_ASSERTION(aInstancePtr,                                                  \
               "QueryInterface requires a non-NULL destination!");            \
  if( !aInstancePtr )                                                         \
    return E_POINTER;                                                         \
  IUnknown* foundInterface;

#define NS_IMPL_COM_QUERY_BODY(_interface)                                    \
  if(IsEqualIID(aIID, __uuidof(_interface)) )                                 \
    foundInterface = static_cast<_interface*>(this);                          \
  else

#define NS_IMPL_COM_QUERY_TAIL_GUTS                                           \
    foundInterface = 0;                                                       \
  HRESULT status;                                                             \
  if( !foundInterface )                                                       \
    status = E_NOINTERFACE;                                                   \
  else                                                                        \
    {                                                                         \
      NS_ADDREF(foundInterface);                                              \
      status = S_OK;                                                          \
    }                                                                         \
  *aInstancePtr = foundInterface;                                             \
  return status;                                                              \
}

#define NS_COM_MAP_BEGIN(_implClass)      NS_IMPL_COM_QUERY_HEAD(_implClass)
#define NS_COM_MAP_ENTRY(_interface)      NS_IMPL_COM_QUERY_BODY(_interface)
#define NS_COM_MAP_END                    NS_IMPL_COM_QUERY_TAIL_GUTS

#define NS_COM_IMPL_ADDREF(_class)                                            \
STDMETHODIMP_(ULONG) _class::AddRef(void)                                     \
{                                                                             \
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");                   \
  NS_ASSERT_OWNINGTHREAD(_class);                                             \
  ++mRefCnt;                                                                  \
  NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                       \
  return mRefCnt;                                                             \
}

#define NS_COM_IMPL_RELEASE(_class)                                           \
STDMETHODIMP_(ULONG) _class::Release(void)                                    \
{                                                                             \
  NS_PRECONDITION(0 != mRefCnt, "dup release");                               \
  NS_ASSERT_OWNINGTHREAD(_class);                                             \
  --mRefCnt;                                                                  \
  NS_LOG_RELEASE(this, mRefCnt, #_class);                                     \
  if(mRefCnt == 0) {                                                         \
    mRefCnt = 1; /* stabilize */                                              \
    delete this;                                                              \
    return 0;                                                                 \
  }                                                                           \
  return mRefCnt;                                                             \
}

extern const nsID NSID_IDISPATCH;






class XPCDispConvert
{
public:
    





    static
    VARTYPE JSTypeToCOMType(XPCCallContext& ccx, jsval val);

    







    static
    JSBool JSToCOM(XPCCallContext& ccx, jsval src, VARIANT & dest, 
                   nsresult& err, JSBool isByRef = JS_FALSE);

    







    static
    JSBool COMToJS(XPCCallContext& ccx, const VARIANT & src, jsval & dest,
                   nsresult& err);
private:
    







    static
    JSBool JSArrayToCOMArray(XPCCallContext& ccx, JSObject *obj, VARIANT & var,
                          nsresult& err);
    







    static
    JSBool COMArrayToJSArray(XPCCallContext& ccx, const VARIANT & src,
                             jsval & dest, nsresult& err);
};










JSBool
XPC_IDispatch_CallMethod(JSContext *cx, JSObject *obj, uintN argc,
                         jsval *argv, jsval *vp);









JSBool
XPC_IDispatch_GetterSetter(JSContext *cx, JSObject *obj, uintN argc, 
                           jsval *argv, jsval *vp);






class XPCDispNameArray
{
public:
    


    XPCDispNameArray();
    


    ~XPCDispNameArray();
    



    void SetSize(PRUint32 size);
    



    PRUint32 GetSize() const;
    





    void SetName(DISPID dispid, nsAString const & name);
    




    const nsAString&  GetName(DISPID dispid) const;
    




    DISPID Find(const nsAString &target) const;
private:
    PRUint32 mCount;
    nsString* mNames;
};





class XPCDispIDArray
{
public:
    




    XPCDispIDArray(XPCCallContext& ccx, JSIdArray* array);

    



    PRUint32 Length() const;

    





    jsval Item(JSContext* cx, PRUint32 index) const;

    


    void TraceJS(JSTracer* trc);

    


    void Unmark();

    


    JSBool IsMarked() const;
private:
    JSBool mMarked;
    nsTArray<jsid> mIDArray;
};




class XPCDispTypeInfo : public ITypeInfo
{
    NS_DECL_IUNKNOWN
public:
    


    class FuncDescArray
    {
    public:
        


        FuncDescArray(XPCCallContext& ccx, JSObject* obj, 
                      const XPCDispIDArray& array, XPCDispNameArray & names);
        


        ~FuncDescArray();
        




        FUNCDESC* Get(PRUint32 index);
        





        void Release(FUNCDESC *);
        



        PRUint32 Length() const;
    private:
        nsTArray<FUNCDESC> mArray;
        





        PRBool BuildFuncDesc(XPCCallContext& ccx, JSObject* obj,
                           XPCDispJSPropertyInfo & propInfo);
    };
    



    static
    XPCDispTypeInfo* New(XPCCallContext& ccx, JSObject* obj);
    virtual ~XPCDispTypeInfo();
    
    STDMETHOD(GetTypeAttr)( 
         TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr);
    
    STDMETHOD(GetTypeComp)(
         ITypeComp __RPC_FAR *__RPC_FAR *ppTComp);
    
    STDMETHOD(GetFuncDesc)( 
         UINT index,
         FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc);
    
    STDMETHOD(GetVarDesc)( 
         UINT index,
         VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc);
    
    STDMETHOD(GetNames)( 
         MEMBERID memid,
         BSTR __RPC_FAR *rgBstrNames,
         UINT cMaxNames,
         UINT __RPC_FAR *pcNames);
    
    STDMETHOD(GetRefTypeOfImplType)( 
         UINT index,
         HREFTYPE __RPC_FAR *pRefType);
    
    STDMETHOD(GetImplTypeFlags)( 
         UINT index,
         INT __RPC_FAR *pImplTypeFlags);
    
    STDMETHOD(GetIDsOfNames)( 
         LPOLESTR __RPC_FAR *rgszNames,
         UINT cNames,
         MEMBERID __RPC_FAR *pMemId);
    
    STDMETHOD(Invoke)( 
         PVOID pvInstance,
         MEMBERID memid,
         WORD wFlags,
         DISPPARAMS __RPC_FAR *pDispParams,
         VARIANT __RPC_FAR *pVarResult,
         EXCEPINFO __RPC_FAR *pExcepInfo,
         UINT __RPC_FAR *puArgErr);
    
    STDMETHOD(GetDocumentation)( 
         MEMBERID memid,
         BSTR __RPC_FAR *pBstrName,
         BSTR __RPC_FAR *pBstrDocString,
         DWORD __RPC_FAR *pdwHelpContext,
         BSTR __RPC_FAR *pBstrHelpFile);
    
    STDMETHOD(GetDllEntry)( 
         MEMBERID memid,
         INVOKEKIND invKind,
         BSTR __RPC_FAR *pBstrDllName,
         BSTR __RPC_FAR *pBstrName,
         WORD __RPC_FAR *pwOrdinal);
    
    STDMETHOD(GetRefTypeInfo)( 
         HREFTYPE hRefType,
         ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    
    STDMETHOD(AddressOfMember)( 
         MEMBERID memid,
         INVOKEKIND invKind,
         PVOID __RPC_FAR *ppv);
    
    STDMETHOD(CreateInstance)( 
         IUnknown __RPC_FAR *pUnkOuter,
         REFIID riid,
         PVOID __RPC_FAR *ppvObj);
    
    STDMETHOD(GetMops)( 
         MEMBERID memid,
         BSTR __RPC_FAR *pBstrMops);
    
    STDMETHOD(GetContainingTypeLib)( 
         ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
         UINT __RPC_FAR *pIndex);
    
    virtual  void STDMETHODCALLTYPE ReleaseTypeAttr( 
         TYPEATTR __RPC_FAR *pTypeAttr);
    
    virtual  void STDMETHODCALLTYPE ReleaseFuncDesc( 
         FUNCDESC __RPC_FAR *pFuncDesc);
    
    virtual  void STDMETHODCALLTYPE ReleaseVarDesc( 
         VARDESC __RPC_FAR *pVarDesc);
    




    const nsAString& GetNameForDispID(DISPID dispID);
private:
    





    XPCDispTypeInfo(XPCCallContext& ccx, JSObject* obj, XPCDispIDArray* array);
    JSObject*               mJSObject;
    XPCDispIDArray*         mIDArray;
    XPCDispNameArray        mNameArray;
    
    
    FuncDescArray        mFuncDescArray;
};




class XPCDispJSPropertyInfo
{
public:
    






    XPCDispJSPropertyInfo(JSContext*cx, PRUint32 memid, JSObject* obj, jsval val);
    



    PRBool Valid() const;
    




    PRUint32 GetParamCount() const;
    




    PRUint32 GetMemID() const;
    



    INVOKEKIND GetInvokeKind() const;
    




    void GetReturnType(XPCCallContext& ccx, ELEMDESC & elemDesc);
    




    ELEMDESC * GetParamInfo();
    



    PRBool IsProperty() const;
    



    PRBool IsReadOnly() const;
    



    PRBool IsSetter() const;
    


    void SetSetter();
    



    nsAString const & GetName() const;
private:
    enum property_type
    {
        INVALID,
        PROPERTY,
        READONLY_PROPERTY,
        FUNCTION,
        SETTER_MODE = 0x20
    };

    PRUint32        mPropertyType;
    PRUint32        mParamCount;
    PRUint32        mMemID;
    jsval           mProperty;
    nsString        mName;

    



    inline
    property_type PropertyType() const;
};




class XPCDispatchTearOff : public IDispatch, public ISupportErrorInfo
{
public:
    


    XPCDispatchTearOff(nsIXPConnectWrappedJS * wrappedJS);
    


    virtual ~XPCDispatchTearOff();
    



    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    


    STDMETHODIMP_(ULONG) AddRef();
    


    STDMETHODIMP_(ULONG) Release();
    







    STDMETHOD(QueryInterface)(REFIID IID,void ** pPtr);
    





    STDMETHOD(GetTypeInfoCount)(unsigned int * pctinfo);
    




    STDMETHOD(GetTypeInfo)(unsigned int iTInfo, LCID lcid, 
                           ITypeInfo FAR* FAR* ppTInfo);
    




    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR FAR* FAR* rgszNames, 
                             unsigned int cNames, LCID  lcid, 
                             DISPID FAR* rgDispId);
    




    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                      DISPPARAMS FAR* pDispParams, VARIANT FAR* pVarResult, 
                      EXCEPINFO FAR* pExcepInfo, unsigned int FAR* puArgErr);

private:
    
    nsCOMPtr<nsIXPConnectWrappedJS> mWrappedJS;
    
    XPCDispTypeInfo *                 mCOMTypeInfo;
    
    ULONG                             mRefCnt;
    
    XPCDispTypeInfo *                 GetCOMTypeInfo();
    
    inline
    JSObject* GetJSObject();

    NS_DECL_OWNINGTHREAD;
};







class XPCDispInterface
{
public:
    



    class Member
    {
    public:
        



        class ParamInfo
        {
        public:
            



            ParamInfo(const ELEMDESC * paramInfo);
            




            JSBool InitializeOutputParam(void * varBuffer, 
                                         VARIANT & var) const;
            




            PRBool IsFlagSet(unsigned short flag) const;
            



            PRBool IsIn() const;
            



            PRBool IsOut() const;
            



            PRBool IsOptional() const;
            



            PRBool IsRetVal() const;
            
            



            VARTYPE GetType() const;
        private:
            const ELEMDESC * mParamInfo;
        };
        Member();
        ~Member();
        




        void* operator new(size_t, Member* p) CPP_THROW_NEW;
        



        PRBool IsSetter() const;
        



        PRBool IsGetter() const;
        



        PRBool IsProperty() const;
        



        PRBool IsParameterizedSetter() const;
        



        PRBool IsParameterizedGetter() const;
        



        PRBool IsParameterizedProperty() const;
        



        PRBool IsFunction() const;
        



        jsval GetName() const;
        






        JSBool GetValue(XPCCallContext& ccx, XPCNativeInterface* iface, 
                        jsval * retval) const;
        



        PRUint32 GetDispID() const;
        




        PRUint32 GetParamCount(PRBool getter = PR_FALSE) const;
        





        ParamInfo GetParamInfo(PRUint32 index, PRBool getter = PR_FALSE) const;
        
        



        void SetName(jsval name);
        



        void MakeGetter();
        


        void MakeSetter();
        



        void SetFunction();
        


        void ResetType();
        




 
        void SetTypeInfo(DISPID dispID, ITypeInfo* pTypeInfo,
                         FUNCDESC* funcdesc);
        



        void SetGetterFuncDesc(FUNCDESC* funcdesc);
        



        void SetMemID(DISPID memID);
        



        DISPID GetMemID() const;

    private:
       DISPID   mMemID;
        



        enum member_type
        {
            UNINITIALIZED = 0,
            SET_PROPERTY = 1,
            GET_PROPERTY = 2,
            FUNCTION = 4,
            RESOLVED = 8
        };
        PRUint16 mType;
        jsval mVal;     
        jsval mName;    
        CComPtr<ITypeInfo> mTypeInfo;
        FUNCDESC* mFuncDesc; 
        FUNCDESC* mGetterFuncDesc; 
        




        PRUint16 GetParamType(PRUint32 index) const;
        




        PRBool IsFlagSet(unsigned short flag) const;
    };
    




    JSObject* GetJSObject() const;
    




    void SetJSObject(JSObject* jsobj);
    




    const Member * FindMember(jsval name) const;
    






    const Member* FindMemberCI(XPCCallContext& ccx, jsval name) const;
    




    const Member & GetMember(PRUint32 index);
    



    PRUint32 GetMemberCount() const;
    





    static
    XPCDispInterface* NewInstance(JSContext* cx, nsISupports * pIface);
    



    void operator delete(void * p);
    


    ~XPCDispInterface();
private:
    





    XPCDispInterface(JSContext* cx, 
                          ITypeInfo * pTypeInfo,
                          PRUint32 members);
    




    void * operator new (size_t, PRUint32 members) CPP_THROW_NEW;

    



    JSObject*   mJSObject;
    PRUint32    mMemberCount;
    Member      mMembers[1];
    







    PRBool InspectIDispatch(JSContext * cx, ITypeInfo * pTypeInfo, 
                          PRUint32 members);

    





    class Allocator
    {
    public:
        





        Allocator(JSContext * cx, ITypeInfo * pTypeInfo);
        


        inline
        ~Allocator();
        



        inline
        XPCDispInterface* Allocate();
    private:
        DISPID * mMemIDs;
        PRUint32 mCount;            
        PRUint32 mIDispatchMembers; 
        JSContext* mCX;
        ITypeInfo* mTypeInfo;

        



        inline
        PRUint32 Count() const;
        



        void Add(DISPID memID);
        



        inline
        PRBool Valid() const;

        
        Allocator(const Allocator&);
        Allocator& operator =(const Allocator&);
    };
    


    friend class Allocator;
};






class XPCDispObject
{
public:
    enum CallMode {CALL_METHOD, CALL_GETTER, CALL_SETTER};
    











    static
    JSBool Dispatch(XPCCallContext& ccx, IDispatch * pDisp,
                    DISPID dispID, CallMode mode, XPCDispParams * params,
                    jsval* retval, XPCDispInterface::Member* member = nsnull,
                    XPCJSRuntime* rt = nsnull);
    




    static
    JSBool Invoke(XPCCallContext & ccx, CallMode mode);
    







    static
    HRESULT SecurityCheck(XPCCallContext & ccx, const CLSID & aCID,
                          IDispatch ** createdObject = nsnull);
    









    static
    HRESULT COMCreateInstance(XPCCallContext & ccx, BSTR className,
                              PRBool enforceSecurity, IDispatch ** result);
    






    static
    PRBool WrapIDispatch(IDispatch *pDispatch, XPCCallContext & ccx,
                         JSObject *obj, jsval *rval);
};

class XPCIDispatchExtension
{
public:
    


    static void InitStatics() { mIsEnabled = PR_TRUE; }

    



    static PRBool IsEnabled() { return mIsEnabled; }
    


    static void Enable() { mIsEnabled = PR_TRUE; }
    


    static void Disable() { mIsEnabled = PR_FALSE; }
    





    static JSBool Initialize(JSContext * aJSContext,
                             JSObject* aGlobalJSObj);
    









    static JSBool DefineProperty(XPCCallContext & ccx, 
                                 JSObject *obj, jsval idval,
                                 XPCWrappedNative* wrapperToReflectInterfaceNames,
                                 uintN propFlags, JSBool* resolved);
    







    static JSBool Enumerate(XPCCallContext& ccx, JSObject* obj, 
                            XPCWrappedNative * wrapper);
    







    static nsresult IDispatchQIWrappedJS(nsXPCWrappedJS * self, 
                                         void ** aInstancePtr);

private:
    static PRBool  mIsEnabled;
};




class XPCDispParams
{
public:
    



    XPCDispParams(PRUint32 args);
    


    ~XPCDispParams();
    



    void SetNamedPropID();
    




    VARIANT & GetParamRef(PRUint32 index);
    




    _variant_t GetParam(PRUint32 index) const;
    




    void * GetOutputBuffer(PRUint32 index);
    



    DISPPARAMS* GetDispParams() const { return &const_cast<XPCDispParams*>(this)->mDispParams; }
    



    uintN GetParamCount() const { return mDispParams.cArgs; }
    




    void InsertParam(_variant_t & var);
private:
    


    XPCDispParams(const XPCDispParams & other) {
        NS_ERROR("XPCDispParams can't be copied"); }
    


    XPCDispParams& operator =(const XPCDispParams&) {
        NS_ERROR("XPCDispParams can't be assigned"); }

    enum
    {
        DEFAULT_ARG_ARRAY_SIZE = 8,
        DEFAULT_REF_BUFFER_SIZE = 8 * sizeof(VARIANT)
    };
    static
    PRUint32 RefBufferSize(PRUint32 args) { return (args + 1) * sizeof(VARIANT); }

    DISPPARAMS  mDispParams;
    char*       mRefBuffer;
    VARIANT*    mDispParamsAllocated;
    char*       mRefBufferAllocated;
    


    char        mStackRefBuffer[DEFAULT_REF_BUFFER_SIZE];
    VARIANT     mStackArgs[DEFAULT_ARG_ARRAY_SIZE];
    DISPID      mPropID;
#ifdef DEBUG
    PRBool mInserted;
#endif
};





class nsDispatchSupport : public nsIDispatchSupport
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDISPATCHSUPPORT
    


    nsDispatchSupport();
    


    virtual ~nsDispatchSupport();
    



    static nsDispatchSupport* GetSingleton();
    


    static void FreeSingleton() { NS_IF_RELEASE(mInstance); }

private:
    static nsDispatchSupport* mInstance;
};




class XPCIDispatchClassInfo : public nsIClassInfo
{
public:
    



    static XPCIDispatchClassInfo* GetSingleton();
    


    static void FreeSingleton();
    NS_DECL_ISUPPORTS
    NS_DECL_NSICLASSINFO
private:
    


    XPCIDispatchClassInfo() {}
    virtual ~XPCIDispatchClassInfo() {}

    static XPCIDispatchClassInfo*  sInstance;
};

#include "XPCDispInlines.h"

#endif
