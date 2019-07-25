






































#ifndef nsDOMClassInfo_h___
#define nsDOMClassInfo_h___

#include "nsIDOMClassInfo.h"
#include "nsIXPCScriptable.h"
#include "jsapi.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptContext.h"
#include "nsDOMJSUtils.h" 
#include "nsIScriptGlobalObject.h"
#include "nsContentUtils.h"

namespace mozilla {
class DOMSVGLengthList;
class DOMSVGNumberList;
class DOMSVGPathSegList;
class DOMSVGPointList;
}
class nsGlobalWindow;
class nsIDOMDocument;
class nsIDOMNSHTMLOptionCollection;
class nsIDOMNodeList;
class nsIDOMSVGLength;
class nsIDOMSVGLengthList;
class nsIDOMSVGNumber;
class nsIDOMSVGNumberList;
class nsIDOMSVGPathSeg;
class nsIDOMSVGPathSegList;
class nsIDOMSVGPoint;
class nsIDOMSVGPointList;
class nsIDOMSVGTransform;
class nsIDOMSVGTransformList;
class nsIDOMWindow;
class nsIForm;
class nsIHTMLDocument;
class nsNPAPIPluginInstance;
class nsSVGTransformList;

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
  PRUint32 mScriptableFlags : 31; 
  PRUint32 mHasClassInterface : 1;
  PRUint32 mInterfacesBitmap;
  PRPackedBool mChromeOnly;
  PRPackedBool mDisabled;
#ifdef NS_DEBUG
  PRUint32 mDebugID;
#endif
};

struct nsExternalDOMClassInfoData : public nsDOMClassInfoData
{
  const nsCID *mConstructorCID;
};


typedef PRUptrdiff PtrBits;




