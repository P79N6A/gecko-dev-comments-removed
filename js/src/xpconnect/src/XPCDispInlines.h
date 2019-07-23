










































inline
PRBool nsXPConnect::IsIDispatchEnabled() 
{
    return XPCIDispatchExtension::IsEnabled();
}




inline
XPCDispInterface::Member::ParamInfo::ParamInfo(
    const ELEMDESC * paramInfo) : mParamInfo(paramInfo) 
{
}

inline
JSBool XPCDispInterface::Member::ParamInfo::InitializeOutputParam(
    void * varBuffer, VARIANT & var) const
{
    var.vt = GetType() | VT_BYREF;
    var.byref = varBuffer;
    return JS_TRUE;
}

inline
PRBool XPCDispInterface::Member::ParamInfo::IsFlagSet(
    unsigned short flag) const 
{
    return mParamInfo->paramdesc.wParamFlags & flag ? PR_TRUE : PR_FALSE; 
}

inline
PRBool XPCDispInterface::Member::ParamInfo::IsIn() const 
{
    return IsFlagSet(PARAMFLAG_FIN) || mParamInfo->paramdesc.wParamFlags == 0;
}

inline
PRBool XPCDispInterface::Member::ParamInfo::IsOut() const 
{
    return IsFlagSet(PARAMFLAG_FOUT);
}

inline
PRBool XPCDispInterface::Member::ParamInfo::IsOptional() const 
{
    return IsFlagSet(PARAMFLAG_FOPT);
}

inline
PRBool XPCDispInterface::Member::ParamInfo::IsRetVal() const 
{
    return IsFlagSet(PARAMFLAG_FRETVAL);
}


inline
VARTYPE XPCDispInterface::Member::ParamInfo::GetType() const 
{
    return mParamInfo->tdesc.vt == VT_PTR ? mParamInfo->tdesc.lptdesc->vt : mParamInfo->tdesc.vt;
}




inline
XPCDispInterface::Member::Member() : 
    mType(UNINITIALIZED), mFuncDesc(nsnull), mGetterFuncDesc(nsnull),
    mTypeInfo(nsnull)
{
}

inline
XPCDispInterface::Member::~Member() 
{
    if(mTypeInfo)
    {
        
        
        PRBool releaseGetter = mGetterFuncDesc != nsnull && mFuncDesc != mGetterFuncDesc;
        if(mFuncDesc) 
            mTypeInfo->ReleaseFuncDesc(mFuncDesc);
        if(releaseGetter)
            mTypeInfo->ReleaseFuncDesc(mGetterFuncDesc);
    }
}

inline
void* XPCDispInterface::Member::operator new(size_t, Member* p)
{
    return p;
}

inline
void XPCDispInterface::Member::MakeGetter() 
{
    NS_ASSERTION(!IsFunction(), "Can't be function and property"); 
    mType |= GET_PROPERTY; 
}

inline
void XPCDispInterface::Member::MakeSetter() 
{ 
    NS_ASSERTION(!IsFunction(), "Can't be function and property"); 
    mType |= SET_PROPERTY; 
}

inline
void XPCDispInterface::Member::ResetType() 
{
    mType = UNINITIALIZED;
}

inline
void XPCDispInterface::Member::SetFunction() 
{ 
    NS_ASSERTION(!IsProperty(), "Can't be function and property"); 
    mType = FUNCTION; 
}

inline
PRBool XPCDispInterface::Member::IsFlagSet(unsigned short flag) const 
{
    return mType & flag ? PR_TRUE : PR_FALSE; 
}

inline
PRBool XPCDispInterface::Member::IsSetter() const
{
    return IsFlagSet(SET_PROPERTY);
}

inline
PRBool XPCDispInterface::Member::IsGetter() const
{
    return IsFlagSet(GET_PROPERTY);
}

inline
PRBool XPCDispInterface::Member::IsProperty() const
{
    return IsSetter() || IsGetter(); 
}

inline
PRBool XPCDispInterface::Member::IsFunction() const
{
    return IsFlagSet(FUNCTION);
}

inline
PRBool XPCDispInterface::Member::IsParameterizedSetter() const
{
    return IsSetter() && GetParamCount() > 1;
}

inline
PRBool XPCDispInterface::Member::IsParameterizedGetter() const
{
    return IsGetter() && (GetParamCount(PR_TRUE) > 1 ||
        (GetParamCount(PR_TRUE) == 1 && !GetParamInfo(0, PR_TRUE).IsRetVal()));
}

inline
PRBool XPCDispInterface::Member::IsParameterizedProperty() const
{
    return IsParameterizedSetter() || IsParameterizedGetter();
}

inline
jsval XPCDispInterface::Member::GetName() const
{
    return mName;
}

inline
void XPCDispInterface::Member::SetName(jsval name)
{
    mName = name;
}

inline
PRUint32 XPCDispInterface::Member::GetDispID() const
{
    return mFuncDesc->memid;
}

inline
PRUint32 XPCDispInterface::Member::GetParamCount(PRBool getter) const
{
    return (getter && mGetterFuncDesc) ? mGetterFuncDesc->cParams : mFuncDesc->cParams;
}

