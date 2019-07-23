



































#include "nsJavaInterfaces.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPTCStub.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "jni.h"
#include "xptcall.h"
#include "nsIInterfaceInfoManager.h"
#include "nsString.h"
#include "nsCRT.h"
#include "prmem.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

static nsID nullID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};


nsresult
CreateJavaArray(JNIEnv* env, PRUint8 aType, PRUint32 aSize, const nsID& aIID,
                jobject* aResult)
{
  jobject array = nsnull;
  switch (aType)
  {
    case nsXPTType::T_I8:
      array = env->NewByteArray(aSize);
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
      array = env->NewShortArray(aSize);
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
      array = env->NewIntArray(aSize);
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
      array = env->NewLongArray(aSize);
      break;

    case nsXPTType::T_FLOAT:
      array = env->NewFloatArray(aSize);
      break;

    
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
      array = env->NewDoubleArray(aSize);
      break;

    case nsXPTType::T_BOOL:
      array = env->NewBooleanArray(aSize);
      break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
      array = env->NewCharArray(aSize);
      break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    case nsXPTType::T_IID:
    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      array = env->NewObjectArray(aSize, stringClass, nsnull);
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      nsCOMPtr<nsIInterfaceInfoManager>
        iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
      NS_ASSERTION(iim, "Failed to get InterfaceInfoManager");
      if (!iim)
        return NS_ERROR_FAILURE;

      
      nsCOMPtr<nsIInterfaceInfo> info;
      nsresult rv = iim->GetInfoForIID(&aIID, getter_AddRefs(info));
      if (NS_FAILED(rv))
        return rv;

      
      const char* iface_name;
      rv = info->GetNameShared(&iface_name);
      if (NS_FAILED(rv))
        return rv;

      
      nsCAutoString class_name("org/mozilla/interfaces/");
      class_name.AppendASCII(iface_name);
      jclass ifaceClass = env->FindClass(class_name.get());
      if (!ifaceClass)
        return NS_ERROR_FAILURE;

      array = env->NewObjectArray(aSize, ifaceClass, nsnull);
      break;
    }

    case nsXPTType::T_VOID:
      array = env->NewLongArray(aSize);
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = array;
  return NS_OK;
}