#define GET_CLEAN_CI_PTR(_ptr) (nsIClassInfo*)(PtrBits(_ptr) & ~0x1)
#define MARK_EXTERNAL(_ptr) (nsIClassInfo*)(PtrBits(_ptr) | 0x1)
#define IS_EXTERNAL(_ptr) (PtrBits(_ptr) & 0x1)


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

  static nsresult ThrowJSException(JSContext *cx, nsresult aResult);

  










  static PRBool ObjectIsNativeWrapper(JSContext* cx, JSObject* obj);

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

  virtual PRUint32 GetInterfacesBitmap()
  {
    return mData->mInterfacesBitmap;
  }

  static nsresult Init();
  static nsresult RegisterClassName(PRInt32 aDOMClassInfoID);
  static nsresult RegisterClassProtos(PRInt32 aDOMClassInfoID);
  static nsresult RegisterExternalClasses();
  nsresult ResolveConstructor(JSContext *cx, JSObject *obj,
                              JSObject **objp);

  
  
  
  static PRInt32 GetArrayIndexFromId(JSContext *cx, jsid id,
                                     PRBool *aIsNumber = nsnull);

  static inline PRBool IsReadonlyReplaceable(jsid id)
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

  static inline PRBool IsWritableReplaceable(jsid id)
  {
    return (id == sInnerHeight_id  ||
            id == sInnerWidth_id   ||
            id == sOpener_id       ||
            id == sOuterHeight_id  ||
            id == sOuterWidth_id   ||
            id == sScreenX_id      ||
            id == sScreenY_id      ||
            id == sStatus_id       ||
            id == sName_id);
  }

  static nsIXPConnect *sXPConnect;
  static nsIScriptSecurityManager *sSecMan;

  
  static nsresult DefineStaticJSVals(JSContext *cx);

  static PRBool sIsInitialized;
  static PRBool sDisableDocumentAllSupport;
  static PRBool sDisableGlobalScopePollutionSupport;

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
  static jsid sInnerHeight_id;
  static jsid sInnerWidth_id;
  static jsid sOuterHeight_id;
  static jsid sOuterWidth_id;
  static jsid sScreenX_id;
  static jsid sScreenY_id;
  static jsid sStatus_id;
  static jsid sName_id;
  static jsid sOnmousedown_id;
  static jsid sOnmouseup_id;
  static jsid sOnclick_id;
  static jsid sOndblclick_id;
  static jsid sOncontextmenu_id;
  static jsid sOnmouseover_id;
  static jsid sOnmouseout_id;
  static jsid sOnkeydown_id;
  static jsid sOnkeyup_id;
  static jsid sOnkeypress_id;
  static jsid sOnmousemove_id;
  static jsid sOnfocus_id;
  static jsid sOnblur_id;
  static jsid sOnsubmit_id;
  static jsid sOnreset_id;
  static jsid sOnchange_id;
  static jsid sOninput_id;
  static jsid sOninvalid_id;
  static jsid sOnselect_id;
  static jsid sOnload_id;
  static jsid sOnpopstate_id;
  static jsid sOnbeforeunload_id;
  static jsid sOnunload_id;
  static jsid sOnhashchange_id;
  static jsid sOnreadystatechange_id;
  static jsid sOnpageshow_id;
  static jsid sOnpagehide_id;
  static jsid sOnabort_id;
  static jsid sOnerror_id;
  static jsid sOnpaint_id;
  static jsid sOnresize_id;
  static jsid sOnscroll_id;
  static jsid sOndrag_id;
  static jsid sOndragend_id;
  static jsid sOndragenter_id;
  static jsid sOndragleave_id;
  static jsid sOndragover_id;
  static jsid sOndragstart_id;
  static jsid sOndrop_id;
  static jsid sScrollX_id;
  static jsid sScrollY_id;
  static jsid sScrollMaxX_id;
  static jsid sScrollMaxY_id;
  static jsid sOpen_id;
  static jsid sItem_id;
  static jsid sNamedItem_id;
  static jsid sEnumerate_id;
  static jsid sNavigator_id;
  static jsid sDocument_id;
  static jsid sFrames_id;
  static jsid sSelf_id;
  static jsid sOpener_id;
  static jsid sAll_id;
  static jsid sTags_id;
  static jsid sAddEventListener_id;
  static jsid sBaseURIObject_id;
  static jsid sNodePrincipal_id;
  static jsid sDocumentURIObject_id;
  static jsid sOncopy_id;
  static jsid sOncut_id;
  static jsid sOnpaste_id;
  static jsid sJava_id;
  static jsid sPackages_id;
  static jsid sOnloadstart_id;
  static jsid sOnprogress_id;
  static jsid sOnsuspend_id;
  static jsid sOnemptied_id;
  static jsid sOnstalled_id;
  static jsid sOnplay_id;
  static jsid sOnpause_id;
  static jsid sOnloadedmetadata_id;
  static jsid sOnloadeddata_id;
  static jsid sOnwaiting_id;
  static jsid sOnplaying_id;
  static jsid sOncanplay_id;
  static jsid sOncanplaythrough_id;
  static jsid sOnseeking_id;
  static jsid sOnseeked_id;
  static jsid sOntimeupdate_id;
  static jsid sOnended_id;
  static jsid sOnratechange_id;
  static jsid sOndurationchange_id;
  static jsid sOnvolumechange_id;
  static jsid sOnmessage_id;
  static jsid sOnbeforescriptexecute_id;
  static jsid sOnafterscriptexecute_id;
  static jsid sWrappedJSObject_id;
  static jsid sURL_id;
  static jsid sKeyPath_id;
  static jsid sAutoIncrement_id;
  static jsid sUnique_id;
  
  static jsid sOntouchstart_id;
  static jsid sOntouchend_id;
  static jsid sOntouchmove_id;
  static jsid sOntouchenter_id;
  static jsid sOntouchleave_id;
  static jsid sOntouchcancel_id;
  static jsid sOnbeforeprint_id;
  static jsid sOnafterprint_id;

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