inline
XPCDispInterface::Member::ParamInfo XPCDispInterface::Member::GetParamInfo(PRUint32 index, PRBool getter) const
{
    NS_ASSERTION(index < GetParamCount(getter), "Array bounds error");
    return ParamInfo(((getter && mGetterFuncDesc) ? mGetterFuncDesc->lprgelemdescParam : mFuncDesc->lprgelemdescParam) + index);
}

inline
void XPCDispInterface::Member::SetTypeInfo(DISPID dispID, 
                                                ITypeInfo* pTypeInfo, 
                                                FUNCDESC* funcdesc)
{
    mTypeInfo = pTypeInfo; 
    mFuncDesc = funcdesc;
}

inline
void XPCDispInterface::Member::SetGetterFuncDesc(FUNCDESC* funcdesc)
{
    mGetterFuncDesc = funcdesc;
}

inline
PRUint16 XPCDispInterface::Member::GetParamType(PRUint32 index) const 
{
    return mFuncDesc->lprgelemdescParam[index].paramdesc.wParamFlags; 
}

inline
void XPCDispInterface::Member::SetMemID(DISPID memID)
{
    mMemID = memID;
}

inline
DISPID XPCDispInterface::Member::GetMemID() const
{
    return mMemID;
}




inline
XPCDispInterface* XPCDispInterface::Allocator::Allocate()
{
    return Valid() ? new (Count()) XPCDispInterface(mCX, mTypeInfo, mIDispatchMembers) : nsnull;
}

inline
XPCDispInterface::Allocator::~Allocator()
{
    delete [] mMemIDs;
}

PRBool XPCDispInterface::Allocator::Valid() const
{
    return mMemIDs ? PR_TRUE : PR_FALSE;
}




inline
JSObject* XPCDispInterface::GetJSObject() const
{
    return mJSObject;
}

inline
void XPCDispInterface::SetJSObject(JSObject* jsobj) 
{
    mJSObject = jsobj;
}

inline
const XPCDispInterface::Member* XPCDispInterface::FindMember(jsval name) const
{
    
    const Member* member = mMembers + mMemberCount;
    while(member > mMembers)
    {
        --member;
        if(name == member->GetName())
        {
            return member;
        }
    }
    return nsnull;
}


inline
const XPCDispInterface::Member& XPCDispInterface::GetMember(PRUint32 index)
{ 
    NS_ASSERTION(index < mMemberCount, "invalid index");
    return mMembers[index]; 
}

inline
PRUint32 XPCDispInterface::GetMemberCount() const 
{
    return mMemberCount;
}

inline
void XPCDispInterface::operator delete(void * p) 
{
    PR_Free(p);
}

inline
XPCDispInterface::~XPCDispInterface()
{
    
    
    
    for(PRUint32 index = 1; index < GetMemberCount(); ++index)
    {
        mMembers[index].~Member();
    }
}

inline
XPCDispInterface::XPCDispInterface(JSContext* cx, ITypeInfo * pTypeInfo,
                                   PRUint32 members) : mJSObject(nsnull)
{
    InspectIDispatch(cx, pTypeInfo, members);
}

inline
void * XPCDispInterface::operator new (size_t, PRUint32 members) 
{
    
    if(!members)
        members = 1;
    
    return PR_Malloc(sizeof(XPCDispInterface) + sizeof(Member) * (members - 1));
}




inline
XPCDispNameArray::XPCDispNameArray() : mCount(0), mNames(0) 
{
}

inline
XPCDispNameArray::~XPCDispNameArray() 
{ 
    delete [] mNames;
}

inline
void XPCDispNameArray::SetSize(PRUint32 size) 
{
    NS_ASSERTION(mCount == 0, "SetSize called more than once");
    mCount = size;
    mNames = (size ? new nsString[size] : 0);
}

inline
PRUint32 XPCDispNameArray::GetSize() const 
{
    return mCount;
}

inline
void XPCDispNameArray::SetName(DISPID dispid, nsAString const & name) 
{
    NS_ASSERTION(dispid <= (PRInt32)mCount, "Array bounds error in XPCDispNameArray::SetName");
    mNames[dispid - 1] = name;
}

inline
const nsAString & XPCDispNameArray::GetName(DISPID dispid) const 
{
    NS_ASSERTION(dispid <= (PRInt32)mCount, "Array bounds error in XPCDispNameArray::Get");
    if(dispid > 0)
        return mNames[dispid - 1];
    return EmptyString();
}

inline
DISPID XPCDispNameArray::Find(const nsAString &target) const
{
    for(PRUint32 index = 0; index < mCount; ++index) 
    {
        if(mNames[index].Equals(target)) 
            return static_cast<DISPID>(index + 1);
    }
    return 0; 
}




inline
PRUint32 XPCDispIDArray::Length() const
{
    return mIDArray.Length();
}

inline
jsval XPCDispIDArray::Item(JSContext* cx, PRUint32 index) const
{
    jsval val;
    if(!JS_IdToValue(cx, mIDArray.ElementAt(index), &val))
        return JSVAL_NULL;
    return val;
}

