




































#include "nsJavaXPTCStub.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "prmem.h"
#include "nsIInterfaceInfoManager.h"
#include "nsString.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"


nsJavaXPTCStub::nsJavaXPTCStub(jobject aJavaObject, nsIInterfaceInfo *aIInfo,
                               nsresult *rv)
  : mJavaStrongRef(nsnull)
  , mIInfo(aIInfo)
  , mMaster(nsnull)
  , mWeakRefCnt(0)
{
  const nsIID *iid = nsnull;
  aIInfo->GetIIDShared(&iid);
  NS_ASSERTION(iid, "GetIIDShared must not fail!");

  *rv = InitStub(*iid);
  if (NS_FAILED(*rv))
    return;

  JNIEnv* env = GetJNIEnv();
  jobject weakref = env->NewObject(weakReferenceClass,
                                   weakReferenceConstructorMID, aJavaObject);
  mJavaWeakRef = env->NewGlobalRef(weakref);
  mJavaRefHashCode = env->CallStaticIntMethod(systemClass, hashCodeMID,
                                              aJavaObject);

#ifdef DEBUG_JAVAXPCOM
  char* iid_str = iid->ToString();
  LOG(("+ nsJavaXPTCStub (Java=%08x | XPCOM=%08x | IID=%s)\n",
      (PRUint32) mJavaRefHashCode, (PRUint32) this, iid_str));
  PR_Free(iid_str);
#endif
}

