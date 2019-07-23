















































#ifndef nsXULElement_h__
#define nsXULElement_h__


#include "nsIDOMEvent.h"
#include "nsIServiceManager.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsIControllers.h"
#include "nsICSSParser.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIEventListenerManager.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFResource.h"
#include "nsIScriptObjectOwner.h"
#include "nsBindingManager.h"
#include "nsIURI.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIBoxObject.h"
#include "nsIXBLService.h"
#include "nsLayoutCID.h"
#include "nsAttrAndChildArray.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsGenericElement.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsIFrameLoader.h"

class nsIDocument;
class nsString;
class nsIDocShell;
class nsICSSStyleRule;

class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIScriptGlobalObjectOwner;
class nsXULPrototypeNode;
typedef nsTArray<nsRefPtr<nsXULPrototypeNode> > nsPrototypeArray;

static NS_DEFINE_CID(kCSSParserCID, NS_CSSPARSER_CID);



#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
#define XUL_PROTOTYPE_ATTRIBUTE_METER(counter) (nsXULPrototypeAttribute::counter++)
#else
#define XUL_PROTOTYPE_ATTRIBUTE_METER(counter) ((void) 0)
#endif








class nsXULPrototypeAttribute
{
public:
    nsXULPrototypeAttribute()
        : mName(nsGkAtoms::id),  
          mEventHandler(nsnull)
    {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumAttributes);
        MOZ_COUNT_CTOR(nsXULPrototypeAttribute);
    }

    ~nsXULPrototypeAttribute();

    nsAttrName mName;
    nsAttrValue mValue;
    
    
    
    void* mEventHandler;

#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
    





































    static PRUint32   gNumElements;
    static PRUint32   gNumAttributes;
    static PRUint32   gNumEventHandlers;
    static PRUint32   gNumCacheTests;
    static PRUint32   gNumCacheHits;
    static PRUint32   gNumCacheSets;
    static PRUint32   gNumCacheFills;
#endif 
};











class nsXULPrototypeNode
{
public:
    enum Type { eType_Element, eType_Script, eType_Text, eType_PI };

    Type                     mType;

    nsAutoRefCnt             mRefCnt;

    virtual ~nsXULPrototypeNode() {}
    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsIScriptGlobalObject* aGlobal,
                               const nsCOMArray<nsINodeInfo> *aNodeInfos) = 0;
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 nsIURI* aDocumentURI,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos) = 0;

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() = 0;
    virtual PRUint32 ClassSize() = 0;
#endif

    void AddRef() {
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, ClassName(), ClassSize());
    }
    void Release()
    {
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, ClassName());
        if (mRefCnt == 0)
            delete this;
    }
    







    virtual void ReleaseSubtree() { }

    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsXULPrototypeNode)

protected:
    nsXULPrototypeNode(Type aType)
        : mType(aType) {}
};

class nsXULPrototypeElement : public nsXULPrototypeNode
{
public:
    nsXULPrototypeElement()
        : nsXULPrototypeNode(eType_Element),
          mNumAttributes(0),
          mAttributes(nsnull),
          mHasIdAttribute(PR_FALSE),
          mHasClassAttribute(PR_FALSE),
          mHasStyleAttribute(PR_FALSE),
          mHoldsScriptObject(PR_FALSE),
          mScriptTypeID(nsIProgrammingLanguage::UNKNOWN)
    {
    }

    virtual ~nsXULPrototypeElement()
    {
        UnlinkJSObjects();
        Unlink();
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypeElement"; }
    virtual PRUint32 ClassSize() { return sizeof(*this); }
#endif

    virtual void ReleaseSubtree()
    {
        for (PRInt32 i = mChildren.Length() - 1; i >= 0; i--) {
            if (mChildren[i].get())
                mChildren[i]->ReleaseSubtree();
        }
        mChildren.Clear();
        nsXULPrototypeNode::ReleaseSubtree();
    }

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsIScriptGlobalObject* aGlobal,
                               const nsCOMArray<nsINodeInfo> *aNodeInfos);
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 nsIURI* aDocumentURI,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos);

    nsresult SetAttrAt(PRUint32 aPos, const nsAString& aValue, nsIURI* aDocumentURI);

    void UnlinkJSObjects();
    void Unlink();

    nsPrototypeArray         mChildren;

    nsCOMPtr<nsINodeInfo>    mNodeInfo;           

    PRUint32                 mNumAttributes;
    nsXULPrototypeAttribute* mAttributes;         
    
    PRPackedBool             mHasIdAttribute:1;
    PRPackedBool             mHasClassAttribute:1;
    PRPackedBool             mHasStyleAttribute:1;
    PRPackedBool             mHoldsScriptObject:1;

    
    
    
    
    PRUint16                 mScriptTypeID;
    static void ReleaseGlobals()
    {
        NS_IF_RELEASE(sCSSParser);
    }

protected:
    static nsICSSParser* GetCSSParser()
    {
        if (!sCSSParser) {
            CallCreateInstance(kCSSParserCID, &sCSSParser);
            if (sCSSParser) {
                sCSSParser->SetQuirkMode(PR_FALSE);
            }
        }
        return sCSSParser;
    }
    static nsICSSParser* sCSSParser;
};