inline
void XPCDispIDArray::Unmark()
{
    mMarked = JS_FALSE;
}

inline
JSBool XPCDispIDArray::IsMarked() const
{
    return mMarked;
}




inline
FUNCDESC* XPCDispTypeInfo::FuncDescArray::Get(PRUint32 index) 
{
    return &mArray[index];
}

inline
void XPCDispTypeInfo::FuncDescArray::Release(FUNCDESC *) 
{
}

inline
PRUint32 XPCDispTypeInfo::FuncDescArray::Length() const 
{
    return mArray.Length();
}

inline
const nsAString & XPCDispTypeInfo::GetNameForDispID(DISPID dispID)
{
    return mNameArray.GetName(dispID);
}




inline
PRBool XPCDispJSPropertyInfo::Valid() const 
{
    return mPropertyType != INVALID;
}

inline
PRUint32 XPCDispJSPropertyInfo::GetParamCount() const
{
    return IsSetter() ? 1 : mParamCount;
}

inline
PRUint32 XPCDispJSPropertyInfo::GetMemID() const
{
    return mMemID;
}

inline
INVOKEKIND XPCDispJSPropertyInfo::GetInvokeKind() const
{
    return IsSetter() ? INVOKE_PROPERTYPUT : 
        (IsProperty() ? INVOKE_PROPERTYGET : INVOKE_FUNC); 
}

inline
PRBool XPCDispJSPropertyInfo::IsProperty() const
{
    return PropertyType() == PROPERTY || PropertyType() == READONLY_PROPERTY;
}

inline
PRBool XPCDispJSPropertyInfo::IsReadOnly() const
{
    return PropertyType()== READONLY_PROPERTY;
}

inline
PRBool XPCDispJSPropertyInfo::IsSetter() const
{
    return (mPropertyType & SETTER_MODE) != 0;
}
inline
void XPCDispJSPropertyInfo::SetSetter()
{
    mPropertyType |= SETTER_MODE;
}

inline
const nsAString & XPCDispJSPropertyInfo::GetName() const
{
    return mName; 
}

inline
XPCDispJSPropertyInfo::property_type XPCDispJSPropertyInfo::PropertyType() const
{
    return static_cast<property_type>(mPropertyType & ~SETTER_MODE);
}




inline
const nsIID & XPCDispIID2nsIID(const IID & iid)
{
    NS_ASSERTION(sizeof(IID) == sizeof(nsIID), "IID is not the same size as nsIID");
    return reinterpret_cast<const nsIID &>(iid);
}

inline
const IID & XPCDispIID2IID(const nsIID & iid)
{
    NS_ASSERTION(sizeof(IID) == sizeof(nsIID), "IID is not the same size as nsIID");
    return reinterpret_cast<const IID &>(iid);
}

inline
const nsCID & XPCDispCLSID2nsCID(const CLSID & clsid)
{
    NS_ASSERTION(sizeof(CLSID) == sizeof(nsCID), "CLSID is not the same size as nsCID");
    return reinterpret_cast<const nsCID &>(clsid);
}

inline
const CLSID & XPCDispnsCID2CLSID(const nsCID & clsid)
{
    NS_ASSERTION(sizeof(CLSID) == sizeof(nsCID), "CLSID is not the same size as nsCID");
    return reinterpret_cast<const CLSID &>(clsid);
}




inline
void XPCDispParams::SetNamedPropID()
{
    mDispParams.rgdispidNamedArgs = &mPropID; 
    mDispParams.cNamedArgs = 1; 
}

inline
VARIANT & XPCDispParams::GetParamRef(PRUint32 index)
{
    NS_ASSERTION(index < mDispParams.cArgs, "XPCDispParams::GetParam bounds error");
    return mDispParams.rgvarg[mDispParams.cArgs - index - 1];
}

inline
_variant_t XPCDispParams::GetParam(PRUint32 index) const
{
    return const_cast<XPCDispParams*>(this)->GetParamRef(index);
}

inline
void * XPCDispParams::GetOutputBuffer(PRUint32 index)
{
    NS_ASSERTION(index < mDispParams.cArgs, "XPCDispParams::GetParam bounds error");
    return mRefBuffer + sizeof(VARIANT) * index;
}










inline
jschar * xpc_JSString2String(JSContext * cx, jsval val, PRUint32 * len = 0)
{
    JSString* str = JSVAL_IS_STRING(val) ? JSVAL_TO_STRING(val) : 
                                           JS_ValueToString(cx, val);
    if(str)
    {
        if(len)
            *len = JS_GetStringLength(str);
        return JS_GetStringChars(str);
    }
    if(len)
        *len = 0;
    return nsnull;
}








inline
PRUnichar* xpc_JSString2PRUnichar(XPCCallContext& ccx, jsval val,
                                  size_t* length = nsnull)
{
    JSString* str = JS_ValueToString(ccx, val);
    if(!str)
        return nsnull;
    if(length)
        *length = JS_GetStringLength(str);
    return reinterpret_cast<PRUnichar*>(JS_GetStringChars(str));
}

