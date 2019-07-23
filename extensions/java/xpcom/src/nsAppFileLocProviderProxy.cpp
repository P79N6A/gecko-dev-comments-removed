




































#include "nsAppFileLocProviderProxy.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"


nsAppFileLocProviderProxy::nsAppFileLocProviderProxy(jobject aJavaObject)
{
  mJavaLocProvider = GetJNIEnv()->NewGlobalRef(aJavaObject);
}

nsAppFileLocProviderProxy::~nsAppFileLocProviderProxy()
{
  GetJNIEnv()->DeleteGlobalRef(mJavaLocProvider);
}

NS_IMPL_ISUPPORTS2(nsAppFileLocProviderProxy,
                   nsIDirectoryServiceProvider,
                   nsIDirectoryServiceProvider2)




NS_IMETHODIMP
nsAppFileLocProviderProxy::GetFile(const char* aProp, PRBool* aIsPersistant,
                                   nsIFile** aResult)
{
  
  JNIEnv* env = GetJNIEnv();
  jstring prop = env->NewStringUTF(aProp);
  if (!prop)
    return NS_ERROR_OUT_OF_MEMORY;
  jbooleanArray persistant = env->NewBooleanArray(1);
  if (!persistant)
    return NS_ERROR_OUT_OF_MEMORY;

  
  jmethodID mid = nsnull;
  jclass clazz = env->GetObjectClass(mJavaLocProvider);
  if (clazz) {
    mid = env->GetMethodID(clazz, "getFile",
                           "(Ljava/lang/String;[Z)Ljava/io/File;");
  }
  if (!mid)
    return NS_ERROR_FAILURE;

  
  jobject javaFile = nsnull;
  javaFile = env->CallObjectMethod(mJavaLocProvider, mid, prop, persistant);
  if (javaFile == nsnull || env->ExceptionCheck())
    return NS_ERROR_FAILURE;

  
  env->GetBooleanArrayRegion(persistant, 0, 1, (jboolean*) aIsPersistant);

  
  nsCOMPtr<nsILocalFile> localFile;
  nsresult rv = File_to_nsILocalFile(env, javaFile, getter_AddRefs(localFile));
  if (NS_SUCCEEDED(rv)) {
    return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)aResult);
  }

  return rv;
}




class DirectoryEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS

  DirectoryEnumerator(jobjectArray aJavaFileArray)
    : mIndex(0)
  {
    JNIEnv* env = GetJNIEnv();
    mJavaFileArray = static_cast<jobjectArray>
                                (env->NewGlobalRef(aJavaFileArray));
    mArraySize = env->GetArrayLength(aJavaFileArray);
  }

  ~DirectoryEnumerator()
  {
    GetJNIEnv()->DeleteGlobalRef(mJavaFileArray);
  }

  NS_IMETHOD HasMoreElements(PRBool* aResult)
  {
    if (!mJavaFileArray) {
      *aResult = PR_FALSE;
    } else {
      *aResult = (mIndex < mArraySize);
    }
    return NS_OK;
  }

  NS_IMETHOD GetNext(nsISupports** aResult)
  {
    nsresult rv = NS_ERROR_FAILURE;

    JNIEnv* env = GetJNIEnv();
    jobject javaFile = env->GetObjectArrayElement(mJavaFileArray, mIndex++);
    if (javaFile) {
      nsCOMPtr<nsILocalFile> localFile;
      rv = File_to_nsILocalFile(env, javaFile, getter_AddRefs(localFile));
      env->DeleteLocalRef(javaFile);

      if (NS_SUCCEEDED(rv)) {
        return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)aResult);
      }
    }

    env->ExceptionClear();
    return NS_ERROR_FAILURE;
  }

private:
  jobjectArray  mJavaFileArray;
  PRUint32      mArraySize;
  PRUint32      mIndex;
};

NS_IMPL_ISUPPORTS1(DirectoryEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsAppFileLocProviderProxy::GetFiles(const char* aProp,
                                    nsISimpleEnumerator** aResult)
{
  nsresult rv = NS_OK;

  
  JNIEnv* env = GetJNIEnv();
  jstring prop = env->NewStringUTF(aProp);
  if (!prop)
    rv = NS_ERROR_OUT_OF_MEMORY;

  
  jmethodID mid = nsnull;
  if (NS_SUCCEEDED(rv)) {
    jclass clazz = env->GetObjectClass(mJavaLocProvider);
    if (clazz) {
      mid = env->GetMethodID(clazz, "getFiles",
                             "(Ljava/lang/String;)[Ljava/io/File;");
      env->DeleteLocalRef(clazz);
    }
    if (!mid)
      rv = NS_ERROR_FAILURE;
  }

  
  jobject javaFileArray = nsnull;
  if (NS_SUCCEEDED(rv)) {
    javaFileArray = env->CallObjectMethod(mJavaLocProvider, mid, prop);

    
    jthrowable exp = env->ExceptionOccurred();
    if (exp) {
#ifdef DEBUG
      env->ExceptionDescribe();
#endif

      
      
      
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
    } else {
      
      if (javaFileArray == nsnull) {
        rv = NS_ERROR_FAILURE;
      }
    }
  }

  if (NS_SUCCEEDED(rv)) {
    
    *aResult = new DirectoryEnumerator(static_cast<jobjectArray>
                                                  (javaFileArray));
    NS_ADDREF(*aResult);
    return NS_OK;
  }

  
  *aResult = nsnull;
  env->ExceptionClear();
  return rv;
}




nsresult
NS_NewAppFileLocProviderProxy(jobject aJavaLocProvider,
                              nsIDirectoryServiceProvider** aResult)
{
  nsAppFileLocProviderProxy* provider =
            new nsAppFileLocProviderProxy(aJavaLocProvider);
  if (provider == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(provider);

  *aResult = provider;
  return NS_OK;
}

