





#ifndef nsDOMClassInfo_h___
#define nsDOMClassInfo_h___

#include "mozilla/Attributes.h"
#include "nsIXPCScriptable.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMScriptObjectFactory.h"
#include "js/Id.h"
#include "nsIXPConnect.h"

#ifdef XP_WIN
#undef GetClassName
#endif

class nsContentList;
class nsDocument;
struct nsGlobalNameStruct;
class nsGlobalWindow;
class nsIScriptSecurityManager;

struct nsDOMClassInfoData;

typedef nsIClassInfo* (*nsDOMClassInfoConstructorFnc)
  (nsDOMClassInfoData* aData);

typedef nsresult (*nsDOMConstructorFunc)(nsISupports** aNewObject);

struct nsDOMClassInfoData
{
  const char *mName;
  const char16_t *mNameUTF16;
  union {
    nsDOMClassInfoConstructorFnc mConstructorFptr;
    nsDOMClassInfoExternalConstructorFnc mExternalConstructorFptr;
  } u;

  nsIClassInfo *mCachedClassInfo; 
                                  
  const nsIID *mProtoChainInterface;
  const nsIID **mInterfaces;
  uint32_t mScriptableFlags : 31; 
  uint32_t mHasClassInterface : 1;
  uint32_t mInterfacesBitmap;
  bool mChromeOnly : 1;
  bool mAllowXBL : 1;
  bool mDisabled : 1;
#ifdef DEBUG
  uint32_t mDebugID;
#endif
};

struct nsExternalDOMClassInfoData : public nsDOMClassInfoData
{
  const nsCID *mConstructorCID;
};





#define GET_CLEAN_CI_PTR(_ptr) (nsIClassInfo*)(uintptr_t(_ptr) & ~0x1)
#define MARK_EXTERNAL(_ptr) (nsIClassInfo*)(uintptr_t(_ptr) | 0x1)
#define IS_EXTERNAL(_ptr) (uintptr_t(_ptr) & 0x1)

class nsWindowSH;

class nsDOMClassInfo : public nsXPCClassInfo
{
  friend class nsWindowSH;

protected:
  virtual ~nsDOMClassInfo();

public:
  explicit nsDOMClassInfo(nsDOMClassInfoData* aData);

  NS_DECL_NSIXPCSCRIPTABLE

  NS_DECL_ISUPPORTS

  NS_DECL_NSICLASSINFO

  
  
  
  
  
  
  
  

  static nsIClassInfo* GetClassInfoInstance(nsDOMClassInfoData* aData);

  static nsresult Init();
  static void ShutDown();

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDOMClassInfo(aData);
  }

  















  static bool ObjectIsNativeWrapper(JSContext* cx, JSObject* obj);

  static nsISupports *GetNative(nsIXPConnectWrappedNative *wrapper, JSObject *obj);

  static nsIXPConnect *XPConnect()
  {
    return sXPConnect;
  }

protected:
  friend nsIClassInfo* NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID);

  const nsDOMClassInfoData* mData;

  virtual void PreserveWrapper(nsISupports *aNative) MOZ_OVERRIDE
  {
  }

  virtual uint32_t GetInterfacesBitmap() MOZ_OVERRIDE
  {
    return mData->mInterfacesBitmap;
  }

  static nsresult RegisterClassProtos(int32_t aDOMClassInfoID);
  static nsresult RegisterExternalClasses();
  nsresult ResolveConstructor(JSContext *cx, JSObject *obj,
                              JSObject **objp);

  static nsIXPConnect *sXPConnect;

  
  static nsresult DefineStaticJSVals(JSContext *cx);

  static bool sIsInitialized;

public:
  static jsid sConstructor_id;
  static jsid sWrappedJSObject_id;
};



inline
const nsQueryInterface
do_QueryWrappedNative(nsIXPConnectWrappedNative *wrapper, JSObject *obj)
{
  return nsQueryInterface(nsDOMClassInfo::GetNative(wrapper, obj));
}



inline
const nsQueryInterfaceWithError
do_QueryWrappedNative(nsIXPConnectWrappedNative *wrapper, JSObject *obj,
                      nsresult *aError)

{
  return nsQueryInterfaceWithError(nsDOMClassInfo::GetNative(wrapper, obj),
                                   aError);
}

inline
nsQueryInterface
do_QueryWrapper(JSContext *cx, JSObject *obj)
{
  nsISupports *native =
    nsDOMClassInfo::XPConnect()->GetNativeOfWrapper(cx, obj);
  return nsQueryInterface(native);
}

inline
nsQueryInterfaceWithError
do_QueryWrapper(JSContext *cx, JSObject *obj, nsresult* error)
{
  nsISupports *native =
    nsDOMClassInfo::XPConnect()->GetNativeOfWrapper(cx, obj);
  return nsQueryInterfaceWithError(native, error);
}


typedef nsDOMClassInfo nsDOMGenericSH;


class nsEventTargetSH : public nsDOMGenericSH
{
protected:
  explicit nsEventTargetSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsEventTargetSH()
  {
  }
public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj) MOZ_OVERRIDE;
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, JS::Value *vp, bool *_retval) MOZ_OVERRIDE;

  virtual void PreserveWrapper(nsISupports *aNative) MOZ_OVERRIDE;

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsEventTargetSH(aData);
  }
};



class nsWindowSH
{
protected:
  static nsresult GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                                JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc);

  friend class nsGlobalWindow;
public:
  static bool NameStructEnabled(JSContext* aCx, nsGlobalWindow *aWin,
                                const nsAString& aName,
                                const nsGlobalNameStruct& aNameStruct);
};









class nsEventListenerThisTranslator : public nsIXPCFunctionThisTranslator
{
  virtual ~nsEventListenerThisTranslator()
  {
  }

public:
  nsEventListenerThisTranslator()
  {
  }

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIXPCFUNCTIONTHISTRANSLATOR
};

class nsDOMConstructorSH : public nsDOMGenericSH
{
protected:
  explicit nsDOMConstructorSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj) MOZ_OVERRIDE;
  NS_IMETHOD PostCreatePrototype(JSContext * cx, JSObject * proto) MOZ_OVERRIDE
  {
    return NS_OK;
  }
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, JSObject **objp,
                        bool *_retval) MOZ_OVERRIDE;
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                  JSObject *obj, const JS::CallArgs &args, bool *_retval) MOZ_OVERRIDE;

  NS_IMETHOD Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, const JS::CallArgs &args, bool *_retval) MOZ_OVERRIDE;

  NS_IMETHOD HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, JS::Handle<JS::Value> val, bool *bp,
                         bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDOMConstructorSH(aData);
  }
};

class nsNonDOMObjectSH : public nsDOMGenericSH
{
protected:
  explicit nsNonDOMObjectSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsNonDOMObjectSH()
  {
  }

public:
  NS_IMETHOD GetFlags(uint32_t *aFlags) MOZ_OVERRIDE;

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNonDOMObjectSH(aData);
  }
};

#endif