class nsEventReceiverSH : public nsDOMGenericSH
{
protected:
  nsEventReceiverSH(nsDOMClassInfoData* aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsEventReceiverSH()
  {
  }

  static PRBool ReallyIsEventName(jsid id, jschar aFirstChar);

  static inline PRBool IsEventName(jsid id)
  {
    NS_ASSERTION(JSID_IS_STRING(id), "Don't pass non-string jsid's here!");

    const jschar *str = ::JS_GetInternedStringChars(JSID_TO_STRING(id));

    if (str[0] == 'o' && str[1] == 'n') {
      return ReallyIsEventName(id, str[2]);
    }

    return PR_FALSE;
  }

  nsresult RegisterCompileHandler(nsIXPConnectWrappedNative *wrapper,
                                  JSContext *cx, JSObject *obj, jsid id,
                                  PRBool compile, PRBool remove,
                                  PRBool *did_define);

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         PRBool *_retval);
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
};



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
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

  virtual void PreserveWrapper(nsISupports *aNative);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsEventTargetSH(aData);
  }
};



class nsWindowSH : public nsEventReceiverSH
{
protected:
  nsWindowSH(nsDOMClassInfoData *aData) : nsEventReceiverSH(aData)
  {
  }

  virtual ~nsWindowSH()
  {
  }

  static nsresult GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                                JSObject *obj, jsid id, PRBool *did_resolve);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
#ifdef DEBUG
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj)
  {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryWrappedNative(wrapper));

    NS_ASSERTION(!sgo || sgo->GetGlobalJSObject() == nsnull,
                 "Multiple wrappers created for global object!");

    return NS_OK;
  }
  NS_IMETHOD GetScriptableFlags(PRUint32 *aFlags)
  {
    PRUint32 flags;
    nsresult rv = nsEventReceiverSH::GetScriptableFlags(&flags);
    if (NS_SUCCEEDED(rv)) {
      *aFlags = flags | nsIXPCScriptable::WANT_POSTCREATE;
    }

    return rv;
  }
#endif
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, PRBool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD Finalize(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                      JSObject *obj);
  NS_IMETHOD Equality(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                      JSObject * obj, const jsval &val, PRBool *bp);
  NS_IMETHOD OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, JSObject * *_retval);

  static JSBool GlobalScopePolluterNewResolve(JSContext *cx, JSObject *obj,
                                              jsid id, uintN flags,
                                              JSObject **objp);
  static JSBool GlobalScopePolluterGetProperty(JSContext *cx, JSObject *obj,
                                               jsid id, jsval *vp);
  static JSBool SecurityCheckOnAddDelProp(JSContext *cx, JSObject *obj, jsid id,
                                          jsval *vp);
  static JSBool SecurityCheckOnSetProp(JSContext *cx, JSObject *obj, jsid id,
                                       JSBool strict, jsval *vp);
  static void InvalidateGlobalScopePolluter(JSContext *cx, JSObject *obj);
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
                         JSObject *obj, jsid id, PRUint32 mode,
                         jsval *vp, PRBool *_retval);

  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

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

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNavigatorSH(aData);
  }
};





class nsNodeSH : public nsEventReceiverSH
{
protected:
  nsNodeSH(nsDOMClassInfoData* aData) : nsEventReceiverSH(aData)
  {
  }

  virtual ~nsNodeSH()
  {
  }

  
  PRBool IsCapabilityEnabled(const char* aCapability);

  inline PRBool IsPrivilegedScript() {
    return IsCapabilityEnabled("UniversalXPConnect");
  }

  
  
  
  
  nsresult DefineVoidProp(JSContext* cx, JSObject* obj, jsid id,
                          JSObject** objp);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD GetFlags(PRUint32 *aFlags);

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
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, PRBool *_retval);

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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, PRBool *_retval);
  
  virtual nsresult GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, PRUint32 *length);

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

  
  
  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

private:
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData);
};



 
class nsNodeListSH : public nsArraySH
{
protected:
  nsNodeListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

  virtual nsresult GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, PRUint32 *length);
  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);
 
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsNodeListSH(aData);
  }
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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);

  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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




class nsHTMLCollectionSH : public nsNamedArraySH
{
protected:
  nsHTMLCollectionSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsHTMLCollectionSH()
  {
  }

  virtual nsresult GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, PRUint32 *length);
  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLCollectionSH(aData);
  }
};




