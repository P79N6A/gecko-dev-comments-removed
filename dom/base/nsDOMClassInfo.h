





#ifndef nsDOMClassInfo_h___
#define nsDOMClassInfo_h___

#include "nsIDOMClassInfo.h"
#include "nsIXPCScriptable.h"
#include "jsapi.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptContext.h"
#include "nsDOMJSUtils.h" 
#include "nsIScriptGlobalObject.h"
#include "xpcpublic.h"

#ifdef XP_WIN
#undef GetClassName
#endif

class nsContentList;
class nsGlobalWindow;
class nsICanvasRenderingContextInternal;
class nsIDOMHTMLOptionsCollection;
class nsIDOMWindow;
class nsIForm;
class nsIHTMLDocument;
class nsNPAPIPluginInstance;

class nsIDOMCrypto;
#ifndef MOZ_DISABLE_CRYPTOLEGACY
class nsIDOMCRMFObject;
#endif

struct nsDOMClassInfoData;

typedef nsIClassInfo* (*nsDOMClassInfoConstructorFnc)
  (nsDOMClassInfoData* aData);

typedef nsresult (*nsDOMConstructorFunc)(nsISupports** aNewObject);

struct nsDOMClassInfoData
{
  const char *mName;
  const PRUnichar *mNameUTF16;
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
  bool mChromeOnly;
  bool mDisabled;
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


class nsDOMClassInfo : public nsXPCClassInfo
{
public:
  nsDOMClassInfo(nsDOMClassInfoData* aData);
  virtual ~nsDOMClassInfo();

  NS_DECL_NSIXPCSCRIPTABLE

  NS_DECL_ISUPPORTS

  NS_DECL_NSICLASSINFO

  
  
  
  
  
  
  
  

  static nsIClassInfo* GetClassInfoInstance(nsDOMClassInfoData* aData);

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

  virtual void PreserveWrapper(nsISupports *aNative)
  {
  }

  virtual uint32_t GetInterfacesBitmap()
  {
    return mData->mInterfacesBitmap;
  }

  static nsresult Init();
  static nsresult RegisterClassProtos(int32_t aDOMClassInfoID);
  static nsresult RegisterExternalClasses();
  nsresult ResolveConstructor(JSContext *cx, JSObject *obj,
                              JSObject **objp);

  
  
  
  static int32_t GetArrayIndexFromId(JSContext *cx, jsid id,
                                     bool *aIsNumber = nullptr);

  static inline bool IsReadonlyReplaceable(jsid id)
  {
    return (id == sParent_id       ||
            id == sScrollbars_id   ||
            id == sContent_id      ||
            id == sMenubar_id      ||
            id == sToolbar_id      ||
            id == sLocationbar_id  ||
            id == sPersonalbar_id  ||
            id == sStatusbar_id    ||
            id == sControllers_id  ||
            id == sScrollX_id      ||
            id == sScrollY_id      ||
            id == sScrollMaxX_id   ||
            id == sScrollMaxY_id   ||
            id == sLength_id       ||
            id == sFrames_id       ||
            id == sSelf_id         ||
            id == sURL_id);
  }

  static nsIXPConnect *sXPConnect;
  static nsIScriptSecurityManager *sSecMan;

  
  static nsresult DefineStaticJSVals(JSContext *cx);

  static bool sIsInitialized;
  static bool sDisableDocumentAllSupport;
  static bool sDisableGlobalScopePollutionSupport;

public:
  static jsid sParent_id;
  static jsid sScrollbars_id;
  static jsid sLocation_id;
  static jsid sConstructor_id;
  static jsid s_content_id;
  static jsid sContent_id;
  static jsid sMenubar_id;
  static jsid sToolbar_id;
  static jsid sLocationbar_id;
  static jsid sPersonalbar_id;
  static jsid sStatusbar_id;
  static jsid sDialogArguments_id;
  static jsid sControllers_id;
  static jsid sLength_id;
  static jsid sScrollX_id;
  static jsid sScrollY_id;
  static jsid sScrollMaxX_id;
  static jsid sScrollMaxY_id;
  static jsid sItem_id;
  static jsid sNamedItem_id;
  static jsid sEnumerate_id;
  static jsid sNavigator_id;
  static jsid sTop_id;
  static jsid sDocument_id;
  static jsid sFrames_id;
  static jsid sSelf_id;
  static jsid sAll_id;
  static jsid sTags_id;
  static jsid sDocumentURIObject_id;
  static jsid sJava_id;
  static jsid sPackages_id;
  static jsid sWrappedJSObject_id;
  static jsid sURL_id;
  static jsid sOnload_id;
  static jsid sOnerror_id;

protected:
  static JSPropertyOp sXPCNativeWrapperGetPropertyOp;
  static JSPropertyOp sXrayWrapperPropertyHolderGetPropertyOp;
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
  nsEventTargetSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsEventTargetSH()
  {
  }
public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

