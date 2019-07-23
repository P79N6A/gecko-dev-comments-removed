




































#ifndef _nsJavaXPCOMBindingUtils_h_
#define _nsJavaXPCOMBindingUtils_h_

#include "jni.h"
#include "xptcall.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "pldhash.h"
#include "nsJavaXPTCStub.h"
#include "nsAutoLock.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"




#ifdef DEBUG_JAVAXPCOM
#define LOG(x)  printf x
#else
#define LOG(x)
#endif






extern jclass systemClass;
extern jclass booleanClass;
extern jclass charClass;
extern jclass byteClass;
extern jclass shortClass;
extern jclass intClass;
extern jclass longClass;
extern jclass floatClass;
extern jclass doubleClass;
extern jclass stringClass;
extern jclass nsISupportsClass;
extern jclass xpcomExceptionClass;
extern jclass xpcomJavaProxyClass;
extern jclass weakReferenceClass;
extern jclass javaXPCOMUtilsClass;

extern jmethodID hashCodeMID;
extern jmethodID booleanValueMID;
extern jmethodID booleanInitMID;
extern jmethodID charValueMID;
extern jmethodID charInitMID;
extern jmethodID byteValueMID;
extern jmethodID byteInitMID;
extern jmethodID shortValueMID;
extern jmethodID shortInitMID;
extern jmethodID intValueMID;
extern jmethodID intInitMID;
extern jmethodID longValueMID;
extern jmethodID longInitMID;
extern jmethodID floatValueMID;
extern jmethodID floatInitMID;
extern jmethodID doubleValueMID;
extern jmethodID doubleInitMID;
extern jmethodID createProxyMID;
extern jmethodID isXPCOMJavaProxyMID;
extern jmethodID getNativeXPCOMInstMID;
extern jmethodID weakReferenceConstructorMID;
extern jmethodID getReferentMID;
extern jmethodID clearReferentMID;
extern jmethodID findClassInLoaderMID;

#ifdef DEBUG_JAVAXPCOM
extern jmethodID getNameMID;
extern jmethodID proxyToStringMID;
#endif

class NativeToJavaProxyMap;
extern NativeToJavaProxyMap* gNativeToJavaProxyMap;
class JavaToXPTCStubMap;
extern JavaToXPTCStubMap* gJavaToXPTCStubMap;

extern nsTHashtable<nsDepCharHashKey>* gJavaKeywords;




extern PRLock* gJavaXPCOMLock;

extern PRBool gJavaXPCOMInitialized;






PRBool InitializeJavaGlobals(JNIEnv *env);





void FreeJavaGlobals(JNIEnv* env);






class JavaXPCOMInstance
{
public:
  JavaXPCOMInstance(nsISupports* aInstance, nsIInterfaceInfo* aIInfo);
  ~JavaXPCOMInstance();

  nsISupports* GetInstance()  { return mInstance; }
  nsIInterfaceInfo* InterfaceInfo() { return mIInfo; }

private:
  nsISupports*        mInstance;
  nsIInterfaceInfo*   mIInfo;
};









class NativeToJavaProxyMap
{
  friend PLDHashOperator DestroyJavaProxyMappingEnum(PLDHashTable* aTable,
                                                     PLDHashEntryHdr* aHeader,
                                                     PRUint32 aNumber,
                                                     void* aData);

protected:
  struct ProxyList
  {
    ProxyList(const jobject aRef, const nsIID& aIID, ProxyList* aList)
      : javaObject(aRef)
      , iid(aIID)
      , next(aList)
    { }

    const jobject   javaObject;
    const nsIID     iid;
    ProxyList*      next;
  };

  struct Entry : public PLDHashEntryHdr
  {
    nsISupports*  key;
    ProxyList*    list;
  };

public:
  NativeToJavaProxyMap()
    : mHashTable(nsnull)
  { }

  ~NativeToJavaProxyMap()
  {
    NS_ASSERTION(mHashTable == nsnull,
                 "MUST call Destroy() before deleting object");
  }

  nsresult Init();

  nsresult Destroy(JNIEnv* env);

  nsresult Add(JNIEnv* env, nsISupports* aXPCOMObject, const nsIID& aIID,
               jobject aProxy);

  nsresult Find(JNIEnv* env, nsISupports* aNativeObject, const nsIID& aIID,
                jobject* aResult);

  nsresult Remove(JNIEnv* env, nsISupports* aNativeObject, const nsIID& aIID);

protected:
  PLDHashTable* mHashTable;
};




class JavaToXPTCStubMap
{
  friend PLDHashOperator DestroyXPTCMappingEnum(PLDHashTable* aTable,
                                                PLDHashEntryHdr* aHeader,
                                                PRUint32 aNumber, void* aData);

protected:
  struct Entry : public PLDHashEntryHdr
  {
    jint              key;
    nsJavaXPTCStub*   xptcstub;
  };

public:
  JavaToXPTCStubMap()
    : mHashTable(nsnull)
  { }

  ~JavaToXPTCStubMap()
  {
    NS_ASSERTION(mHashTable == nsnull,
                 "MUST call Destroy() before deleting object");
  }

  nsresult Init();

  nsresult Destroy();

  nsresult Add(jint aJavaObjectHashCode, nsJavaXPTCStub* aProxy);

  nsresult Find(jint aJavaObjectHashCode, const nsIID& aIID,
                nsJavaXPTCStub** aResult);

  nsresult Remove(jint aJavaObjectHashCode);

protected:
  PLDHashTable* mHashTable;
};


















nsresult NativeInterfaceToJavaObject(JNIEnv* env, nsISupports* aXPCOMObject,
                                     const nsIID& aIID, jobject aObjectLoader,
                                     jobject* aResult);











nsresult JavaObjectToNativeInterface(JNIEnv* env, jobject aJavaObject,
                                     const nsIID& aIID, void** aResult);

nsresult GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                              const XPTMethodDescriptor *methodInfo,
                              const nsXPTParamInfo &paramInfo,
                              PRUint8 paramType, PRUint16 methodIndex,
                              nsXPTCMiniVariant *dispatchParams,
                              PRBool isFullVariantArray,
                              nsID &result);














inline jclass
FindClassInLoader(JNIEnv* env, jobject aObjectLoader, const char* aClassName)
{
  jclass clazz = nsnull;
  jstring name = env->NewStringUTF(aClassName);
  if (name)
    clazz = (jclass) env->CallStaticObjectMethod(javaXPCOMUtilsClass,
                                  findClassInLoaderMID, aObjectLoader, name);

#ifdef DEBUG
  if (!clazz)
    fprintf(stderr, "WARNING: failed to find class [%s]\n", aClassName);
#endif
  return clazz;
}













JNIEnv* GetJNIEnv();

















void ThrowException(JNIEnv* env, const nsresult aErrorCode,
                    const char* aMessage);












nsAString* jstring_to_nsAString(JNIEnv* env, jstring aString);
nsACString* jstring_to_nsACString(JNIEnv* env, jstring aString);










nsresult File_to_nsILocalFile(JNIEnv* env, jobject aFile,
                              nsILocalFile** aLocalFile);

#endif 