nsJavaXPTCStub::~nsJavaXPTCStub()
{
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::AddRefInternal()
{
  
  
  if (mRefCnt == 0) {
    JNIEnv* env = GetJNIEnv();
    jobject referent = env->CallObjectMethod(mJavaWeakRef, getReferentMID);
    if (!env->IsSameObject(referent, NULL)) {
      mJavaStrongRef = env->NewGlobalRef(referent);
    }
    NS_ASSERTION(mJavaStrongRef != nsnull, "Failed to acquire strong ref");
  }

  
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
  NS_ASSERT_OWNINGTHREAD(nsJavaXPTCStub);
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsJavaXPTCStub", sizeof(*this));
  return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::AddRef()
{
#ifdef DEBUG_JAVAXPCOM_REFCNT
  nsIID* iid;
  mIInfo->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  int refcnt = PRInt32(mMaster ? mMaster->mRefCnt : mRefCnt) + 1;
  LOG(("= nsJavaXPTCStub::AddRef (XPCOM=%08x | refcnt = %d | IID=%s)\n",
       (int) this, refcnt, iid_str));
  PR_Free(iid_str);
  nsMemory::Free(iid);
#endif

  nsJavaXPTCStub* master = mMaster ? mMaster : this;
  return master->AddRefInternal();
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::ReleaseInternal()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(nsJavaXPTCStub);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsJavaXPTCStub");
  if (mRefCnt == 0) {
    
    DeleteStrongRef();

    
    if (mWeakRefCnt == 0) {
      mRefCnt = 1; 
      Destroy();
      delete this;
    }
    return 0;
  }
  return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::Release()
{
#ifdef DEBUG_JAVAXPCOM_REFCNT
  nsIID* iid;
  mIInfo->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  int refcnt = PRInt32(mMaster ? mMaster->mRefCnt : mRefCnt) - 1;
  LOG(("= nsJavaXPTCStub::Release (XPCOM=%08x | refcnt = %d | IID=%s)\n",
       (int) this, refcnt, iid_str));
  PR_Free(iid_str);
  nsMemory::Free(iid);
#endif

  nsJavaXPTCStub* master = mMaster ? mMaster : this;
  return master->ReleaseInternal();
}

void
nsJavaXPTCStub::Destroy()
{
  JNIEnv* env = GetJNIEnv();

#ifdef DEBUG_JAVAXPCOM
  nsIID* iid;
  mIInfo->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  LOG(("- nsJavaXPTCStub (Java=%08x | XPCOM=%08x | IID=%s)\n",
      (PRUint32) mJavaRefHashCode, (PRUint32) this, iid_str));
  PR_Free(iid_str);
  nsMemory::Free(iid);
#endif

  if (!mMaster) {
    
    mChildren.Clear();

    
    
    
    if (gJavaXPCOMInitialized) {
      gJavaToXPTCStubMap->Remove(mJavaRefHashCode);
    }
  }

  env->CallVoidMethod(mJavaWeakRef, clearReferentMID);
  env->DeleteGlobalRef(mJavaWeakRef);
}

void
nsJavaXPTCStub::ReleaseWeakRef()
{
  
  if (mMaster)
    mMaster->ReleaseWeakRef();

  --mWeakRefCnt;

  
  
  if (mWeakRefCnt == 0 && mRefCnt == 0) {
    NS_ASSERT_OWNINGTHREAD(nsJavaXPTCStub);
    mRefCnt = 1; 
    Destroy();
    delete this;
  }
}

void
nsJavaXPTCStub::DeleteStrongRef()
{
  if (mJavaStrongRef == nsnull)
    return;

  GetJNIEnv()->DeleteGlobalRef(mJavaStrongRef);
  mJavaStrongRef = nsnull;
}

NS_IMETHODIMP
nsJavaXPTCStub::QueryInterface(const nsID &aIID, void **aInstancePtr)
{
  nsresult rv;

  LOG(("JavaStub::QueryInterface()\n"));
  *aInstancePtr = nsnull;
  nsJavaXPTCStub *master = mMaster ? mMaster : this;

  
  if (aIID.Equals(NS_GET_IID(nsJavaXPTCStub)))
  {
    *aInstancePtr = master;
    NS_ADDREF(this);
    return NS_OK;
  }

  
  if (aIID.Equals(NS_GET_IID(nsISupports)))
  {
    *aInstancePtr = master->mXPTCStub;
    NS_ADDREF(master);
    return NS_OK;
  }

  
  if (aIID.Equals(NS_GET_IID(nsISupportsWeakReference)))
  {
    *aInstancePtr = static_cast<nsISupportsWeakReference*>(master);
    NS_ADDREF(master);
    return NS_OK;
  }

  
  nsJavaXPTCStub *stub = master->FindStubSupportingIID(aIID);
  if (stub)
  {
    *aInstancePtr = stub->mXPTCStub;
    NS_ADDREF(stub);
    return NS_OK;
  }

  JNIEnv* env = GetJNIEnv();

  
  LOG(("\tCalling Java object queryInterface\n"));
  jobject javaObject = env->CallObjectMethod(mJavaWeakRef, getReferentMID);

  jmethodID qiMID = 0;
  jclass clazz = env->GetObjectClass(javaObject);
  if (clazz) {
    char* sig = "(Ljava/lang/String;)Lorg/mozilla/interfaces/nsISupports;";
    qiMID = env->GetMethodID(clazz, "queryInterface", sig);
    NS_ASSERTION(qiMID, "Failed to get queryInterface method ID");
  }

  if (qiMID == 0) {
    env->ExceptionClear();
    return NS_NOINTERFACE;
  }

  
  jstring iid_jstr = nsnull;
  char* iid_str = aIID.ToString();
  if (iid_str) {
    iid_jstr = env->NewStringUTF(iid_str);
  }
  if (!iid_str || !iid_jstr) {
    env->ExceptionClear();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PR_Free(iid_str);

  
  jobject obj = env->CallObjectMethod(javaObject, qiMID, iid_jstr);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return NS_ERROR_FAILURE;
  }
  if (!obj)
    return NS_NOINTERFACE;

  
  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceInfo> iinfo;
  rv = iim->GetInfoForIID(&aIID, getter_AddRefs(iinfo));
  if (NS_FAILED(rv))
    return rv;

  stub = new nsJavaXPTCStub(obj, iinfo, &rv);
  if (!stub)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(rv)) {
    delete stub;
    return rv;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  stub->mMaster = master;
  master->mChildren.AppendElement(stub);

  *aInstancePtr = stub->mXPTCStub;
  NS_ADDREF(stub);
  return NS_OK;
}

PRBool
nsJavaXPTCStub::SupportsIID(const nsID &iid)
{
  PRBool match;
  nsCOMPtr<nsIInterfaceInfo> iter = mIInfo;
  do
  {
    if (NS_SUCCEEDED(iter->IsIID(&iid, &match)) && match)
      return PR_TRUE;

    nsCOMPtr<nsIInterfaceInfo> parent;
    iter->GetParent(getter_AddRefs(parent));
    iter = parent;
  }
  while (iter != nsnull);

  return PR_FALSE;
}

nsJavaXPTCStub *
nsJavaXPTCStub::FindStubSupportingIID(const nsID &iid)
{
  NS_ASSERTION(mMaster == nsnull, "this is not a master stub");

  if (SupportsIID(iid))
    return this;

  for (PRUint32 i = 0; i < mChildren.Length(); i++)
  {
    nsJavaXPTCStub *child = mChildren[i];
    if (child->SupportsIID(iid))
      return child;
  }
  return nsnull;
}

NS_IMETHODIMP
nsJavaXPTCStub::CallMethod(PRUint16 aMethodIndex,
                           const XPTMethodDescriptor *aMethodInfo,
                           nsXPTCMiniVariant *aParams)
{
#ifdef DEBUG_JAVAXPCOM
  const char* ifaceName;
  mIInfo->GetNameShared(&ifaceName);
  LOG(("---> (Java) %s::%s()\n", ifaceName, aMethodInfo->name));
#endif

  nsresult rv = NS_OK;
  JNIEnv* env = GetJNIEnv();
  jobject javaObject = env->CallObjectMethod(mJavaWeakRef, getReferentMID);

  nsCAutoString methodSig("(");

  
  PRUint8 paramCount = aMethodInfo->num_args;
  jvalue* java_params = nsnull;
  const nsXPTParamInfo* retvalInfo = nsnull;
  if (paramCount) {
    java_params = new jvalue[paramCount];
    if (!java_params)
      return NS_ERROR_OUT_OF_MEMORY;

    for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
    {
      const nsXPTParamInfo &paramInfo = aMethodInfo->params[i];
      if (!paramInfo.IsRetval()) {
        rv = SetupJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                             aParams[i], java_params[i], methodSig);
      } else {
        retvalInfo = &paramInfo;
      }
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetupJavaParams failed");
  }

  
  if (NS_SUCCEEDED(rv)) {
    methodSig.Append(')');
    if (retvalInfo) {
      nsCAutoString retvalSig;
      rv = GetRetvalSig(retvalInfo, aMethodInfo, aMethodIndex, aParams,
                        retvalSig);
      methodSig.Append(retvalSig);
    } else {
      methodSig.Append('V');
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetRetvalSig failed");
  }

  
  jmethodID mid = nsnull;
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString methodName;
    if (XPT_MD_IS_GETTER(aMethodInfo->flags) ||
        XPT_MD_IS_SETTER(aMethodInfo->flags)) {
      if (XPT_MD_IS_GETTER(aMethodInfo->flags))
        methodName.AppendLiteral("get");
      else
        methodName.AppendLiteral("set");
      methodName.AppendASCII(aMethodInfo->name);
      methodName.SetCharAt(toupper(methodName[3]), 3);
    } else {
      methodName.AppendASCII(aMethodInfo->name);
      methodName.SetCharAt(tolower(methodName[0]), 0);
    }
    
    if (gJavaKeywords->GetEntry(methodName.get())) {
      methodName.Insert('_', 0);
    }

    jclass clazz = env->GetObjectClass(javaObject);
    if (clazz)
      mid = env->GetMethodID(clazz, methodName.get(), methodSig.get());
    NS_ASSERTION(mid, "Failed to get requested method for Java object");
    if (!mid)
      rv = NS_ERROR_FAILURE;
  }

  
  jvalue retval;
  if (NS_SUCCEEDED(rv)) {
    if (!retvalInfo) {
      env->CallVoidMethodA(javaObject, mid, java_params);
    } else {
      switch (retvalInfo->GetType().TagPart())
      {
        case nsXPTType::T_I8:
          retval.b = env->CallByteMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_I16:
        case nsXPTType::T_U8:
          retval.s = env->CallShortMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_I32:
        case nsXPTType::T_U16:
          retval.i = env->CallIntMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_I64:
        case nsXPTType::T_U32:
          retval.j = env->CallLongMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_FLOAT:
          retval.f = env->CallFloatMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_U64:
        case nsXPTType::T_DOUBLE:
          retval.d = env->CallDoubleMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_BOOL:
          retval.z = env->CallBooleanMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_CHAR:
        case nsXPTType::T_WCHAR:
          retval.c = env->CallCharMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_CHAR_STR:
        case nsXPTType::T_WCHAR_STR:
        case nsXPTType::T_IID:
        case nsXPTType::T_ASTRING:
        case nsXPTType::T_DOMSTRING:
        case nsXPTType::T_UTF8STRING:
        case nsXPTType::T_CSTRING:
        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
          retval.l = env->CallObjectMethodA(javaObject, mid, java_params);
          break;

        case nsXPTType::T_VOID:
          retval.j = env->CallLongMethodA(javaObject, mid, java_params);
          break;

        default:
          NS_WARNING("Unhandled retval type");
          break;
      }
    }

    
    jthrowable exp = env->ExceptionOccurred();
    if (exp) {
      
      
      
      if (env->IsInstanceOf(exp, xpcomExceptionClass)) {
        jfieldID fid;
        fid = env->GetFieldID(xpcomExceptionClass, "errorcode", "J");
        if (fid) {
          rv = env->GetLongField(exp, fid);
        } else {
          rv = NS_ERROR_FAILURE;
        }
        NS_ASSERTION(fid, "Couldn't get 'errorcode' field of XPCOMException");
      } else {
        rv = NS_ERROR_FAILURE;
      }
    }
  }

  
  if (NS_SUCCEEDED(rv)) {
    for (PRUint8 i = 0; i < paramCount; i++)
    {
      const nsXPTParamInfo &paramInfo = aMethodInfo->params[i];
      if (paramInfo.IsIn() && !paramInfo.IsOut() && !paramInfo.IsDipper()) 
        continue;

      
      if (aParams[i].val.p == nsnull)
        continue;

      if (!paramInfo.IsRetval()) {
        rv = FinalizeJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                                aParams[i], java_params[i]);
      } else {
        rv = FinalizeJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                                aParams[i], retval);
      }
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "FinalizeJavaParams/SetXPCOMRetval failed");
  }

  if (java_params)
    delete [] java_params;

#ifdef DEBUG
  if (env->ExceptionCheck())
    env->ExceptionDescribe();
#endif
  env->ExceptionClear();

  LOG(("<--- (Java) %s::%s()\n", ifaceName, aMethodInfo->name));
  return rv;
}




nsresult
nsJavaXPTCStub::SetupJavaParams(const nsXPTParamInfo &aParamInfo,
                const XPTMethodDescriptor* aMethodInfo,
                PRUint16 aMethodIndex,
                nsXPTCMiniVariant* aDispatchParams,
                nsXPTCMiniVariant &aVariant, jvalue &aJValue,
                nsACString &aMethodSig)
{
  nsresult rv = NS_OK;
  JNIEnv* env = GetJNIEnv();
  const nsXPTType &type = aParamInfo.GetType();

  PRUint8 tag = type.TagPart();
  switch (tag)
  {
    case nsXPTType::T_I8:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.b = aVariant.val.i8;
        aMethodSig.Append('B');
      } else {  
        if (aVariant.val.p) {
          jbyteArray array = env->NewByteArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetByteArrayRegion(array, 0, 1, (jbyte*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[B");
      }
    }
    break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.s = (tag == nsXPTType::T_I16) ? aVariant.val.i16 :
                                                aVariant.val.u8;
        aMethodSig.Append('S');
      } else {  
        if (aVariant.val.p) {
          jshortArray array = env->NewShortArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetShortArrayRegion(array, 0, 1, (jshort*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[S");
      }
    }
    break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.i = (tag == nsXPTType::T_I32) ? aVariant.val.i32 :
                                                aVariant.val.u16;
        aMethodSig.Append('I');
      } else {  
        if (aVariant.val.p) {
          jintArray array = env->NewIntArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetIntArrayRegion(array, 0, 1, (jint*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[I");
      }
    }
    break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.j = (tag == nsXPTType::T_I64) ? aVariant.val.i64 :
                                                aVariant.val.u32;
        aMethodSig.Append('J');
      } else {  
        if (aVariant.val.p) {
          jlongArray array = env->NewLongArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetLongArrayRegion(array, 0, 1, (jlong*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[J");
      }
    }
    break;

    case nsXPTType::T_FLOAT:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.f = aVariant.val.f;
        aMethodSig.Append('F');
      } else {  
        if (aVariant.val.p) {
          jfloatArray array = env->NewFloatArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetFloatArrayRegion(array, 0, 1, (jfloat*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[F");
      }
    }
    break;

    
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.d = (tag == nsXPTType::T_DOUBLE) ? aVariant.val.d :
                                                   aVariant.val.u64;
        aMethodSig.Append('D');
      } else {  
        if (aVariant.val.p) {
          jdoubleArray array = env->NewDoubleArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetDoubleArrayRegion(array, 0, 1, (jdouble*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[D");
      }
    }
    break;

    case nsXPTType::T_BOOL:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.z = aVariant.val.b;
        aMethodSig.Append('Z');
      } else {  
        if (aVariant.val.p) {
          jbooleanArray array = env->NewBooleanArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetBooleanArrayRegion(array, 0, 1, (jboolean*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Z");
      }
    }
    break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (!aParamInfo.IsOut()) {  
        if (tag == nsXPTType::T_CHAR)
          aJValue.c = aVariant.val.c;
        else
          aJValue.c = aVariant.val.wc;
        aMethodSig.Append('C');
      } else {  
        if (aVariant.val.p) {
          jcharArray array = env->NewCharArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetCharArrayRegion(array, 0, 1, (jchar*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[C");
      }
    }
    break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      void* ptr = nsnull;
      if (!aParamInfo.IsOut()) {  
        ptr = aVariant.val.p;
      } else if (aVariant.val.p) {  
        void** variant = static_cast<void**>(aVariant.val.p);
        ptr = *variant;
      }

      jobject str;
      if (ptr) {
        if (tag == nsXPTType::T_CHAR_STR) {
          str = env->NewStringUTF((const char*) ptr);
        } else {
          const PRUnichar* buf = (const PRUnichar*) ptr;
          str = env->NewString(buf, nsCRT::strlen(buf));
        }
        if (!str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      } else {
        str = nsnull;
      }

      if (!aParamInfo.IsOut()) {  
        aJValue.l = str;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  
        if (aVariant.val.p) {
          aJValue.l = env->NewObjectArray(1, stringClass, str);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    case nsXPTType::T_IID:
    {
      nsID* iid = nsnull;
      if (!aParamInfo.IsOut()) {  
        iid = static_cast<nsID*>(aVariant.val.p);
      } else if (aVariant.val.p) {  
        nsID** variant = static_cast<nsID**>(aVariant.val.p);
        iid = *variant;
      }

      jobject str = nsnull;
      if (iid) {
        char iid_str[NSID_LENGTH];
        iid->ToProvidedString(iid_str);
        str = env->NewStringUTF(iid_str);
        if (!str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      }

      if (!aParamInfo.IsOut()) {  
        aJValue.l = str;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  
        if (aVariant.val.p) {
          aJValue.l = env->NewObjectArray(1, stringClass, str);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      nsISupports* xpcom_obj = nsnull;
      if (!aParamInfo.IsOut()) {  
        xpcom_obj = static_cast<nsISupports*>(aVariant.val.p);
      } else if (aVariant.val.p) {  
        nsISupports** variant = static_cast<nsISupports**>(aVariant.val.p);
        xpcom_obj = *variant;
      }

      nsID iid;
      rv = GetIIDForMethodParam(mIInfo, aMethodInfo, aParamInfo,
                                aParamInfo.GetType().TagPart(), aMethodIndex,
                                aDispatchParams, PR_FALSE, iid);
      if (NS_FAILED(rv))
        break;

      
      char* iface_name = nsnull;
      nsCOMPtr<nsIInterfaceInfoManager>
        iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv));
      if (NS_FAILED(rv))
        break;

      rv = iim->GetNameForIID(&iid, &iface_name);
      if (NS_FAILED(rv) || !iface_name)
        break;

      jobject java_stub = nsnull;
      if (xpcom_obj) {
        
        jobject objLoader = env->CallObjectMethod(mJavaWeakRef, getReferentMID);
        rv = NativeInterfaceToJavaObject(env, xpcom_obj, iid, objLoader,
                                         &java_stub);
        if (NS_FAILED(rv))
          break;
      }

      if (!aParamInfo.IsOut()) {  
        aJValue.l = java_stub;
      } else {  
        if (aVariant.val.p) {
          aJValue.l = env->NewObjectArray(1, nsISupportsClass, java_stub);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.Append('[');
      }

      if (tag != nsXPTType::T_INTERFACE_IS) {
        aMethodSig.AppendLiteral("Lorg/mozilla/interfaces/");
        aMethodSig.AppendASCII(iface_name);
        aMethodSig.Append(';');
      } else {
        aMethodSig.AppendLiteral("Lorg/mozilla/interfaces/nsISupports;");
      }

      nsMemory::Free(iface_name);
    }
    break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      
      
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsString* str = static_cast<nsString*>(aVariant.val.p);
      if (!str) {
        rv = NS_ERROR_FAILURE;
        break;
      }

      jstring jstr = nsnull;
      if (!str->IsVoid()) {
        jstr = env->NewString(str->get(), str->Length());
        if (!jstr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      }

      aJValue.l = jstr;
      aMethodSig.AppendLiteral("Ljava/lang/String;");
    }
    break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      
      
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsCString* str = static_cast<nsCString*>(aVariant.val.p);
      if (!str) {
        rv = NS_ERROR_FAILURE;
        break;
      }

      jstring jstr = nsnull;
      if (!str->IsVoid()) {
        jstr = env->NewStringUTF(str->get());
        if (!jstr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      }

      aJValue.l = jstr;
      aMethodSig.AppendLiteral("Ljava/lang/String;");
    }
    break;

    
    case nsXPTType::T_VOID:
    {
      if (!aParamInfo.IsOut()) {  
        aJValue.j = reinterpret_cast<jlong>(aVariant.val.p);
        aMethodSig.Append('J');
      } else {  
        if (aVariant.val.p) {
          jlongArray array = env->NewLongArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          env->SetLongArrayRegion(array, 0, 1, (jlong*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[J");
      }
    }
    break;

    case nsXPTType::T_ARRAY:
      NS_WARNING("array types are not yet supported");
      return NS_ERROR_NOT_IMPLEMENTED;
      break;

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

nsresult
nsJavaXPTCStub::GetRetvalSig(const nsXPTParamInfo* aParamInfo,
                             const XPTMethodDescriptor* aMethodInfo,
                             PRUint16 aMethodIndex,
                             nsXPTCMiniVariant* aDispatchParams,
                             nsACString &aRetvalSig)
{
  PRUint8 type = aParamInfo->GetType().TagPart();
  switch (type)
  {
    case nsXPTType::T_I8:
      aRetvalSig.Append('B');
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
      aRetvalSig.Append('S');
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
      aRetvalSig.Append('I');
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
      aRetvalSig.Append('J');
      break;

    case nsXPTType::T_FLOAT:
      aRetvalSig.Append('F');
      break;

    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
      aRetvalSig.Append('D');
      break;

    case nsXPTType::T_BOOL:
      aRetvalSig.Append('Z');
      break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
      aRetvalSig.Append('C');
      break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    case nsXPTType::T_IID:
    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      aRetvalSig.AppendLiteral("Ljava/lang/String;");
      break;

    case nsXPTType::T_INTERFACE:
    {
      nsID iid;
      nsresult rv = GetIIDForMethodParam(mIInfo, aMethodInfo, *aParamInfo, type,
                                         aMethodIndex, aDispatchParams,
                                         PR_FALSE, iid);
      if (NS_FAILED(rv))
        break;

      
      char* iface_name = nsnull;
      nsCOMPtr<nsIInterfaceInfoManager>
        iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv));
      if (NS_FAILED(rv))
        break;

      rv = iim->GetNameForIID(&iid, &iface_name);
      if (NS_FAILED(rv) || !iface_name)
        break;

      aRetvalSig.AppendLiteral("Lorg/mozilla/interfaces/");
      aRetvalSig.AppendASCII(iface_name);
      aRetvalSig.Append(';');
      nsMemory::Free(iface_name);
      break;
    }

    case nsXPTType::T_INTERFACE_IS:
      aRetvalSig.AppendLiteral("Lorg/mozilla/interfaces/nsISupports;");
      break;

    case nsXPTType::T_VOID:
      aRetvalSig.Append('J');
      break;

    case nsXPTType::T_ARRAY:
      NS_WARNING("array types are not yet supported");
      return NS_ERROR_NOT_IMPLEMENTED;
      break;

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}




nsresult
nsJavaXPTCStub::FinalizeJavaParams(const nsXPTParamInfo &aParamInfo,
                                 const XPTMethodDescriptor *aMethodInfo,
                                 PRUint16 aMethodIndex,
                                 nsXPTCMiniVariant* aDispatchParams,
                                 nsXPTCMiniVariant &aVariant, jvalue &aJValue)
{
  nsresult rv = NS_OK;
  JNIEnv* env = GetJNIEnv();
  const nsXPTType &type = aParamInfo.GetType();

  PRUint8 tag = type.TagPart();
  switch (tag)
  {
    case nsXPTType::T_I8:
    {
      jbyte value;
      if (aParamInfo.IsRetval()) {  
        value = aJValue.b;
      } else if (aJValue.l) {  
        env->GetByteArrayRegion((jbyteArray) aJValue.l, 0, 1, &value);
      }
      if (aVariant.val.p)
        *((PRInt8 *) aVariant.val.p) = value;
    }
    break;

    case nsXPTType::T_U8:
    case nsXPTType::T_I16:
    {
      jshort value = 0;
      if (aParamInfo.IsRetval()) {  
        value = aJValue.s;
      } else if (aJValue.l) {  
        env->GetShortArrayRegion((jshortArray) aJValue.l, 0, 1, &value);
      }

      if (aVariant.val.p) {
        if (tag == nsXPTType::T_U8)
          *((PRUint8 *) aVariant.val.p) = value;
        else
          *((PRInt16 *) aVariant.val.p) = value;
      }
    }
    break;

    case nsXPTType::T_U16:
    case nsXPTType::T_I32:
    {
      jint value = 0;
      if (aParamInfo.IsRetval()) {  
        value = aJValue.i;
      } else if (aJValue.l) {  
        env->GetIntArrayRegion((jintArray) aJValue.l, 0, 1, &value);
      }

      if (aVariant.val.p) {
        if (tag == nsXPTType::T_U16)
          *((PRUint16 *) aVariant.val.p) = value;
        else
          *((PRInt32 *) aVariant.val.p) = value;
      }
    }
    break;

    case nsXPTType::T_U32:
    case nsXPTType::T_I64:
    {
      jlong value = 0;
      if (aParamInfo.IsRetval()) {  
        value = aJValue.j;
      } else if (aJValue.l) {  
        env->GetLongArrayRegion((jlongArray) aJValue.l, 0, 1, &value);
      }

      if (aVariant.val.p) {
        if (tag == nsXPTType::T_U32)
          *((PRUint32 *) aVariant.val.p) = value;
        else
          *((PRInt64 *) aVariant.val.p) = value;
      }
    }
    break;

    case nsXPTType::T_FLOAT:
    {
      if (aParamInfo.IsRetval()) {  
        *((float *) aVariant.val.p) = aJValue.f;
      } else if (aJValue.l) {  
        env->GetFloatArrayRegion((jfloatArray) aJValue.l, 0, 1,
                                 (jfloat*) aVariant.val.p);
      }
    }
    break;

    
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
    {
      jdouble value = 0;
      if (aParamInfo.IsRetval()) {  
        value = aJValue.d;
      } else if (aJValue.l) {  
        env->GetDoubleArrayRegion((jdoubleArray) aJValue.l, 0, 1, &value);
      }

      if (aVariant.val.p) {
        if (tag == nsXPTType::T_DOUBLE)
          *((double *) aVariant.val.p) = value;
        else
          *((PRUint64 *) aVariant.val.p) = static_cast<PRUint64>(value);
      }
    }
    break;

    case nsXPTType::T_BOOL:
    {
      if (aParamInfo.IsRetval()) {  
        *((PRBool *) aVariant.val.p) = aJValue.z;
      } else if (aJValue.l) {  
        env->GetBooleanArrayRegion((jbooleanArray) aJValue.l, 0, 1,
                                   (jboolean*) aVariant.val.p);
      }
    }
    break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (aParamInfo.IsRetval()) {  
        if (type.TagPart() == nsXPTType::T_CHAR)
          *((char *) aVariant.val.p) = aJValue.c;
        else
          *((PRUnichar *) aVariant.val.p) = aJValue.c;
      } else if (aJValue.l) {  
        jchar* array = env->GetCharArrayElements((jcharArray) aJValue.l,
                                                 nsnull);
        if (!array) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        if (type.TagPart() == nsXPTType::T_CHAR)
          *((char *) aVariant.val.p) = array[0];
        else
          *((PRUnichar *) aVariant.val.p) = array[0];

        env->ReleaseCharArrayElements((jcharArray) aJValue.l, array, JNI_ABORT);
      }
    }
    break;

    case nsXPTType::T_CHAR_STR:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  
        str = (jstring) aJValue.l;
      } else {  
        str = (jstring) env->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      char** variant = static_cast<char**>(aVariant.val.p);
      if (str) {
        
        const char* char_ptr = env->GetStringUTFChars(str, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        
        
        if (aParamInfo.IsRetval() ||
            *variant == nsnull || strcmp(*variant, char_ptr) != 0)
        {
          if (!aParamInfo.IsRetval() && *variant)
            PR_Free(*variant);

          *variant = strdup(char_ptr);
          if (*variant == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            
          }
        }

        
        env->ReleaseStringUTFChars(str, char_ptr);
      } else {
        
        
        if (*variant && !aParamInfo.IsRetval()) {
          PR_Free(*variant);
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_WCHAR_STR:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  
        str = (jstring) aJValue.l;
      } else {  
        str = (jstring) env->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      PRUnichar** variant = static_cast<PRUnichar**>(aVariant.val.p);
      if (str) {
        
        const jchar* wchar_ptr = env->GetStringChars(str, nsnull);
        if (!wchar_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        
        
        if (aParamInfo.IsRetval() ||
            *variant == nsnull || nsCRT::strcmp(*variant, wchar_ptr) != 0)
        {
          if (!aParamInfo.IsRetval() && *variant)
            PR_Free(*variant);

          PRUint32 length = nsCRT::strlen(wchar_ptr);
          *variant = (PRUnichar*) PR_Malloc((length + 1) * sizeof(PRUnichar));
          if (*variant) {
            memcpy(*variant, wchar_ptr, length * sizeof(PRUnichar));
            (*variant)[length] = 0;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            
          }
        }

        
        env->ReleaseStringChars(str, wchar_ptr);
      } else {
        
        
        if (*variant && !aParamInfo.IsRetval()) {
          PR_Free(*variant);
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_IID:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  
        str = (jstring) aJValue.l;
      } else {  
        str = (jstring) env->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      nsID** variant = static_cast<nsID**>(aVariant.val.p);
      if (str) {
        
        const char* char_ptr = env->GetStringUTFChars(str, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        if (!aParamInfo.IsRetval() && *variant) {
          
          nsID* oldIID = *variant;
          oldIID->Parse(char_ptr);
        } else {
          
          
          nsID* newIID = new nsID;
          if (newIID) {
            newIID->Parse(char_ptr);
            *variant = newIID;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            
          }
        }

        
        env->ReleaseStringUTFChars(str, char_ptr);
      } else {
        
        
        if (*variant && !aParamInfo.IsRetval()) {
          delete *variant;
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      jobject java_obj = nsnull;
      if (aParamInfo.IsRetval()) {  
        java_obj = aJValue.l;
      } else if (aJValue.l) {  
        java_obj = env->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      void* xpcom_obj = nsnull;
      if (java_obj) {
        
        nsID iid;
        rv = GetIIDForMethodParam(mIInfo, aMethodInfo, aParamInfo,
                                  aParamInfo.GetType().TagPart(), aMethodIndex,
                                  aDispatchParams, PR_FALSE, iid);
        if (NS_FAILED(rv))
          break;

        
        
        
        PRBool isWeakRef;
        if (iid.Equals(NS_GET_IID(nsIWeakReference))) {
          isWeakRef = PR_TRUE;
          iid = NS_GET_IID(nsISupports);
        } else {
          isWeakRef = PR_FALSE;
        }

        rv = JavaObjectToNativeInterface(env, java_obj, iid, &xpcom_obj);
        if (NS_FAILED(rv))
          break;
        rv = ((nsISupports*) xpcom_obj)->QueryInterface(iid, &xpcom_obj);
        if (NS_FAILED(rv))
          break;

        
        
        if (isWeakRef) {
          nsISupports* isupports = (nsISupports*) xpcom_obj;
          nsCOMPtr<nsISupportsWeakReference> supportsweak =
                  do_QueryInterface(isupports);
          if (supportsweak) {
            nsWeakPtr weakref;
            supportsweak->GetWeakReference(getter_AddRefs(weakref));
            NS_RELEASE(isupports);
            xpcom_obj = weakref;
            NS_ADDREF((nsISupports*) xpcom_obj);
          } else {
            xpcom_obj = nsnull;
          }
        }
      }

      
      
      nsISupports** variant = static_cast<nsISupports**>(aVariant.val.p);
      if (aParamInfo.IsIn() && *variant) {
        nsCOMPtr<nsISupports> in = do_QueryInterface(*variant);
        nsCOMPtr<nsISupports> out = do_QueryInterface((nsISupports*) xpcom_obj);
        if (in != out) {
          NS_RELEASE(*variant);
        }
      }

      *(static_cast<void**>(aVariant.val.p)) = xpcom_obj;
    }
    break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsDipper(), "string argument is not dipper");
      if (!aParamInfo.IsDipper()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = (jstring) aJValue.l;
      nsString* variant = static_cast<nsString*>(aVariant.val.p);
      
      if (jstr) {
        
        const jchar* wchar_ptr = env->GetStringChars(jstr, nsnull);
        if (!wchar_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        variant->Assign(wchar_ptr);

        
        env->ReleaseStringChars(jstr, wchar_ptr);
      } else {
        variant->SetIsVoid(PR_TRUE);
      }
    }
    break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsDipper(), "string argument is not dipper");
      if (!aParamInfo.IsDipper()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = (jstring) aJValue.l;
      nsCString* variant = static_cast<nsCString*>(aVariant.val.p);
      
      if (jstr) {
        
        const char* char_ptr = env->GetStringUTFChars(jstr, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        variant->Assign(char_ptr);

        
        env->ReleaseStringUTFChars(jstr, char_ptr);
      } else {
        variant->SetIsVoid(PR_TRUE);
      }
    }
    break;

    case nsXPTType::T_VOID:
    {
      if (aParamInfo.IsRetval()) {  
        aVariant.val.p = reinterpret_cast<void*>(aJValue.j);
      } else if (aJValue.l) {  
        env->GetLongArrayRegion((jlongArray) aJValue.l, 0, 1,
                                (jlong*) aVariant.val.p);
      }
    }
    break;

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

NS_IMETHODIMP
nsJavaXPTCStub::GetWeakReference(nsIWeakReference** aInstancePtr)
{
  if (mMaster)
    return mMaster->GetWeakReference(aInstancePtr);

  LOG(("==> nsJavaXPTCStub::GetWeakReference()\n"));

  if (!aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  jobject javaObject = GetJNIEnv()->CallObjectMethod(mJavaWeakRef,
                                                     getReferentMID);
  nsJavaXPTCStubWeakRef* weakref;
  weakref = new nsJavaXPTCStubWeakRef(javaObject, this);
  if (!weakref)
    return NS_ERROR_OUT_OF_MEMORY;

  *aInstancePtr = weakref;
  NS_ADDREF(*aInstancePtr);
  ++mWeakRefCnt;

  return NS_OK;
}

jobject
nsJavaXPTCStub::GetJavaObject()
{
  JNIEnv* env = GetJNIEnv();
  jobject javaObject = env->CallObjectMethod(mJavaWeakRef, getReferentMID);

#ifdef DEBUG_JAVAXPCOM
  nsIID* iid;
  mIInfo->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  LOG(("< nsJavaXPTCStub (Java=%08x | XPCOM=%08x | IID=%s)\n",
       (PRUint32) mJavaRefHashCode, (PRUint32) this, iid_str));
  PR_Free(iid_str);
  nsMemory::Free(iid);
#endif

  return javaObject;
}


 nsresult
nsJavaXPTCStub::GetNewOrUsed(JNIEnv* env, jobject aJavaObject,
                             const nsIID& aIID, void** aResult)
{
  nsJavaXPTCStub* stub;
  jint hash = env->CallStaticIntMethod(systemClass, hashCodeMID, aJavaObject);
  nsresult rv = gJavaToXPTCStubMap->Find(hash, aIID, &stub);
  NS_ENSURE_SUCCESS(rv, rv);
  if (stub) {
    
    *aResult = stub;
    return NS_OK;
  }

  
  
  
  

  
  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceInfo> iinfo;
  rv = iim->GetInfoForIID(&aIID, getter_AddRefs(iinfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  stub = new nsJavaXPTCStub(aJavaObject, iinfo, &rv);
  if (!stub)
    return NS_ERROR_OUT_OF_MEMORY;
  if (NS_FAILED(rv)) {
    delete stub;
    return rv;
  }

  rv = gJavaToXPTCStubMap->Add(hash, stub);
  if (NS_FAILED(rv)) {
    delete stub;
    return rv;
  }

  NS_ADDREF(stub);
  *aResult = stub;

  return NS_OK;
}