class nsXULDocument;

class nsXULPrototypeScript : public nsXULPrototypeNode
{
public:
    nsXULPrototypeScript(PRUint32 aLangID, PRUint32 aLineNo, PRUint32 version);
    virtual ~nsXULPrototypeScript();

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypeScript"; }
    virtual PRUint32 ClassSize() { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsIScriptGlobalObject* aGlobal,
                               const nsCOMArray<nsINodeInfo> *aNodeInfos);
    nsresult SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                nsIScriptGlobalObject* aGlobal);
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 nsIURI* aDocumentURI,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos);
    nsresult DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                  nsIScriptGlobalObject* aGlobal);

    nsresult Compile(const PRUnichar* aText, PRInt32 aTextLength,
                     nsIURI* aURI, PRUint32 aLineNo,
                     nsIDocument* aDocument,
                     nsIScriptGlobalObjectOwner* aGlobalOwner);

    void UnlinkJSObjects()
    {
        if (mScriptObject.mObject) {
            nsContentUtils::DropScriptObjects(mScriptObject.mLangID, this,
                                              &NS_CYCLE_COLLECTION_NAME(nsXULPrototypeNode));
            mScriptObject.mObject = nsnull;
        }
    }

    void Set(nsScriptObjectHolder &aHolder)
    {
        NS_ASSERTION(mScriptObject.mLangID == aHolder.getScriptTypeID(),
                     "Wrong language, this will leak the previous object.");

        mScriptObject.mLangID = aHolder.getScriptTypeID();
        Set((void*)aHolder);
    }
    void Set(void *aObject)
    {
        NS_ASSERTION(!mScriptObject.mObject, "Leaking script object.");

        nsresult rv = nsContentUtils::HoldScriptObject(mScriptObject.mLangID,
                                                       this,
                                                       &NS_CYCLE_COLLECTION_NAME(nsXULPrototypeNode),
                                                       aObject, PR_FALSE);
        if (NS_SUCCEEDED(rv)) {
            mScriptObject.mObject = aObject;
        }
    }

    struct ScriptObjectHolder
    {
        ScriptObjectHolder(PRUint32 aLangID) : mLangID(aLangID),
                                               mObject(nsnull)
        {
        }
        PRUint32 mLangID;
        void* mObject;
    };
    nsCOMPtr<nsIURI>         mSrcURI;
    PRUint32                 mLineNo;
    PRPackedBool             mSrcLoading;
    PRPackedBool             mOutOfLine;
    nsXULDocument*           mSrcLoadWaiters;   
    PRUint32                 mLangVersion;
    ScriptObjectHolder       mScriptObject;
};

class nsXULPrototypeText : public nsXULPrototypeNode
{
public:
    nsXULPrototypeText()
        : nsXULPrototypeNode(eType_Text)
    {
    }

    virtual ~nsXULPrototypeText()
    {
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypeText"; }
    virtual PRUint32 ClassSize() { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsIScriptGlobalObject* aGlobal,
                               const nsCOMArray<nsINodeInfo> *aNodeInfos);
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 nsIURI* aDocumentURI,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos);

    nsString                 mValue;
};

class nsXULPrototypePI : public nsXULPrototypeNode
{
public:
    nsXULPrototypePI()
        : nsXULPrototypeNode(eType_PI)
    {
    }

    virtual ~nsXULPrototypePI()
    {
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypePI"; }
    virtual PRUint32 ClassSize() { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsIScriptGlobalObject* aGlobal,
                               const nsCOMArray<nsINodeInfo> *aNodeInfos);
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 nsIURI* aDocumentURI,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos);

    nsString                 mTarget;
    nsString                 mData;
};









#define XUL_ELEMENT_TEMPLATE_GENERATED 1 << NODE_TYPE_SPECIFIC_BITS_OFFSET

class nsScriptEventHandlerOwnerTearoff;

class nsXULElement : public nsGenericElement, public nsIDOMXULElement
{
public:

    
    static nsXULElement* FromContent(nsIContent *aContent)
    {
        if (aContent->IsXUL())
            return static_cast<nsXULElement*>(aContent);
        return nsnull;
    }

public:
    static nsIXBLService* GetXBLService() {
        if (!gXBLService)
            CallGetService("@mozilla.org/xbl;1", &gXBLService);
        return gXBLService;
    }
    static void ReleaseGlobals() {
        NS_IF_RELEASE(gXBLService);
    }

protected:
    