class nsContentListSH : public nsNamedArraySH
{
protected:
  nsContentListSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

  virtual nsresult GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, PRUint32 *length);
  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsContentListSH(aData);
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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD GetFlags(PRUint32* aFlags);
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
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

  static JSBool DocumentOpen(JSContext *cx, uintN argc, jsval *vp);
  static JSBool GetDocumentAllNodeList(JSContext *cx, JSObject *obj,
                                       nsDocument *doc,
                                       nsContentList **nodeList);

public:
  static JSBool DocumentAllGetProperty(JSContext *cx, JSObject *obj, jsid id,
                                       jsval *vp);
  static JSBool DocumentAllNewResolve(JSContext *cx, JSObject *obj, jsid id,
                                      uintN flags, JSObject **objp);
  static void ReleaseDocument(JSContext *cx, JSObject *obj);
  static JSBool CallToGetPropMapper(JSContext *cx, uintN argc, jsval *vp);
  static JSBool DocumentAllHelperGetProperty(JSContext *cx, JSObject *obj,
                                             jsid id, jsval *vp);
  static JSBool DocumentAllHelperNewResolve(JSContext *cx, JSObject *obj,
                                            jsid id, uintN flags,
                                            JSObject **objp);
  static JSBool DocumentAllTagsNewResolve(JSContext *cx, JSObject *obj,
                                          jsid id, uintN flags,
                                          JSObject **objp);

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLDocumentSH(aData);
  }
};




class nsHTMLBodyElementSH : public nsElementSH
{
protected:
  nsHTMLBodyElementSH(nsDOMClassInfoData* aData) : nsElementSH(aData)
  {
  }

  virtual ~nsHTMLBodyElementSH()
  {
  }

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);

  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         PRBool *_retval);

  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLBodyElementSH(aData);
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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         PRBool *_retval);

  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj,
                          PRUint32 enum_op, jsval *statep,
                          jsid *idp, PRBool *_retval);

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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp,
                         PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

  static nsresult SetOption(JSContext *cx, jsval *vp, PRUint32 aIndex,
                            nsIDOMNSHTMLOptionCollection *aOptCollection);

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
                                          nsNPAPIPluginInstance **aResult);

  static nsresult GetPluginJSObject(JSContext *cx, JSObject *obj,
                                    nsNPAPIPluginInstance *plugin_inst,
                                    JSObject **plugin_obj,
                                    JSObject **plugin_proto);

public:
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                  JSObject *obj, PRUint32 argc, jsval *argv, jsval *vp,
                  PRBool *_retval);


  static nsresult SetupProtoChain(nsIXPConnectWrappedNative *wrapper,
                                  JSContext *cx, JSObject *obj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLPluginObjElementSH(aData);
  }
};




class nsHTMLOptionsCollectionSH : public nsHTMLCollectionSH
{
protected:
  nsHTMLOptionsCollectionSH(nsDOMClassInfoData* aData)
    : nsHTMLCollectionSH(aData)
  {
  }

  virtual ~nsHTMLOptionsCollectionSH()
  {
  }

public:
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsHTMLOptionsCollectionSH(aData);
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult) = 0;

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
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

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);

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

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult);

public:
  
  
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStringListSH(aData);
  }
};




class nsDOMTokenListSH : public nsStringArraySH
{
protected:
  nsDOMTokenListSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsDOMTokenListSH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult);

public:

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsDOMTokenListSH(aData);
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

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStyleSheetListSH(aData);
  }
};




class nsCSSValueListSH : public nsArraySH
{
protected:
  nsCSSValueListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsCSSValueListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsCSSValueListSH(aData);
  }
};




class nsCSSStyleDeclSH : public nsStringArraySH
{
protected:
  nsCSSStyleDeclSH(nsDOMClassInfoData* aData) : nsStringArraySH(aData)
  {
  }

  virtual ~nsCSSStyleDeclSH()
  {
  }

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsCSSStyleDeclSH(aData);
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsCSSRuleListSH(aData);
  }
};



class nsClientRectListSH : public nsArraySH
{
protected:
  nsClientRectListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsClientRectListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsClientRectListSH(aData);
  }
};