  virtual void PreserveWrapper(nsISupports *aNative);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsEventTargetSH(aData);
  }
};



class nsWindowSH : public nsDOMGenericSH
{
protected:
  nsWindowSH(nsDOMClassInfoData *aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsWindowSH()
  {
  }

  static nsresult GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                                JSObject *obj, jsid id, bool *did_resolve);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
#ifdef DEBUG
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj)
  {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryWrappedNative(wrapper));

    NS_ASSERTION(!sgo || sgo->GetGlobalJSObject() == nullptr,
                 "Multiple wrappers created for global object!");

    return NS_OK;
  }
  virtual uint32_t GetScriptableFlags()
  {
    return nsDOMGenericSH::GetScriptableFlags() |
           nsIXPCScriptable::WANT_POSTCREATE;
  }
#endif
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, bool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD Finalize(nsIXPConnectWrappedNative *wrapper, JSFreeOp *fop,
                      JSObject *obj);
  NS_IMETHOD OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, JSObject * *_retval);

  static JSBool GlobalScopePolluterNewResolve(JSContext *cx, JSHandleObject obj,
                                              JSHandleId id, unsigned flags,
                                              JSMutableHandleObject objp);
  static JSBool GlobalScopePolluterGetProperty(JSContext *cx, JSHandleObject obj,
                                               JSHandleId id, JSMutableHandleValue vp);
  static JSBool InvalidateGlobalScopePolluter(JSContext *cx, JSObject *obj);
  static nsresult InstallGlobalScopePolluter(JSContext *cx, JSObject *obj,
                                             nsIHTMLDocument *doc);
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsWindowSH(aData);
  }
};



class nsLocationSH : public nsDOMGenericSH
{
protected:
  nsLocationSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsLocationSH()
  {
  }

public:
  NS_IMETHOD CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, uint32_t mode,
                         jsval *vp, bool *_retval);

  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHODIMP AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, jsval *vp, bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsLocationSH(aData);
  }
};




class nsNavigatorSH : public nsDOMGenericSH
{
protected:
  nsNavigatorSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsNavigatorSH()
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNavigatorSH(aData);
  }
};




class nsNodeSH : public nsDOMGenericSH
{
protected:
  nsNodeSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsNodeSH()
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD GetFlags(uint32_t *aFlags);

  virtual void PreserveWrapper(nsISupports *aNative);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNodeSH(aData);
  }
};




class nsElementSH : public nsNodeSH
{
protected:
  nsElementSH(nsDOMClassInfoData* aData) : nsNodeSH(aData)
  {
  }

  virtual ~nsElementSH()
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj);
  NS_IMETHOD PostTransplant(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsElementSH(aData);
  }
};




class nsGenericArraySH : public nsDOMClassInfo
{
protected:
  nsGenericArraySH(nsDOMClassInfoData* aData) : nsDOMClassInfo(aData)
  {
  }

  virtual ~nsGenericArraySH()
  {
  }
  
public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, bool *_retval);
  
  virtual nsresult GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, uint32_t *length);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsGenericArraySH(aData);
  }
};




class nsArraySH : public nsGenericArraySH
{
protected:
  nsArraySH(nsDOMClassInfoData* aData) : nsGenericArraySH(aData)
  {
  }

  virtual ~nsArraySH()
  {
  }

  
  
  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

private:
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData);
};




class nsNamedArraySH : public nsArraySH
{
protected:
  nsNamedArraySH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsNamedArraySH()
  {
  }

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);

  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

private:
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData);
};




class nsNamedNodeMapSH : public nsNamedArraySH
{
protected:
  nsNamedNodeMapSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsNamedNodeMapSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNamedNodeMapSH(aData);
  }
};



