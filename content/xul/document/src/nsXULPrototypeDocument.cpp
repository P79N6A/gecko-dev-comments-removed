









































#include "nsXULPrototypeDocument.h"
#include "nsXULDocument.h"

#include "nsAString.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIPrincipal.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptRuntime.h"
#include "nsIServiceManager.h"
#include "nsIArray.h"
#include "nsIURI.h"
#include "jsapi.h"
#include "nsString.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsDOMCID.h"
#include "nsNodeInfoManager.h"
#include "nsContentUtils.h"
#include "nsCCUncollectableMarker.h"
#include "nsDOMJSUtils.h" 

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);


class nsXULPDGlobalObject : public nsIScriptGlobalObject,
                            public nsIScriptObjectPrincipal
{
public:
    nsXULPDGlobalObject(nsXULPrototypeDocument* owner);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    
    virtual void OnFinalize(PRUint32 aLangID, void *aGlobal);
    virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);
    virtual nsresult SetNewArguments(nsIArray *aArguments);

    virtual void *GetScriptGlobal(PRUint32 lang);
    virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID);

    virtual nsIScriptContext *GetScriptContext(PRUint32 lang);
    virtual nsresult SetScriptContext(PRUint32 language, nsIScriptContext *ctx);

    
    virtual nsIPrincipal* GetPrincipal();

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULPDGlobalObject,
                                             nsIScriptGlobalObject)

    void ClearGlobalObjectOwner();

protected:
    virtual ~nsXULPDGlobalObject();

    nsXULPrototypeDocument* mGlobalObjectOwner; 

    nsCOMPtr<nsIScriptContext>  mScriptContexts[NS_STID_ARRAY_UBOUND];
    void *                      mScriptGlobals[NS_STID_ARRAY_UBOUND];

    nsCOMPtr<nsIPrincipal> mCachedPrincipal;

    static JSClass gSharedGlobalClass;
};

nsIPrincipal* nsXULPrototypeDocument::gSystemPrincipal;
nsXULPDGlobalObject* nsXULPrototypeDocument::gSystemGlobal;
PRUint32 nsXULPrototypeDocument::gRefCnt;


void
nsXULPDGlobalObject_finalize(JSContext *cx, JSObject *obj)
{
    nsISupports *nativeThis = (nsISupports*)JS_GetPrivate(cx, obj);

    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeThis));

    if (sgo) {
        sgo->OnFinalize(nsIProgrammingLanguage::JAVASCRIPT, obj);
    }

    
    NS_RELEASE(nativeThis);
}


JSBool
nsXULPDGlobalObject_resolve(JSContext *cx, JSObject *obj, jsval id)
{
    JSBool did_resolve = JS_FALSE;

    return JS_ResolveStandardClass(cx, obj, id, &did_resolve);
}


JSClass nsXULPDGlobalObject::gSharedGlobalClass = {
    "nsXULPrototypeScript compilation scope",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, nsXULPDGlobalObject_resolve,  JS_ConvertStub,
    nsXULPDGlobalObject_finalize
};








nsXULPrototypeDocument::nsXULPrototypeDocument()
    : mRoot(nsnull),
      mLoaded(PR_FALSE)
{
    ++gRefCnt;
}


nsresult
nsXULPrototypeDocument::Init()
{
    mNodeInfoManager = new nsNodeInfoManager();
    NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_OUT_OF_MEMORY);

    return mNodeInfoManager->Init(nsnull);
}

nsXULPrototypeDocument::~nsXULPrototypeDocument()
{
    if (mGlobalObject) {
        
        mGlobalObject->ClearGlobalObjectOwner();
    }

    if (mRoot)
        mRoot->ReleaseSubtree();

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gSystemPrincipal);
        NS_IF_RELEASE(gSystemGlobal);
    }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULPrototypeDocument)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsXULPrototypeDocument)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULPrototypeDocument)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mRoot,
                                                    nsXULPrototypeElement)
    cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObject*>(tmp->mGlobalObject));
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNodeInfoManager,
                                                    nsNodeInfoManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULPrototypeDocument)
    NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObjectOwner)
    NS_INTERFACE_MAP_ENTRY(nsISerializable)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObjectOwner)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXULPrototypeDocument,
                                          nsIScriptGlobalObjectOwner)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXULPrototypeDocument,
                                           nsIScriptGlobalObjectOwner)