    static nsIXBLService*       gXBLService;

public:
    static nsresult
    Create(nsXULPrototypeElement* aPrototype, nsIDocument* aDocument,
           PRBool aIsScriptable, nsIContent** aResult);

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXULElement,
                                                       nsGenericElement)

    
    virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

    
    virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers);
    virtual void UnbindFromTree(PRBool aDeep, PRBool aNullParent);
    virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);
    virtual nsIAtom *GetIDAttributeName() const;
    virtual nsIAtom *GetClassAttributeName() const;
    virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsAString& aResult) const;
    virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const;
    virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                               const nsAString& aValue,
                               nsCaseTreatment aCaseSensitive) const;
    virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                               nsIAtom* aValue,
                               nsCaseTreatment aCaseSensitive) const;
    virtual PRInt32 FindAttrValueIn(PRInt32 aNameSpaceID,
                                    nsIAtom* aName,
                                    AttrValuesArray* aValues,
                                    nsCaseTreatment aCaseSensitive) const;
    virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                               PRBool aNotify);
    virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const;
    virtual PRUint32 GetAttrCount() const;
    virtual void DestroyContent();

#ifdef DEBUG
    virtual void List(FILE* out, PRInt32 aIndent) const;
    virtual void DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const
    {
    }
#endif

    virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                  PRBool aIsTrustedEvent);

    virtual nsIContent *GetBindingParent() const;
    virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
    virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
    virtual nsIAtom* GetID() const;
    virtual const nsAttrValue* DoGetClasses() const;

    NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
    virtual nsICSSStyleRule* GetInlineStyleRule();
    NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);
    virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                                PRInt32 aModType) const;
    NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

    
    



    void SetTemplateGenerated() { SetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    void ClearTemplateGenerated() { UnsetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    PRBool GetTemplateGenerated() { return HasFlag(XUL_ELEMENT_TEMPLATE_GENERATED); }

    
    NS_FORWARD_NSIDOMNODE(nsGenericElement::)

    
    NS_FORWARD_NSIDOMELEMENT(nsGenericElement::)

    
    NS_DECL_NSIDOMXULELEMENT

    virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
    virtual PRInt32 IntrinsicState() const;

    nsresult GetStyle(nsIDOMCSSStyleDeclaration** aStyle);

    nsresult GetFrameLoader(nsIFrameLoader** aFrameLoader);
    already_AddRefed<nsFrameLoader> GetFrameLoader();
    nsresult SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner);

    virtual void RecompileScriptEventListeners();

    
    
    
    void SetXULBindingParent(nsIContent* aBindingParent)
    {
      mBindingParent = aBindingParent;
    }

protected:
    
    
    friend class nsNodeUtils;

    
    friend class nsNSElementTearoff;

    nsXULElement(nsINodeInfo* aNodeInfo);

    
    nsresult EnsureContentsGenerated(void) const;

    nsresult ExecuteOnBroadcastHandler(nsIDOMElement* anElement, const nsAString& attrName);

    static nsresult
    ExecuteJSCode(nsIDOMElement* anElement, nsEvent* aEvent);

    
    NS_IMETHOD GetParentTree(nsIDOMXULMultiSelectControlElement** aTreeElement);

    nsresult AddPopupListener(nsIAtom* aName);

    class nsXULSlots : public nsGenericElement::nsDOMSlots
    {
    public:
       nsXULSlots(PtrBits aFlags);
       virtual ~nsXULSlots();

       nsRefPtr<nsFrameLoader> mFrameLoader;
    };

    virtual nsINode::nsSlots* CreateSlots();

    nsresult LoadSrc();

    
    nsRefPtr<nsXULPrototypeElement>     mPrototype;

    



    nsIContent*                         mBindingParent;

    


    nsresult MakeHeavyweight();

    




    virtual nsAttrInfo GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const;

    const nsAttrValue* FindLocalOrProtoAttr(PRInt32 aNameSpaceID,
                                            nsIAtom *aName) const {
        return nsXULElement::GetAttrInfo(aNameSpaceID, aName).mValue;
    }

    virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                   const nsAString* aValue, PRBool aNotify);
    virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                  const nsAString* aValue, PRBool aNotify);

    virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult);

    virtual nsresult
      GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                     nsISupports** aTarget,
                                     PRBool* aDefer);
  
    


    nsXULPrototypeAttribute *FindPrototypeAttribute(PRInt32 aNameSpaceID,
                                                    nsIAtom *aName) const;
    


    void AddListenerFor(const nsAttrName& aName,
                        PRBool aCompileEventHandlers);
    void MaybeAddPopupListener(nsIAtom* aLocalName);

    nsIWidget* GetWindowWidget();

    nsresult HideWindowChrome(PRBool aShouldHide);

    void SetTitlebarColor(nscolor aColor, PRBool aActive);

    void SetDrawsInTitlebar(PRBool aState);

    const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

    void RemoveBroadcaster(const nsAString & broadcasterId);

protected:
    
    
    nsIControllers *Controllers() {
      nsDOMSlots* slots = GetExistingDOMSlots();
      return slots ? slots->mControllers : nsnull; 
    }

    void UnregisterAccessKey(const nsAString& aOldValue);
    PRBool BoolAttrIsTrue(nsIAtom* aName);

    friend nsresult
    NS_NewXULElement(nsIContent** aResult, nsINodeInfo *aNodeInfo);

    static already_AddRefed<nsXULElement>
    Create(nsXULPrototypeElement* aPrototype, nsINodeInfo *aNodeInfo,
           PRBool aIsScriptable);

    friend class nsScriptEventHandlerOwnerTearoff;
};

#endif 