class nsDocumentSH : public nsNodeSH
{
public:
  nsDocumentSH(nsDOMClassInfoData* aData) : nsNodeSH(aData)
  {
  }

  virtual ~nsDocumentSH()
  {
  }

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD GetFlags(uint32_t* aFlags);
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj);
  NS_IMETHOD  PostTransplant(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDocumentSH(aData);
  }
};




class nsHTMLDocumentSH : public nsDocumentSH
{
protected:
  nsHTMLDocumentSH(nsDOMClassInfoData* aData) : nsDocumentSH(aData)
  {
  }

  virtual ~nsHTMLDocumentSH()
  {
  }

  static JSBool GetDocumentAllNodeList(JSContext *cx, JSObject *obj,
                                       nsDocument *doc,
                                       nsContentList **nodeList);

public:
  static JSBool DocumentAllGetProperty(JSContext *cx, JSHandleObject obj, JSHandleId id,
                                       JSMutableHandleValue vp);
  static JSBool DocumentAllNewResolve(JSContext *cx, JSHandleObject obj, JSHandleId id,
                                      unsigned flags, JSMutableHandleObject objp);
  static void ReleaseDocument(JSFreeOp *fop, JSObject *obj);
  static JSBool CallToGetPropMapper(JSContext *cx, unsigned argc, jsval *vp);
  static JSBool DocumentAllHelperGetProperty(JSContext *cx, JSHandleObject obj,
                                             JSHandleId id, JSMutableHandleValue vp);
  static JSBool DocumentAllHelperNewResolve(JSContext *cx, JSHandleObject obj,
                                            JSHandleId id, unsigned flags,
                                            JSMutableHandleObject objp);
  static JSBool DocumentAllTagsNewResolve(JSContext *cx, JSHandleObject obj,
                                          JSHandleId id, unsigned flags,
                                          JSMutableHandleObject objp);

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLDocumentSH(aData);
  }
};




class nsHTMLFormElementSH : public nsElementSH
{
protected:
  nsHTMLFormElementSH(nsDOMClassInfoData* aData) : nsElementSH(aData)
  {
  }

  virtual ~nsHTMLFormElementSH()
  {
  }

  static nsresult FindNamedItem(nsIForm *aForm, jsid id,
                                nsISupports **aResult, nsWrapperCache **aCache);

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         bool *_retval);

  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj,
                          uint32_t enum_op, jsval *statep,
                          jsid *idp, bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLFormElementSH(aData);
  }
};




class nsHTMLSelectElementSH : public nsElementSH
{
protected:
  nsHTMLSelectElementSH(nsDOMClassInfoData* aData) : nsElementSH(aData)
  {
  }

  virtual ~nsHTMLSelectElementSH()
  {
  }

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         bool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

  static nsresult SetOption(JSContext *cx, jsval *vp, uint32_t aIndex,
                            nsIDOMHTMLOptionsCollection *aOptCollection);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLSelectElementSH(aData);
  }
};




class nsHTMLPluginObjElementSH : public nsElementSH
{
protected:
  nsHTMLPluginObjElementSH(nsDOMClassInfoData* aData)
    : nsElementSH(aData)
  {
  }

  virtual ~nsHTMLPluginObjElementSH()
  {
  }

  static nsresult GetPluginInstanceIfSafe(nsIXPConnectWrappedNative *aWrapper,
                                          JSObject *obj,
                                          JSContext *cx,
                                          nsNPAPIPluginInstance **aResult);

  static nsresult GetPluginJSObject(JSContext *cx, JSObject *obj,
                                    nsNPAPIPluginInstance *plugin_inst,
                                    JSObject **plugin_obj,
                                    JSObject **plugin_proto);

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj);
  NS_IMETHOD PostTransplant(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                  JSObject *obj, uint32_t argc, jsval *argv, jsval *vp,
                  bool *_retval);


  static nsresult SetupProtoChain(nsIXPConnectWrappedNative *wrapper,
                                  JSContext *cx, JSObject *obj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLPluginObjElementSH(aData);
  }
};




class nsPluginSH : public nsNamedArraySH
{
protected:
  nsPluginSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsPluginSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsPluginSH(aData);
  }
};