nsresult
GetNativeArrayElement(PRUint8 aType, void* aArray, PRUint32 aIndex,
                      nsXPTCVariant* aResult)
{
  switch (aType)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      aResult->val.u8 = NS_STATIC_CAST(PRUint8*, aArray)[aIndex];
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      aResult->val.u16 = NS_STATIC_CAST(PRUint16*, aArray)[aIndex];
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      aResult->val.u32 = NS_STATIC_CAST(PRUint32*, aArray)[aIndex];
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      aResult->val.u64 = NS_STATIC_CAST(PRUint64*, aArray)[aIndex];
      break;

    case nsXPTType::T_FLOAT:
      aResult->val.f = NS_STATIC_CAST(float*, aArray)[aIndex];
      break;

    case nsXPTType::T_DOUBLE:
      aResult->val.d = NS_STATIC_CAST(double*, aArray)[aIndex];
      break;

    case nsXPTType::T_BOOL:
      aResult->val.b = NS_STATIC_CAST(PRBool*, aArray)[aIndex];
      break;

    case nsXPTType::T_CHAR:
      aResult->val.c = NS_STATIC_CAST(char*, aArray)[aIndex];
      break;

    case nsXPTType::T_WCHAR:
      aResult->val.wc = NS_STATIC_CAST(PRUnichar*, aArray)[aIndex];
      break;

    case nsXPTType::T_CHAR_STR:
      aResult->val.p = NS_STATIC_CAST(char**, aArray)[aIndex];
      break;

    case nsXPTType::T_WCHAR_STR:
      aResult->val.p = NS_STATIC_CAST(PRUnichar**, aArray)[aIndex];
      break;

    case nsXPTType::T_IID:
      aResult->val.p = NS_STATIC_CAST(nsID**, aArray)[aIndex];
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      aResult->val.p = NS_STATIC_CAST(nsISupports**, aArray)[aIndex];
      break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
      aResult->val.p = NS_STATIC_CAST(nsString**, aArray)[aIndex];
      break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      aResult->val.p = NS_STATIC_CAST(nsCString**, aArray)[aIndex];
      break;

    case nsXPTType::T_VOID:
      aResult->val.p = NS_STATIC_CAST(void**, aArray)[aIndex];
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
CreateNativeArray(PRUint8 aType, PRUint32 aSize, void** aResult)
{
  void* array = nsnull;
  switch (aType)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      array = PR_Malloc(aSize * sizeof(PRUint8));
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      array = PR_Malloc(aSize * sizeof(PRUint16));
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      array = PR_Malloc(aSize * sizeof(PRUint32));
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      array = PR_Malloc(aSize * sizeof(PRUint64));
      break;

    case nsXPTType::T_FLOAT:
      array = PR_Malloc(aSize * sizeof(float));
      break;

    case nsXPTType::T_DOUBLE:
      array = PR_Malloc(aSize * sizeof(double));
      break;

    case nsXPTType::T_BOOL:
      array = PR_Malloc(aSize * sizeof(PRBool));
      break;

    case nsXPTType::T_CHAR:
      array = PR_Malloc(aSize * sizeof(char));
      break;

    case nsXPTType::T_WCHAR:
      array = PR_Malloc(aSize * sizeof(PRUnichar));
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
      array = PR_Malloc(aSize * sizeof(void*));
      break;

    case nsXPTType::T_VOID:
      array = PR_Malloc(aSize * sizeof(void*));
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = array;
  return NS_OK;
}




nsresult
SetupParams(JNIEnv *env, const jobject aParam, PRUint8 aType, PRBool aIsOut,
            const nsID& aIID, PRUint8 aArrayType, PRUint32 aArraySize,
            PRBool aIsArrayElement, PRUint32 aIndex, nsXPTCVariant &aVariant)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case nsXPTType::T_I8:
    {
      LOG(("byte\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.i8 = env->CallByteMethod(aParam, byteValueMID);
      } else { 
        jbyte value;
        if (aParam) {
          env->GetByteArrayRegion((jbyteArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.i8 = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(PRInt8*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:   
    {
      LOG(("short\n"));
      if (!aIsOut && !aIsArrayElement) {  
        jshort value = env->CallShortMethod(aParam, shortValueMID);
        if (aType == nsXPTType::T_I16)
          aVariant.val.i16 = value;
        else
          aVariant.val.u8 = value;
      } else { 
        jshort value;
        if (aParam) {
          env->GetShortArrayRegion((jshortArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            if (aType == nsXPTType::T_I16)
              aVariant.val.i16 = value;
            else
              aVariant.val.u8 = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          if (aType == nsXPTType::T_I16)
            NS_STATIC_CAST(PRInt16*, aVariant.val.p)[aIndex] = value;
          else
            NS_STATIC_CAST(PRUint8*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:  
    {
      LOG(("int\n"));
      if (!aIsOut && !aIsArrayElement) {  
        jint value = env->CallIntMethod(aParam, intValueMID);
        if (aType == nsXPTType::T_I32)
          aVariant.val.i32 = value;
        else
          aVariant.val.u16 = value;
      } else { 
        jint value;
        if (aParam) {
          env->GetIntArrayRegion((jintArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            if (aType == nsXPTType::T_I32)
              aVariant.val.i32 = value;
            else
              aVariant.val.u16 = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          if (aType == nsXPTType::T_I32)
            NS_STATIC_CAST(PRInt32*, aVariant.val.p)[aIndex] = value;
          else
            NS_STATIC_CAST(PRUint16*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:  
    {
      LOG(("long\n"));
      if (!aIsOut && !aIsArrayElement) {  
        jlong value = env->CallLongMethod(aParam, longValueMID);
        if (aType == nsXPTType::T_I64)
          aVariant.val.i64 = value;
        else
          aVariant.val.u32 = value;
      } else { 
        jlong value;
        if (aParam) {
          env->GetLongArrayRegion((jlongArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            if (aType == nsXPTType::T_I64)
              aVariant.val.i64 = value;
            else
              aVariant.val.u32 = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          if (aType == nsXPTType::T_I64)
            NS_STATIC_CAST(PRInt64*, aVariant.val.p)[aIndex] = value;
          else
            NS_STATIC_CAST(PRUint32*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_FLOAT:
    {
      LOG(("float\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.f = env->CallFloatMethod(aParam, floatValueMID);
      } else { 
        jfloat value;
        if (aParam) {
          env->GetFloatArrayRegion((jfloatArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.f = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(float*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    
    case nsXPTType::T_U64:  
    case nsXPTType::T_DOUBLE:
    {
      LOG(("double\n"));
      if (!aIsOut && !aIsArrayElement) {  
        jdouble value = env->CallDoubleMethod(aParam, doubleValueMID);
        if (aType == nsXPTType::T_DOUBLE)
          aVariant.val.d = value;
        else
          aVariant.val.u64 = NS_STATIC_CAST(PRUint64, value);
      } else { 
        jdouble value;
        if (aParam) {
          env->GetDoubleArrayRegion((jdoubleArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            if (aType == nsXPTType::T_DOUBLE)
              aVariant.val.d = value;
            else
              aVariant.val.u64 = NS_STATIC_CAST(PRUint64, value);
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          if (aType == nsXPTType::T_DOUBLE)
            NS_STATIC_CAST(double*, aVariant.val.p)[aIndex] = value;
          else
            NS_STATIC_CAST(PRUint64*, aVariant.val.p)[aIndex] =
                                                NS_STATIC_CAST(PRUint64, value);
        }
      }
      break;
    }

    case nsXPTType::T_BOOL:
    {
      LOG(("boolean\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.b = env->CallBooleanMethod(aParam, booleanValueMID);
      } else { 
        jboolean value;
        if (aParam) {
          env->GetBooleanArrayRegion((jbooleanArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.b = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(PRBool*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_CHAR:
    {
      LOG(("char\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.c = env->CallCharMethod(aParam, charValueMID);
      } else { 
        jchar value;
        if (aParam) {
          env->GetCharArrayRegion((jcharArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.c = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(char*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_WCHAR:
    {
      LOG(("char\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.wc = env->CallCharMethod(aParam, charValueMID);
      } else { 
        jchar value;
        if (aParam) {
          env->GetCharArrayRegion((jcharArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.wc = value;
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(PRUnichar*, aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      LOG(("String\n"));
      jstring data = nsnull;
      if (!aIsOut && !aIsArrayElement) {  
        data = (jstring) aParam;
      } else if (aParam) {  
        data = (jstring) env->GetObjectArrayElement((jobjectArray) aParam,
                                                    aIndex);
      }

      void* buf = nsnull;
      if (data) {
        jsize uniLength = env->GetStringLength(data);
        if (uniLength > 0) {
          if (aType == nsXPTType::T_CHAR_STR) {
            jsize utf8Length = env->GetStringUTFLength(data);
            buf = nsMemory::Alloc((utf8Length + 1) * sizeof(char));
            if (!buf) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }

            char* char_str = NS_STATIC_CAST(char*, buf);
            env->GetStringUTFRegion(data, 0, uniLength, char_str);
            char_str[utf8Length] = '\0';

          } else {  
            buf = nsMemory::Alloc((uniLength + 1) * sizeof(jchar));
            if (!buf) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }

            jchar* jchar_str = NS_STATIC_CAST(jchar*, buf);
            env->GetStringRegion(data, 0, uniLength, jchar_str);
            jchar_str[uniLength] = '\0';
          }
        } else {
          
          buf = nsMemory::Alloc(2);
          if (!buf) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          ((jchar*)buf)[0] = '\0';
        }
      }

      if (!aIsArrayElement) { 
        aVariant.val.p = buf;
        if (aIsOut) { 
          aVariant.ptr = &aVariant.val;
          aVariant.SetPtrIsData();
        }
      } else {  
        if (aType == nsXPTType::T_CHAR_STR) {
          char* str = NS_STATIC_CAST(char*, buf);
          NS_STATIC_CAST(char**, aVariant.val.p)[aIndex] = str;
        } else {
          PRUnichar* str = NS_STATIC_CAST(PRUnichar*, buf);
          NS_STATIC_CAST(PRUnichar**, aVariant.val.p)[aIndex] = str;
        }
      }
      break;
    }

    case nsXPTType::T_IID:
    {
      LOG(("String(IID)\n"));
      jstring data = nsnull;
      if (!aIsOut && !aIsArrayElement) {  
        data = (jstring) aParam;
      } else if (aParam) {  
        data = (jstring) env->GetObjectArrayElement((jobjectArray) aParam,
                                                    aIndex);
      }

      nsID* iid = new nsID;
      if (!iid) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }
      if (data) {
        
        const char* str = env->GetStringUTFChars(data, nsnull);
        if (!str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        
        iid->Parse(str);
        env->ReleaseStringUTFChars(data, str);
      } else {
        *iid = nullID;
      }

      if (!aIsArrayElement) { 
        aVariant.val.p = iid;
        if (aIsOut) { 
          aVariant.ptr = &aVariant.val;
          aVariant.SetPtrIsData();
        }
      } else {  
        NS_STATIC_CAST(nsID**, aVariant.val.p)[aIndex] = iid;
      }
      break;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      LOG(("nsISupports\n"));
      jobject java_obj = nsnull;
      if (!aIsOut && !aIsArrayElement) {  
        java_obj = (jobject) aParam;
      } else if (aParam) {  
        java_obj = (jobject) env->GetObjectArrayElement((jobjectArray) aParam,
                                                        aIndex);
      }

      nsISupports* xpcom_obj;
      if (java_obj) {
        
        
        
        PRBool isWeakRef;
        nsID iid;
        if (aIID.Equals(NS_GET_IID(nsIWeakReference))) {
          isWeakRef = PR_TRUE;
          iid = NS_GET_IID(nsISupports);
        } else {
          isWeakRef = PR_FALSE;
          iid = aIID;
        }

        rv = GetNewOrUsedXPCOMObject(env, java_obj, iid, &xpcom_obj);
        if (NS_FAILED(rv))
          break;

        
        
        if (isWeakRef) {
          nsCOMPtr<nsISupportsWeakReference> supportsweak =
                                                 do_QueryInterface(xpcom_obj);
          if (supportsweak) {
            nsWeakPtr weakref;
            supportsweak->GetWeakReference(getter_AddRefs(weakref));
            NS_RELEASE(xpcom_obj);
            xpcom_obj = weakref;
            NS_ADDREF(xpcom_obj);
          } else {
            xpcom_obj = nsnull;
          }
        }
      } else {
        xpcom_obj = nsnull;
      }

      if (!aIsArrayElement) { 
        aVariant.val.p = xpcom_obj;
        aVariant.SetValIsInterface();
        if (aIsOut) { 
          aVariant.ptr = &aVariant.val;
          aVariant.SetPtrIsData();
        }
      } else {  
        NS_STATIC_CAST(nsISupports**, aVariant.val.p)[aIndex] = xpcom_obj;
      }
      break;
    }

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      LOG(("String\n"));
      
      NS_PRECONDITION(!aIsOut, "unexpected param descriptor");
      if (aIsOut) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = NS_STATIC_CAST(jstring, aParam);
      nsAString* str = jstring_to_nsAString(env, jstr);
      if (!str) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      aVariant.val.p = str;
      aVariant.SetValIsDOMString();
      break;
    }

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      LOG(("StringUTF\n"));
      
      NS_PRECONDITION(!aIsOut, "unexpected param descriptor");
      if (aIsOut) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = NS_STATIC_CAST(jstring, aParam);
      nsACString* str = jstring_to_nsACString(env, jstr);
      if (!str) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      aVariant.val.p = str;
      if (aType == nsXPTType::T_CSTRING) {
        aVariant.SetValIsCString();
      } else {
        aVariant.SetValIsUTF8String();
      }
      break;
    }

    
    case nsXPTType::T_VOID:
    {
      LOG(("long (void*)\n"));
      if (!aIsOut && !aIsArrayElement) {  
        aVariant.val.p =
          NS_REINTERPRET_CAST(void*, env->CallLongMethod(aParam, longValueMID));
      } else { 
        jlong value;
        if (aParam) {
          env->GetLongArrayRegion((jlongArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { 
          if (aParam) {
            aVariant.val.p = NS_REINTERPRET_CAST(void*, value);
            aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nsnull;
          }
          aVariant.SetPtrIsData();
        } else {  
          NS_STATIC_CAST(void**, aVariant.val.p)[aIndex] =
                  NS_REINTERPRET_CAST(void*, value);
        }
      }
      break;
    }

    case nsXPTType::T_ARRAY:
    {
      jobject sourceArray = nsnull;
      if (!aIsOut) {  
        sourceArray = aParam;
      } else if (aParam) {  
        jobjectArray array = NS_STATIC_CAST(jobjectArray, aParam);
        sourceArray = env->GetObjectArrayElement(array, 0);
      }

      if (sourceArray) {
        rv = CreateNativeArray(aArrayType, aArraySize, &aVariant.val.p);

        for (PRUint32 i = 0; i < aArraySize && NS_SUCCEEDED(rv); i++) {
          rv = SetupParams(env, sourceArray, aArrayType, PR_FALSE, aIID, 0, 0,
                           PR_TRUE, i, aVariant);
        }
      }

      if (aIsOut) { 
        aVariant.ptr = &aVariant.val.p;
        aVariant.SetPtrIsData();
      }
      break;
    }

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}








nsresult
FinalizeParams(JNIEnv *env, const nsXPTParamInfo &aParamInfo, PRUint8 aType,
               nsXPTCVariant &aVariant, const nsID& aIID,
               PRBool aIsArrayElement, PRUint8 aArrayType, PRUint32 aArraySize,
               PRUint32 aIndex, nsresult aInvokeResult, jobject* aParam)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case nsXPTType::T_I8:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jbyte value = aVariant.val.i8;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(byteClass, byteInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetByteArrayRegion((jbyteArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jshort value = (aType == nsXPTType::T_I16) ? aVariant.val.i16 :
                                                     aVariant.val.u8;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(shortClass, shortInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && aParam) {
          env->SetShortArrayRegion((jshortArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jint value = (aType == nsXPTType::T_I32) ? aVariant.val.i32 :
                                                   aVariant.val.u16;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(intClass, intInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetIntArrayRegion((jintArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jlong value = (aType == nsXPTType::T_I64) ? aVariant.val.i64 :
                                                    aVariant.val.u32;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(longClass, longInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetLongArrayRegion((jlongArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_FLOAT:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jfloat value = aVariant.val.f;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(floatClass, floatInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetFloatArrayRegion((jfloatArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jdouble value = (aType == nsXPTType::T_DOUBLE) ? aVariant.val.d :
                                                         aVariant.val.u64;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(doubleClass, doubleInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetDoubleArrayRegion((jdoubleArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_BOOL:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jboolean value = aVariant.val.b;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(booleanClass, booleanInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetBooleanArrayRegion((jbooleanArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jchar value;
        if (aType == nsXPTType::T_CHAR)
          value = aVariant.val.c;
        else
          value = aVariant.val.wc;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(charClass, charInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetCharArrayRegion((jcharArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        
        jstring str = nsnull;
        if (aVariant.val.p) {
          if (aType == nsXPTType::T_CHAR_STR) {
            str = env->NewStringUTF((const char*) aVariant.val.p);
          } else {
            PRUint32 length = nsCRT::strlen((const PRUnichar*) aVariant.val.p);
            str = env->NewString((const jchar*) aVariant.val.p, length);
          }
          if (!str) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = str;
        } else if (*aParam) {
          
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, str);
        }
      }

      
      if (aVariant.val.p)
        nsMemory::Free(aVariant.val.p);
      break;
    }

    case nsXPTType::T_IID:
    {
      nsID* iid = NS_STATIC_CAST(nsID*, aVariant.val.p);

      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        
        jstring str = nsnull;
        if (iid) {
          char* iid_str = iid->ToString();
          if (iid_str) {
            str = env->NewStringUTF(iid_str);
          }
          if (!iid_str || !str) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          PR_Free(iid_str);
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = str;
        } else if (*aParam) {
          
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, str);
        }
      }

      
      
      
      if (aIsArrayElement)
        delete iid;

      break;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      nsISupports* xpcom_obj = NS_STATIC_CAST(nsISupports*, aVariant.val.p);

      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        jobject java_obj = nsnull;
        if (xpcom_obj) {
          
          rv = GetNewOrUsedJavaObject(env, xpcom_obj, aIID, nsnull, &java_obj);
          if (NS_FAILED(rv))
            break;
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = java_obj;
        } else if (*aParam) {
          
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, java_obj);
        }
      }

      
      NS_IF_RELEASE(xpcom_obj);
      break;
    }

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsString* str = NS_STATIC_CAST(nsString*, aVariant.val.p);
      if (NS_SUCCEEDED(aInvokeResult) && aParamInfo.IsDipper()) {
        
        jstring jstr = nsnull;
        if (str && !str->IsVoid()) {
          jstr = env->NewString((const jchar*) str->get(), str->Length());
          if (!jstr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        *aParam = jstr;        
      }

      
      if (str) {
        delete str;
      }
      break;
    }

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsCString* str = NS_STATIC_CAST(nsCString*, aVariant.val.p);
      if (NS_SUCCEEDED(aInvokeResult) && aParamInfo.IsDipper()) {
        
        jstring jstr = nsnull;
        if (str && !str->IsVoid()) {
          jstr = env->NewStringUTF((const char*) str->get());
          if (!jstr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        *aParam = jstr;        
      }

      
      if (str) {
        delete str;
      }
      break;
    }

    case nsXPTType::T_VOID:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jlong value = NS_REINTERPRET_CAST(jlong, aVariant.val.p);
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(longClass, longInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetLongArrayRegion((jlongArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_ARRAY:
    {
      if (aParamInfo.IsOut() && NS_SUCCEEDED(aInvokeResult)) {
        
        jobject jarray = nsnull;
        if (aVariant.val.p) {
          rv = CreateJavaArray(env, aArrayType, aArraySize, aIID, &jarray);
          if (NS_FAILED(rv))
            break;

          nsXPTCVariant var;
          for (PRUint32 i = 0; i < aArraySize && NS_SUCCEEDED(rv); i++) {
            rv = GetNativeArrayElement(aArrayType, aVariant.val.p, i, &var);
            if (NS_SUCCEEDED(rv)) {
              rv = FinalizeParams(env, aParamInfo, aArrayType, var, aIID,
                                  PR_TRUE, 0, 0, i, aInvokeResult, &jarray);
            }
          }
        }

        if (aParamInfo.IsRetval()) {
          *aParam = jarray;
        } else if (*aParam) {
          
          env->SetObjectArrayElement((jobjectArray) *aParam, 0, jarray);
        }
      }

      
      
      
      if (!aParamInfo.IsOut() || NS_FAILED(aInvokeResult)) {
        nsXPTCVariant var;
        for (PRUint32 i = 0; i < aArraySize; i++) {
          rv = GetNativeArrayElement(aArrayType, aVariant.val.p, i, &var);
          if (NS_SUCCEEDED(rv)) {
            FinalizeParams(env, aParamInfo, aArrayType, var, aIID, PR_TRUE,
                           0, 0, i, NS_ERROR_FAILURE, nsnull);
          }
        }
      }
      PR_Free(aVariant.val.p);
      break;
    }

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  
  if (NS_SUCCEEDED(rv) && env->ExceptionCheck())
    rv = NS_ERROR_FAILURE;

  return rv;
}

nsresult
QueryAttributeInfo(nsIInterfaceInfo* aIInfo, const char* aMethodName,
                   PRBool aCapitalizedAttr, PRUint16* aMethodIndex,
                   const nsXPTMethodInfo** aMethodInfo)

{
  nsresult rv = NS_ERROR_FAILURE;

  
  
  
  if (strlen(aMethodName) > 3) {
    if (strncmp("get", aMethodName, 3) == 0) {
      char* getterName = strdup(aMethodName + 3);
      if (!aCapitalizedAttr) {
        getterName[0] = tolower(getterName[0]);
      }
      rv = aIInfo->GetMethodInfoForName(getterName, aMethodIndex, aMethodInfo);
      free(getterName);
    } else if (strncmp("set", aMethodName, 3) == 0) {
      char* setterName = strdup(aMethodName + 3);
      if (!aCapitalizedAttr) {
        setterName[0] = tolower(setterName[0]);
      }
      rv = aIInfo->GetMethodInfoForName(setterName, aMethodIndex, aMethodInfo);
      if (NS_SUCCEEDED(rv)) {
        
        
        
        
        (*aMethodIndex)++;
        rv = aIInfo->GetMethodInfo(*aMethodIndex, aMethodInfo);
        if (NS_SUCCEEDED(rv)) {
          
          if (!(*aMethodInfo)->IsSetter() ||
              strcmp(setterName, (*aMethodInfo)->name) != 0) {
            rv = NS_ERROR_FAILURE;
          }
        }
      }
      free(setterName);
    }
  }

  return rv;
}









nsresult
QueryMethodInfo(nsIInterfaceInfo* aIInfo, const char* aMethodName,
                PRUint16* aMethodIndex, const nsXPTMethodInfo** aMethodInfo)
{
  
  
  const char* methodName = aMethodName;
  if (methodName[0] == '_') {
    methodName++;
  }

  
  
  nsresult rv;
  rv = aIInfo->GetMethodInfoForName(methodName, aMethodIndex, aMethodInfo);
  if (NS_SUCCEEDED(rv))
    return rv;

  
  
  rv = QueryAttributeInfo(aIInfo, methodName, PR_FALSE, aMethodIndex,
                          aMethodInfo);
  if (NS_SUCCEEDED(rv))
    return rv;

  
  char* name = strdup(methodName);
  name[0] = toupper(name[0]);
  rv = aIInfo->GetMethodInfoForName(name, aMethodIndex, aMethodInfo);
  free(name);
  if (NS_SUCCEEDED(rv))
    return rv;

  
  
  rv = QueryAttributeInfo(aIInfo, methodName, PR_TRUE, aMethodIndex,
                          aMethodInfo);

  return rv;
}




extern "C" NS_EXPORT jobject JNICALL
JAVAPROXY_NATIVE(callXPCOMMethod) (JNIEnv *env, jclass that, jobject aJavaProxy,
                                   jstring aMethodName, jobjectArray aParams)
{
  nsresult rv;

  
  void* xpcom_obj;
  rv = GetXPCOMInstFromProxy(env, aJavaProxy, &xpcom_obj);
  if (NS_FAILED(rv)) {
    ThrowException(env, 0, "Failed to get matching XPCOM object");
    return nsnull;
  }
  JavaXPCOMInstance* inst = NS_STATIC_CAST(JavaXPCOMInstance*, xpcom_obj);

  
  PRUint16 methodIndex;
  const nsXPTMethodInfo* methodInfo;
  nsIInterfaceInfo* iinfo = inst->InterfaceInfo();
  const char* methodName = env->GetStringUTFChars(aMethodName, nsnull);
  rv = QueryMethodInfo(iinfo, methodName, &methodIndex, &methodInfo);
  env->ReleaseStringUTFChars(aMethodName, methodName);

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "GetMethodInfoForName failed");
    return nsnull;
  }

#ifdef DEBUG_JAVAXPCOM
  const char* ifaceName;
  iinfo->GetNameShared(&ifaceName);
  LOG(("===> (XPCOM) %s::%s()\n", ifaceName, methodInfo->GetName()));
#endif

  
  PRUint8 paramCount = methodInfo->GetParamCount();
  nsXPTCVariant* params = nsnull;
  if (paramCount)
  {
    params = new nsXPTCVariant[paramCount];
    if (!params) {
      ThrowException(env, NS_ERROR_OUT_OF_MEMORY, "Can't create params array");
      return nsnull;
    }
    memset(params, 0, paramCount * sizeof(nsXPTCVariant));

    for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
    {
      LOG(("\t Param %d: ", i));
      const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
      params[i].type = paramInfo.GetType();

      if (paramInfo.IsIn()) {
        PRUint8 type = params[i].type.TagPart();

        
        PRUint8 arrayType = 0;
        PRUint32 arraySize = 0;
        if (type == nsXPTType::T_ARRAY) {
          
          nsXPTType xpttype;
          rv = iinfo->GetTypeForParam(methodIndex, &paramInfo, 1, &xpttype);
          if (NS_FAILED(rv))
            break;
          arrayType = xpttype.TagPart();

          
          PRUint8 argnum;
          rv = iinfo->GetSizeIsArgNumberForParam(methodIndex, &paramInfo, 0,
                                                 &argnum);
          if (NS_FAILED(rv))
            break;
          arraySize = params[argnum].val.u32;
        }

        
        nsID iid;
        if (type == nsXPTType::T_INTERFACE ||
            type == nsXPTType::T_INTERFACE_IS ||
            type == nsXPTType::T_ARRAY &&
              (arrayType == nsXPTType::T_INTERFACE ||
               arrayType == nsXPTType::T_INTERFACE_IS))
        {
          PRUint8 paramType = type == nsXPTType::T_ARRAY ? arrayType : type;
          rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, paramType,
                                    methodIndex, params, PR_TRUE, iid);
        }

        if (NS_SUCCEEDED(rv)) {
          jobject param = nsnull;
          if (aParams && !paramInfo.IsRetval()) {
            param = env->GetObjectArrayElement(aParams, i);
          }
          rv = SetupParams(env, param, type, paramInfo.IsOut(), iid, arrayType,
                           arraySize, PR_FALSE, 0, params[i]);
        }
      } else {
        LOG(("out/retval\n"));
        params[i].ptr = &(params[i].val);
        params[i].SetPtrIsData();
      }
    }
    if (NS_FAILED(rv)) {
      ThrowException(env, rv, "SetupParams failed");
      return nsnull;
    }
  }

  
  const nsIID* iid;
  iinfo->GetIIDShared(&iid);
  nsISupports* realObject;
  rv = inst->GetInstance()->QueryInterface(*iid, (void**) &realObject);
  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to get real XPCOM object");
    return nsnull;
  }
  nsresult invokeResult = NS_InvokeByIndex(realObject, methodIndex,
                                           paramCount, params);
  NS_RELEASE(realObject);

  
  jobject result = nsnull;
  for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
    PRUint8 type = paramInfo.GetType().TagPart();

    
    PRUint8 arrayType = 0;
    PRUint32 arraySize = 0;
    if (type == nsXPTType::T_ARRAY) {
      
      nsXPTType array_xpttype;
      rv = iinfo->GetTypeForParam(methodIndex, &paramInfo, 1, &array_xpttype);
      if (NS_FAILED(rv))
        break;
      arrayType = array_xpttype.TagPart();

      
      PRUint8 argnum;
      rv = iinfo->GetSizeIsArgNumberForParam(methodIndex, &paramInfo, 0,
                                             &argnum);
      if (NS_FAILED(rv))
        break;
      arraySize = params[argnum].val.u32;
    }

    
    nsID iid;
    if (type == nsXPTType::T_INTERFACE || type == nsXPTType::T_INTERFACE_IS ||
        type == nsXPTType::T_ARRAY && (arrayType == nsXPTType::T_INTERFACE ||
                                       arrayType == nsXPTType::T_INTERFACE_IS))
    {
      PRUint8 paramType = type == nsXPTType::T_ARRAY ? arrayType : type;
      rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, paramType,
                                methodIndex, params, PR_TRUE, iid);
      if (NS_FAILED(rv))
        break;
    }

    jobject* javaElement;
    if (!paramInfo.IsRetval()) {
      jobject element = env->GetObjectArrayElement(aParams, i);
      javaElement = &element;
    } else {
      javaElement = &result;
    }
    rv = FinalizeParams(env, paramInfo, type, params[i], iid, PR_FALSE,
                        arrayType, arraySize, 0, invokeResult, javaElement);
  }
  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "FinalizeParams failed");
    return nsnull;
  }

  
  
  
  
  for (PRUint8 j = 0; j < paramCount && NS_SUCCEEDED(rv); j++)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(j);
    const nsXPTType &type = paramInfo.GetType();
    if (type.TagPart() == nsXPTType::T_IID) {
      nsID* iid = (nsID*) params[j].val.p;
      delete iid;
    }
  }

  if (params) {
    delete params;
  }

  
  
  if (NS_FAILED(invokeResult)) {
    nsCAutoString message("The function \"");
    message.AppendASCII(methodInfo->GetName());
    message.AppendLiteral("\" returned an error condition");
    ThrowException(env, invokeResult, message.get());
  }

  LOG(("<=== (XPCOM) %s::%s()\n", ifaceName, methodInfo->GetName()));
  return result;
}

nsresult
CreateJavaProxy(JNIEnv* env, nsISupports* aXPCOMObject, const nsIID& aIID,
                jobject aObjectLoader, jobject* aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ASSERTION(iim, "Failed to get InterfaceInfoManager");
  if (!iim)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIInterfaceInfo> info;
  nsresult rv = iim->GetInfoForIID(&aIID, getter_AddRefs(info));
  if (NS_FAILED(rv))
    return rv;

  
  JavaXPCOMInstance* inst = new JavaXPCOMInstance(aXPCOMObject, info);
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;

  
  const char* iface_name;
  rv = info->GetNameShared(&iface_name);

  if (NS_SUCCEEDED(rv)) {
    jobject java_obj = nsnull;

    
    nsCAutoString class_name("org.mozilla.interfaces.");
    class_name.AppendASCII(iface_name);
    jclass ifaceClass = FindClassInLoader(env, aObjectLoader, class_name.get());

    if (ifaceClass) {
      java_obj = env->CallStaticObjectMethod(xpcomJavaProxyClass,
                                             createProxyMID, ifaceClass,
                                             NS_REINTERPRET_CAST(jlong, inst));
      if (env->ExceptionCheck())
        java_obj = nsnull;
    }

    if (java_obj) {
#ifdef DEBUG_JAVAXPCOM
      char* iid_str = aIID.ToString();
      LOG(("+ CreateJavaProxy (Java=%08x | XPCOM=%08x | IID=%s)\n",
           (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                               java_obj),
           (PRUint32) aXPCOMObject, iid_str));
      PR_Free(iid_str);
#endif

      
      rv = gNativeToJavaProxyMap->Add(env, aXPCOMObject, aIID, java_obj);
      if (NS_SUCCEEDED(rv)) {
        *aResult = java_obj;
        return NS_OK;
      }
    } else {
      rv = NS_ERROR_FAILURE;
    }
  }

  
  delete inst;
  return rv;
}

nsresult
GetXPCOMInstFromProxy(JNIEnv* env, jobject aJavaObject, void** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  jlong xpcom_obj = env->CallStaticLongMethod(xpcomJavaProxyClass,
                                            getNativeXPCOMInstMID, aJavaObject);

  if (!xpcom_obj || env->ExceptionCheck()) {
    return NS_ERROR_FAILURE;
  }

  *aResult = NS_REINTERPRET_CAST(void*, xpcom_obj);
#ifdef DEBUG_JAVAXPCOM
  JavaXPCOMInstance* inst = NS_STATIC_CAST(JavaXPCOMInstance*, *aResult);
  nsIID* iid;
  inst->InterfaceInfo()->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  LOG(("< GetXPCOMInstFromProxy (Java=%08x | XPCOM=%08x | IID=%s)\n",
       (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                           aJavaObject),
       (PRUint32) inst->GetInstance(), iid_str));
  PR_Free(iid_str);
  nsMemory::Free(iid);
#endif
  return NS_OK;
}




extern "C" NS_EXPORT void JNICALL
JAVAPROXY_NATIVE(finalizeProxy) (JNIEnv *env, jclass that, jobject aJavaProxy)
{
#ifdef DEBUG_JAVAXPCOM
  PRUint32 xpcom_addr = 0;
#endif

  
  
  
  if (gJavaXPCOMLock) {
    nsAutoLock lock(gJavaXPCOMLock);

    
    
    
    
    
    if (gJavaXPCOMInitialized) {
      
      void* xpcom_obj;
      nsresult rv = GetXPCOMInstFromProxy(env, aJavaProxy, &xpcom_obj);
      if (NS_SUCCEEDED(rv)) {
        JavaXPCOMInstance* inst = NS_STATIC_CAST(JavaXPCOMInstance*, xpcom_obj);
#ifdef DEBUG_JAVAXPCOM
        xpcom_addr = NS_REINTERPRET_CAST(PRUint32, inst->GetInstance());
#endif
        nsIID* iid;
        rv = inst->InterfaceInfo()->GetInterfaceIID(&iid);
        if (NS_SUCCEEDED(rv)) {
          rv = gNativeToJavaProxyMap->Remove(env, inst->GetInstance(), *iid);
          nsMemory::Free(iid);
        }
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to RemoveJavaProxy");
        
        lock.unlock();
        delete inst;
      }
    }
  }

#ifdef DEBUG_JAVAXPCOM
  LOG(("- Finalize (Java=%08x | XPCOM=%08x)\n",
       (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                           aJavaProxy),
       xpcom_addr));
#endif
}




extern "C" NS_EXPORT jboolean JNICALL
JAVAPROXY_NATIVE(isSameXPCOMObject) (JNIEnv *env, jclass that,
                                     jobject aProxy1, jobject aProxy2)
{
  void* xpcom_obj1;
  nsresult rv = GetXPCOMInstFromProxy(env, aProxy1, &xpcom_obj1);
  if (NS_SUCCEEDED(rv)) {
    void* xpcom_obj2;
    rv = GetXPCOMInstFromProxy(env, aProxy2, &xpcom_obj2);
    if (NS_SUCCEEDED(rv)) {
      JavaXPCOMInstance* inst1 = NS_STATIC_CAST(JavaXPCOMInstance*, xpcom_obj1);
      JavaXPCOMInstance* inst2 = NS_STATIC_CAST(JavaXPCOMInstance*, xpcom_obj2);
      if (inst1->GetInstance() == inst2->GetInstance()) {
        return JNI_TRUE;
      }
    }
  }
  return JNI_FALSE;
}




extern "C" NS_EXPORT void JNICALL
LOCKPROXY_NATIVE(release) (JNIEnv *env, jclass that, jlong aLockObject)
{
  
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIThread> thread = do_GetMainThread();
  if (thread) {
    rv = NS_ProxyRelease(thread, NS_REINTERPRET_CAST(nsISupports*, aLockObject));
  }
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to release using NS_ProxyRelease");
}