NS_IMETHODIMP
NS_NewXULPrototypeDocument(nsXULPrototypeDocument** aResult)
{
    *aResult = new nsXULPrototypeDocument();
    if (! *aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    rv = (*aResult)->Init();
    if (NS_FAILED(rv)) {
        delete *aResult;
        *aResult = nsnull;
        return rv;
    }

    NS_ADDREF(*aResult);
    return rv;
}








nsXULPDGlobalObject *
nsXULPrototypeDocument::NewXULPDGlobalObject()
{
    
    
    
    nsXULPDGlobalObject *global;
    if (DocumentPrincipal() == gSystemPrincipal) {
        if (!gSystemGlobal) {
            gSystemGlobal = new nsXULPDGlobalObject(nsnull);
            if (! gSystemGlobal)
                return nsnull;
            NS_ADDREF(gSystemGlobal);
        }
        global = gSystemGlobal;
    } else {
        global = new nsXULPDGlobalObject(this); 
        if (! global)
            return nsnull;
    }
    return global;
}






NS_IMETHODIMP
nsXULPrototypeDocument::Read(nsIObjectInputStream* aStream)
{
    nsresult rv;

    rv = aStream->ReadObject(PR_TRUE, getter_AddRefs(mURI));

    PRUint32 count, i;
    nsCOMPtr<nsIURI> styleOverlayURI;

    rv |= aStream->Read32(&count);
    if (NS_FAILED(rv)) return rv;

    for (i = 0; i < count; ++i) {
        rv |= aStream->ReadObject(PR_TRUE, getter_AddRefs(styleOverlayURI));
        mStyleSheetReferences.AppendObject(styleOverlayURI);
    }


    
    nsCOMPtr<nsIPrincipal> principal;
    rv |= aStream->ReadObject(PR_TRUE, getter_AddRefs(principal));
    
    mNodeInfoManager->SetDocumentPrincipal(principal);


    
    mGlobalObject = NewXULPDGlobalObject();
    if (! mGlobalObject)
        return NS_ERROR_OUT_OF_MEMORY;

    mRoot = new nsXULPrototypeElement();
    if (! mRoot)
       return NS_ERROR_OUT_OF_MEMORY;

    
    nsCOMArray<nsINodeInfo> nodeInfos;

    rv |= aStream->Read32(&count);
    nsAutoString namespaceURI, prefixStr, localName;
    PRBool prefixIsNull;
    nsCOMPtr<nsIAtom> prefix;
    for (i = 0; i < count; ++i) {
        rv |= aStream->ReadString(namespaceURI);
        rv |= aStream->ReadBoolean(&prefixIsNull);
        if (prefixIsNull) {
            prefix = nsnull;
        } else {
            rv |= aStream->ReadString(prefixStr);
            prefix = do_GetAtom(prefixStr);
        }
        rv |= aStream->ReadString(localName);

        nsCOMPtr<nsINodeInfo> nodeInfo;
        rv |= mNodeInfoManager->GetNodeInfo(localName, prefix, namespaceURI,
                                            getter_AddRefs(nodeInfo));
        if (!nodeInfos.AppendObject(nodeInfo))
            rv |= NS_ERROR_OUT_OF_MEMORY;
    }

    
    PRUint32 type;
    while (NS_SUCCEEDED(rv)) {
        rv |= aStream->Read32(&type);

        if ((nsXULPrototypeNode::Type)type == nsXULPrototypeNode::eType_PI) {
            nsRefPtr<nsXULPrototypePI> pi = new nsXULPrototypePI();
            if (! pi) {
               rv |= NS_ERROR_OUT_OF_MEMORY;
               break;
            }

            rv |= pi->Deserialize(aStream, mGlobalObject, mURI, &nodeInfos);
            rv |= AddProcessingInstruction(pi);
        } else if ((nsXULPrototypeNode::Type)type == nsXULPrototypeNode::eType_Element) {
            rv |= mRoot->Deserialize(aStream, mGlobalObject, mURI, &nodeInfos);
            break;
        } else {
            NS_NOTREACHED("Unexpected prototype node type");
            rv |= NS_ERROR_FAILURE;
            break;
        }
    }
    rv |= NotifyLoadDone();

    return rv;
}

static nsresult
GetNodeInfos(nsXULPrototypeElement* aPrototype,
             nsCOMArray<nsINodeInfo>& aArray)
{
    nsresult rv;
    if (aArray.IndexOf(aPrototype->mNodeInfo) < 0) {
        if (!aArray.AppendObject(aPrototype->mNodeInfo)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    
    PRUint32 i;
    for (i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsCOMPtr<nsINodeInfo> ni;
        nsAttrName* name = &aPrototype->mAttributes[i].mName;
        if (name->IsAtom()) {
            ni = aPrototype->mNodeInfo->NodeInfoManager()->
                GetNodeInfo(name->Atom(), nsnull, kNameSpaceID_None);
            NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
            ni = name->NodeInfo();
        }

        if (aArray.IndexOf(ni) < 0) {
            if (!aArray.AppendObject(ni)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    
    for (i = 0; i < aPrototype->mChildren.Length(); ++i) {
        nsXULPrototypeNode* child = aPrototype->mChildren[i];
        if (child->mType == nsXULPrototypeNode::eType_Element) {
            rv = GetNodeInfos(static_cast<nsXULPrototypeElement*>(child),
                              aArray);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULPrototypeDocument::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv;

    rv = aStream->WriteCompoundObject(mURI, NS_GET_IID(nsIURI), PR_TRUE);
    
    PRUint32 count;

    count = mStyleSheetReferences.Count();
    rv |= aStream->Write32(count);

    PRUint32 i;
    for (i = 0; i < count; ++i) {
        rv |= aStream->WriteCompoundObject(mStyleSheetReferences[i],
                                           NS_GET_IID(nsIURI), PR_TRUE);
    }

    
    rv |= aStream->WriteObject(mNodeInfoManager->DocumentPrincipal(),
                               PR_TRUE);
    
    
    nsCOMArray<nsINodeInfo> nodeInfos;
    if (mRoot)
        rv |= GetNodeInfos(mRoot, nodeInfos);

    PRUint32 nodeInfoCount = nodeInfos.Count();
    rv |= aStream->Write32(nodeInfoCount);
    for (i = 0; i < nodeInfoCount; ++i) {
        nsINodeInfo *nodeInfo = nodeInfos[i];
        NS_ENSURE_TRUE(nodeInfo, NS_ERROR_FAILURE);

        nsAutoString namespaceURI;
        rv |= nodeInfo->GetNamespaceURI(namespaceURI);
        rv |= aStream->WriteWStringZ(namespaceURI.get());

        nsAutoString prefix;
        nodeInfo->GetPrefix(prefix);
        PRBool nullPrefix = DOMStringIsNull(prefix);
        rv |= aStream->WriteBoolean(nullPrefix);
        if (!nullPrefix) {
            rv |= aStream->WriteWStringZ(prefix.get());
        }

        nsAutoString localName;
        nodeInfo->GetName(localName);
        rv |= aStream->WriteWStringZ(localName.get());
    }

    
    nsIScriptGlobalObject* globalObject = GetScriptGlobalObject();
    NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

    count = mProcessingInstructions.Length();
    for (i = 0; i < count; ++i) {
        nsXULPrototypePI* pi = mProcessingInstructions[i];
        rv |= pi->Serialize(aStream, globalObject, &nodeInfos);
    }

    if (mRoot)
        rv |= mRoot->Serialize(aStream, globalObject, &nodeInfos);
 
    return rv;
}





nsresult
nsXULPrototypeDocument::InitPrincipal(nsIURI* aURI, nsIPrincipal* aPrincipal)
{
    NS_ENSURE_ARG_POINTER(aURI);

    mURI = aURI;
    mNodeInfoManager->SetDocumentPrincipal(aPrincipal);
    return NS_OK;
}
    

nsIURI*
nsXULPrototypeDocument::GetURI()
{
    NS_ASSERTION(mURI, "null URI");
    return mURI;
}


nsXULPrototypeElement*
nsXULPrototypeDocument::GetRootElement()
{
    return mRoot;
}


void
nsXULPrototypeDocument::SetRootElement(nsXULPrototypeElement* aElement)
{
    mRoot = aElement;
}

nsresult
nsXULPrototypeDocument::AddProcessingInstruction(nsXULPrototypePI* aPI)
{
    NS_PRECONDITION(aPI, "null ptr");
    if (!mProcessingInstructions.AppendElement(aPI)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

const nsTArray<nsRefPtr<nsXULPrototypePI> >&
nsXULPrototypeDocument::GetProcessingInstructions() const
{
    return mProcessingInstructions;
}

void
nsXULPrototypeDocument::AddStyleSheetReference(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "null ptr");
    if (!mStyleSheetReferences.AppendObject(aURI)) {
        NS_WARNING("mStyleSheetReferences->AppendElement() failed."
                   "Stylesheet overlay dropped.");
    }
}

const nsCOMArray<nsIURI>&
nsXULPrototypeDocument::GetStyleSheetReferences() const
{
    return mStyleSheetReferences;
}

NS_IMETHODIMP
nsXULPrototypeDocument::GetHeaderData(nsIAtom* aField, nsAString& aData) const
{
    
    aData.Truncate();
    return NS_OK;
}


NS_IMETHODIMP
nsXULPrototypeDocument::SetHeaderData(nsIAtom* aField, const nsAString& aData)
{
    
    return NS_OK;
}



nsIPrincipal*
nsXULPrototypeDocument::DocumentPrincipal()
{
    NS_PRECONDITION(mNodeInfoManager, "missing nodeInfoManager");
    return mNodeInfoManager->DocumentPrincipal();
}

void
nsXULPrototypeDocument::SetDocumentPrincipal(nsIPrincipal* aPrincipal)
{
    mNodeInfoManager->SetDocumentPrincipal(aPrincipal);
}

nsNodeInfoManager*
nsXULPrototypeDocument::GetNodeInfoManager()
{
    return mNodeInfoManager;
}


nsresult
nsXULPrototypeDocument::AwaitLoadDone(nsXULDocument* aDocument, PRBool* aResult)
{
    nsresult rv = NS_OK;

    *aResult = mLoaded;

    if (!mLoaded) {
        rv = mPrototypeWaiters.AppendElement(aDocument)
              ? NS_OK : NS_ERROR_OUT_OF_MEMORY; 
    }

    return rv;
}


nsresult
nsXULPrototypeDocument::NotifyLoadDone()
{
    
    
    
    

    nsresult rv = NS_OK;

    mLoaded = PR_TRUE;

    for (PRUint32 i = mPrototypeWaiters.Length(); i > 0; ) {
        --i;
        
        
        rv = mPrototypeWaiters[i]->OnPrototypeLoadDone(PR_TRUE);
        if (NS_FAILED(rv)) break;
    }
    mPrototypeWaiters.Clear();

    return rv;
}






nsIScriptGlobalObject*
nsXULPrototypeDocument::GetScriptGlobalObject()
{
    if (!mGlobalObject)
        mGlobalObject = NewXULPDGlobalObject();

    return mGlobalObject;
}






nsXULPDGlobalObject::nsXULPDGlobalObject(nsXULPrototypeDocument* owner)
    :  mGlobalObjectOwner(owner)
{
  memset(mScriptGlobals, 0, sizeof(mScriptGlobals));
}


nsXULPDGlobalObject::~nsXULPDGlobalObject()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULPDGlobalObject)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsXULPDGlobalObject)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULPDGlobalObject)
  {
    PRUint32 lang_index;
    NS_STID_FOR_INDEX(lang_index) {
      cb.NoteXPCOMChild(tmp->mScriptContexts[lang_index]);
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULPDGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObject)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXULPDGlobalObject,
                                          nsIScriptGlobalObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXULPDGlobalObject,
                                           nsIScriptGlobalObject)






nsresult
nsXULPDGlobalObject::SetScriptContext(PRUint32 lang_id, nsIScriptContext *aScriptContext)
{
  
  nsresult rv;

  PRBool ok = NS_STID_VALID(lang_id);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
  PRUint32 lang_ndx = NS_STID_INDEX(lang_id);

  if (!aScriptContext)
    NS_WARNING("Possibly early removal of script object, see bug #41608");
  else {
    
    aScriptContext->WillInitializeContext();
    
    
    rv = aScriptContext->InitContext(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!aScriptContext || !mScriptContexts[lang_ndx],
               "Bad call to SetContext()!");

  void *script_glob = nsnull;

  if (aScriptContext) {
    aScriptContext->SetGCOnDestruction(PR_FALSE);
    aScriptContext->DidInitializeContext();
    script_glob = aScriptContext->GetNativeGlobal();
    NS_ASSERTION(script_glob, "GetNativeGlobal returned NULL!");
  }
  mScriptContexts[lang_ndx] = aScriptContext;
  mScriptGlobals[lang_ndx] = script_glob;
  return NS_OK;
}

nsresult
nsXULPDGlobalObject::EnsureScriptEnvironment(PRUint32 lang_id)
{
  PRBool ok = NS_STID_VALID(lang_id);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, NS_ERROR_INVALID_ARG);
  PRUint32 lang_ndx = NS_STID_INDEX(lang_id);

  if (mScriptContexts[lang_ndx] == nsnull) {
    nsresult rv;
    NS_ASSERTION(mScriptGlobals[lang_ndx] == nsnull, "Have global without context?");

    nsCOMPtr<nsIScriptRuntime> languageRuntime;
    rv = NS_GetScriptRuntimeByID(lang_id, getter_AddRefs(languageRuntime));
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsCOMPtr<nsIScriptContext> ctxNew;
    rv = languageRuntime->CreateContext(getter_AddRefs(ctxNew));
    
    
    
    if (lang_id == nsIProgrammingLanguage::JAVASCRIPT) {
      
      JSContext *cx = (JSContext *)ctxNew->GetNativeContext();
      JSAutoRequest ar(cx);
      JSObject *newGlob = ::JS_NewObject(cx, &gSharedGlobalClass, nsnull, nsnull);
      if (!newGlob)
        return nsnull;

      ::JS_SetGlobalObject(cx, newGlob);

      
      
      ::JS_SetPrivate(cx, newGlob, this);
      NS_ADDREF(this);
    }

    NS_ENSURE_SUCCESS(rv, nsnull);
    rv = SetScriptContext(lang_id, ctxNew);
    NS_ENSURE_SUCCESS(rv, nsnull);
  }
  return NS_OK;
}

nsIScriptContext *
nsXULPDGlobalObject::GetScriptContext(PRUint32 lang_id)
{
  
  nsresult rv = EnsureScriptEnvironment(lang_id);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to setup script language");
    return nsnull;
  }
  
  return mScriptContexts[NS_STID_INDEX(lang_id)];
}

void *
nsXULPDGlobalObject::GetScriptGlobal(PRUint32 lang_id)
{
  PRBool ok = NS_STID_VALID(lang_id);
  NS_ASSERTION(ok, "Invalid programming language ID requested");
  NS_ENSURE_TRUE(ok, nsnull);
  PRUint32 lang_ndx = NS_STID_INDEX(lang_id);

  NS_ASSERTION(mScriptContexts[lang_ndx] != nsnull, "Querying for global before setting up context?");
  return mScriptGlobals[lang_ndx];
}


void
nsXULPDGlobalObject::ClearGlobalObjectOwner()
{
    NS_ASSERTION(!mCachedPrincipal, "This shouldn't ever be set until now!");

    
    if (this != nsXULPrototypeDocument::gSystemGlobal)
        mCachedPrincipal = mGlobalObjectOwner->DocumentPrincipal();

    PRUint32 lang_ndx;
    NS_STID_FOR_INDEX(lang_ndx) {
        if (mScriptContexts[lang_ndx]) {
            mScriptContexts[lang_ndx]->FinalizeContext();
            mScriptContexts[lang_ndx] = nsnull;
        }
    }

    mGlobalObjectOwner = nsnull;
}


void
nsXULPDGlobalObject::OnFinalize(PRUint32 aLangID, void *aObject)
{
    NS_ASSERTION(NS_STID_VALID(aLangID), "Invalid language ID");
    NS_ASSERTION(aObject == mScriptGlobals[NS_STID_INDEX(aLangID)],
                 "Wrong object finalized!");
    mScriptGlobals[NS_STID_INDEX(aLangID)] = nsnull;
}

void
nsXULPDGlobalObject::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
    
}

nsresult
nsXULPDGlobalObject::SetNewArguments(nsIArray *aArguments)
{
    NS_NOTREACHED("waaah!");
    return NS_ERROR_UNEXPECTED;
}






nsIPrincipal*
nsXULPDGlobalObject::GetPrincipal()
{
    if (!mGlobalObjectOwner) {
        
        
        if (this == nsXULPrototypeDocument::gSystemGlobal) {
            return nsXULPrototypeDocument::gSystemPrincipal;
        }
        
        return mCachedPrincipal;
    }

    return mGlobalObjectOwner->DocumentPrincipal();
}