class nsPluginArraySH : public nsNamedArraySH
{
protected:
  nsPluginArraySH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsPluginArraySH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsPluginArraySH(aData);
  }
};




class nsMimeTypeArraySH : public nsNamedArraySH
{
protected:
  nsMimeTypeArraySH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsMimeTypeArraySH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsMimeTypeArraySH(aData);
  }
};




class nsStringArraySH : public nsGenericArraySH
{
protected:
  nsStringArraySH(nsDOMClassInfoData* aData) : nsGenericArraySH(aData)
  {
  }

  virtual ~nsStringArraySH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
};




class nsHistorySH : public nsStringArraySH
{
protected:
  nsHistorySH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsHistorySH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHistorySH(aData);
  }
};



class nsStringListSH : public nsStringArraySH
{
protected:
  nsStringListSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsStringListSH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult);

public:
  
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStringListSH(aData);
  }
};




class nsMediaListSH : public nsStringArraySH
{
protected:
  nsMediaListSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsMediaListSH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsMediaListSH(aData);
  }
};




class nsStyleSheetListSH : public nsArraySH
{
protected:
  nsStyleSheetListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsStyleSheetListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStyleSheetListSH(aData);
  }
};




class nsCSSRuleListSH : public nsArraySH
{
protected:
  nsCSSRuleListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsCSSRuleListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsCSSRuleListSH(aData);
  }
};

class nsDOMTouchListSH : public nsArraySH
{
  protected:
  nsDOMTouchListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsDOMTouchListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  public:
  static nsIClassInfo* doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDOMTouchListSH(aData);
  }
};

#ifdef MOZ_XUL


class nsTreeColumnsSH : public nsNamedArraySH
{
protected:
  nsTreeColumnsSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsTreeColumnsSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, uint32_t aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsTreeColumnsSH(aData);
  }
};
#endif



class nsStorage2SH : public nsDOMGenericSH
{
protected:
  nsStorage2SH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsStorage2SH()
  {
  }

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, bool *_retval);
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, uint32_t enum_op, jsval *statep,
                          jsid *idp, bool *_retval);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStorage2SH(aData);
  }
};








class nsEventListenerThisTranslator : public nsIXPCFunctionThisTranslator
{
public:
  nsEventListenerThisTranslator()
  {
  }

  virtual ~nsEventListenerThisTranslator()
  {
  }

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIXPCFUNCTIONTHISTRANSLATOR
};

class nsDOMConstructorSH : public nsDOMGenericSH
{
protected:
  nsDOMConstructorSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD PostCreatePrototype(JSContext * cx, JSObject * proto)
  {
    return NS_OK;
  }
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, uint32_t flags,
                        JSObject **objp, bool *_retval);
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                  JSObject *obj, uint32_t argc, jsval *argv, jsval *vp,
                  bool *_retval);

  NS_IMETHOD Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, uint32_t argc, jsval *argv,
                       jsval *vp, bool *_retval);

  NS_IMETHOD HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, const jsval &val, bool *bp,
                         bool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDOMConstructorSH(aData);
  }
};

class nsNonDOMObjectSH : public nsDOMGenericSH
{
protected:
  nsNonDOMObjectSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsNonDOMObjectSH()
  {
  }

public:
  NS_IMETHOD GetFlags(uint32_t *aFlags);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNonDOMObjectSH(aData);
  }
};


class nsAttributeSH : public nsNodeSH
{
protected:
  nsAttributeSH(nsDOMClassInfoData* aData) : nsNodeSH(aData)
  {
  }

  virtual ~nsAttributeSH()
  {
  }

public:
  NS_IMETHOD GetFlags(uint32_t *aFlags);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsAttributeSH(aData);
  }
};

class nsOfflineResourceListSH : public nsStringArraySH
{
protected:
  nsOfflineResourceListSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsOfflineResourceListSH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsOfflineResourceListSH(aData);
  }
};



class nsSVGStringListSH : public nsStringArraySH
{
protected:
  nsSVGStringListSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }
  
  virtual ~nsSVGStringListSH()
  {
  }
  
  virtual nsresult GetStringAt(nsISupports *aNative, int32_t aIndex,
                               nsAString& aResult);
  
public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsSVGStringListSH(aData);
  }
};

#endif