class nsPaintRequestListSH : public nsArraySH
{
protected:
  nsPaintRequestListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

  virtual ~nsPaintRequestListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsPaintRequestListSH(aData);
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
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



class nsStorageSH : public nsNamedArraySH
{
protected:
  nsStorageSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsStorageSH()
  {
  }

  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRUint32 enum_op, jsval *statep,
                          jsid *idp, PRBool *_retval);

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult)
  {
    return nsnull;
  }
  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStorageSH(aData);
  }
};


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
                        JSObject *obj, jsid id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid id, jsval *vp, PRBool *_retval);
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRUint32 enum_op, jsval *statep,
                          jsid *idp, PRBool *_retval);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStorage2SH(aData);
  }
};

class nsStorageListSH : public nsNamedArraySH
{
protected:
  nsStorageListSH(nsDOMClassInfoData* aData) : nsNamedArraySH(aData)
  {
  }

  virtual ~nsStorageListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult)
  {
    return nsnull;
  }
  
  virtual nsISupports* GetNamedItem(nsISupports *aNative,
                                    const nsAString& aName,
                                    nsWrapperCache **cache,
                                    nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsStorageListSH(aData);
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
  NS_IMETHOD Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                  JSObject *obj, PRUint32 argc, jsval *argv, jsval *vp,
                  PRBool *_retval);

  NS_IMETHOD Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, PRUint32 argc, jsval *argv,
                       jsval *vp, PRBool *_retval);

  NS_IMETHOD HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, const jsval &val, PRBool *bp,
                         PRBool *_retval);

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
  NS_IMETHOD GetFlags(PRUint32 *aFlags);

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
  NS_IMETHOD GetFlags(PRUint32 *aFlags);

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

  virtual nsresult GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                               nsAString& aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsOfflineResourceListSH(aData);
  }
};

class nsFileListSH : public nsArraySH
{
protected:
  nsFileListSH(nsDOMClassInfoData *aData) : nsArraySH(aData)
  {
  }

  virtual ~nsFileListSH()
  {
  }

  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);

public:
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsFileListSH(aData);
  }
};

class nsWebGLViewportHandlerSH : public nsDOMGenericSH
{
protected:
  nsWebGLViewportHandlerSH(nsDOMClassInfoData *aData) : nsDOMGenericSH(aData)
  {
  }

  virtual ~nsWebGLViewportHandlerSH()
  {
  }

public:
  NS_IMETHOD PostCreatePrototype(JSContext * cx, JSObject * proto) {
    nsresult rv = nsDOMGenericSH::PostCreatePrototype(cx, proto);
    if (NS_SUCCEEDED(rv)) {
      if (!::JS_DefineProperty(cx, proto, "VIEWPORT", INT_TO_JSVAL(0x0BA2),
                               nsnull, nsnull, JSPROP_ENUMERATE))
      {
        return NS_ERROR_UNEXPECTED;
      }
    }
    return rv;
  }

  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsWebGLViewportHandlerSH(aData);
  }
};



 
template<class ListInterfaceType, class ListType>
class nsSVGListSH : public nsArraySH
{
protected:
  nsSVGListSH(nsDOMClassInfoData* aData) : nsArraySH(aData)
  {
  }

public:
  virtual nsISupports* GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                                 nsWrapperCache **aCache, nsresult *aResult);
 
  static nsIClassInfo *doCreate(nsDOMClassInfoData* aData)
  {
    return new nsSVGListSH(aData);
  }
};

typedef nsSVGListSH<nsIDOMSVGLengthList, mozilla::DOMSVGLengthList> nsSVGLengthListSH;
typedef nsSVGListSH<nsIDOMSVGNumberList, mozilla::DOMSVGNumberList> nsSVGNumberListSH;
typedef nsSVGListSH<nsIDOMSVGPathSegList, mozilla::DOMSVGPathSegList> nsSVGPathSegListSH;
typedef nsSVGListSH<nsIDOMSVGPointList, mozilla::DOMSVGPointList> nsSVGPointListSH;

#endif
