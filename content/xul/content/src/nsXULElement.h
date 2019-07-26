










#ifndef nsXULElement_h__
#define nsXULElement_h__


#include "nsIDOMEvent.h"
#include "nsIServiceManager.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsIControllers.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsEventListenerManager.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFResource.h"
#include "nsBindingManager.h"
#include "nsIURI.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIBoxObject.h"
#include "nsLayoutCID.h"
#include "nsAttrAndChildArray.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyledElement.h"
#include "nsIFrameLoader.h"
#include "jspubtd.h"
#include "nsFrameLoader.h"

class nsIDocument;
class nsString;
class nsIDocShell;

class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIScriptGlobalObjectOwner;
class nsXULPrototypeNode;
typedef nsTArray<nsRefPtr<nsXULPrototypeNode> > nsPrototypeArray;

namespace mozilla {
namespace css {
class StyleRule;
}
}



#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
#define XUL_PROTOTYPE_ATTRIBUTE_METER(counter) (nsXULPrototypeAttribute::counter++)
#else
#define XUL_PROTOTYPE_ATTRIBUTE_METER(counter) ((void) 0)
#endif








class nsXULPrototypeAttribute
{
public:
    nsXULPrototypeAttribute()
        : mName(nsGkAtoms::id)  
    {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumAttributes);
        MOZ_COUNT_CTOR(nsXULPrototypeAttribute);
    }

    ~nsXULPrototypeAttribute();

    nsAttrName mName;
    nsAttrValue mValue;

#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
    static uint32_t   gNumElements;
    static uint32_t   gNumAttributes;
    static uint32_t   gNumCacheTests;
    static uint32_t   gNumCacheHits;
    static uint32_t   gNumCacheSets;
    static uint32_t   gNumCacheFills;
#endif 
};









class nsXULPrototypeNode
{
public:
    enum Type { eType_Element, eType_Script, eType_Text, eType_PI };

    Type                     mType;

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
    virtual uint32_t ClassSize() = 0;
#endif

    







    virtual void ReleaseSubtree() { }

    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsXULPrototypeNode)
    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsXULPrototypeNode)

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
          mHasIdAttribute(false),
          mHasClassAttribute(false),
          mHasStyleAttribute(false),
          mAttributes(nullptr)
    {
    }

    virtual ~nsXULPrototypeElement()
    {
        Unlink();
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypeElement"; }
    virtual uint32_t ClassSize() { return sizeof(*this); }
#endif

    virtual void ReleaseSubtree()
    {
        for (int32_t i = mChildren.Length() - 1; i >= 0; i--) {
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

    nsresult SetAttrAt(uint32_t aPos, const nsAString& aValue, nsIURI* aDocumentURI);

    void Unlink();

    
    void TraceAllScripts(JSTracer* aTrc);

    nsPrototypeArray         mChildren;

    nsCOMPtr<nsINodeInfo>    mNodeInfo;           

    uint32_t                 mNumAttributes:29;
    uint32_t                 mHasIdAttribute:1;
    uint32_t                 mHasClassAttribute:1;
    uint32_t                 mHasStyleAttribute:1;
    nsXULPrototypeAttribute* mAttributes;         
};

class nsXULDocument;

class nsXULPrototypeScript : public nsXULPrototypeNode
{
public:
    nsXULPrototypeScript(uint32_t aLineNo, uint32_t version);
    virtual ~nsXULPrototypeScript();

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() { return "nsXULPrototypeScript"; }
    virtual uint32_t ClassSize() { return sizeof(*this); }
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

    nsresult Compile(const PRUnichar* aText, int32_t aTextLength,
                     nsIURI* aURI, uint32_t aLineNo,
                     nsIDocument* aDocument,
                     nsIScriptGlobalObjectOwner* aGlobalOwner);

    void UnlinkJSObjects();

    void Set(JSScript* aObject);

    JSScript *GetScriptObject()
    {
        return mScriptObject;
    }

    nsCOMPtr<nsIURI>         mSrcURI;
    uint32_t                 mLineNo;
    bool                     mSrcLoading;
    bool                     mOutOfLine;
    nsXULDocument*           mSrcLoadWaiters;   
    uint32_t                 mLangVersion;
private:
    JSScript*                mScriptObject;
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
    virtual uint32_t ClassSize() { return sizeof(*this); }
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
    virtual uint32_t ClassSize() { return sizeof(*this); }
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









#define XUL_ELEMENT_FLAG_BIT(n_) NODE_FLAG_BIT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  XUL_ELEMENT_TEMPLATE_GENERATED =        XUL_ELEMENT_FLAG_BIT(0),
  XUL_ELEMENT_HAS_CONTENTMENU_LISTENER =  XUL_ELEMENT_FLAG_BIT(1),
  XUL_ELEMENT_HAS_POPUP_LISTENER =        XUL_ELEMENT_FLAG_BIT(2)
};


PR_STATIC_ASSERT((ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + 2) < 32);

#undef XUL_ELEMENT_FLAG_BIT

class nsScriptEventHandlerOwnerTearoff;

class nsXULElement : public nsStyledElement,
                     public nsIDOMXULElement
{
public:
    nsXULElement(already_AddRefed<nsINodeInfo> aNodeInfo);

    static nsresult
    Create(nsXULPrototypeElement* aPrototype, nsIDocument* aDocument,
           bool aIsScriptable, bool aIsRoot, mozilla::dom::Element** aResult);

    NS_IMPL_FROMCONTENT(nsXULElement, kNameSpaceID_XUL)

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXULElement,
                                                       mozilla::dom::Element)

    
    virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

    
    virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers);
    virtual void UnbindFromTree(bool aDeep, bool aNullParent);
    virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
    virtual void DestroyContent();

#ifdef DEBUG
    virtual void List(FILE* out, int32_t aIndent) const;
    virtual void DumpContent(FILE* out, int32_t aIndent,bool aDumpAll) const
    {
    }
#endif

    virtual void PerformAccesskey(bool aKeyCausesActivation,
                                  bool aIsTrustedEvent);
    nsresult ClickWithInputSource(uint16_t aInputSource);

    virtual nsIContent *GetBindingParent() const;
    virtual bool IsNodeOfType(uint32_t aFlags) const;
    virtual bool IsFocusable(int32_t *aTabIndex = nullptr, bool aWithMouse = false);

    NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
    virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                                int32_t aModType) const;
    NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

    
    



    void SetTemplateGenerated() { SetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    void ClearTemplateGenerated() { UnsetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    bool GetTemplateGenerated() { return HasFlag(XUL_ELEMENT_TEMPLATE_GENERATED); }

    
    NS_FORWARD_NSIDOMNODE_TO_NSINODE
    
    
    using nsStyledElement::GetParentElement;

    
    NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

    
    NS_DECL_NSIDOMXULELEMENT

    virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
    virtual nsEventStates IntrinsicState() const;

    nsresult GetFrameLoader(nsIFrameLoader** aFrameLoader);
    nsresult SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner);

    virtual void RecompileScriptEventListeners();

    
    
    
    void SetXULBindingParent(nsIContent* aBindingParent)
    {
      mBindingParent = aBindingParent;
    }

    virtual nsIDOMNode* AsDOMNode() { return this; }

    virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;

    void SetXULAttr(nsIAtom* aName, const nsAString& aValue,
                    mozilla::ErrorResult& aError)
    {
        aError = SetAttr(kNameSpaceID_None, aName, aValue, true);
    }
    void SetXULBoolAttr(nsIAtom* aName, bool aValue)
    {
        if (aValue) {
            SetAttr(kNameSpaceID_None, aName, NS_LITERAL_STRING("true"), true);
        } else {
            UnsetAttr(kNameSpaceID_None, aName, true);
        }
    }

    
    
    
    void SetClassName(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::_class, aValue, rv);
    }
    void SetAlign(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::align, aValue, rv);
    }
    void SetDir(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::dir, aValue, rv);
    }
    void SetFlex(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::flex, aValue, rv);
    }
    void SetFlexGroup(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::flexgroup, aValue, rv);
    }
    void SetOrdinal(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::ordinal, aValue, rv);
    }
    void SetOrient(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::orient, aValue, rv);
    }
    void SetPack(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::pack, aValue, rv);
    }
    bool Hidden() const
    {
        return BoolAttrIsTrue(nsGkAtoms::hidden);
    }
    bool Collapsed() const
    {
        return BoolAttrIsTrue(nsGkAtoms::collapsed);
    }
    void SetObserves(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::observes, aValue, rv);
    }
    void SetMenu(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::menu, aValue, rv);
    }
    void SetContextMenu(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::contextmenu, aValue, rv);
    }
    void SetTooltip(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::tooltip, aValue, rv);
    }
    void SetWidth(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::width, aValue, rv);
    }
    void SetHeight(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::height, aValue, rv);
    }
    void SetMinWidth(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::minwidth, aValue, rv);
    }
    void SetMinHeight(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::minheight, aValue, rv);
    }
    void SetMaxWidth(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::maxwidth, aValue, rv);
    }
    void SetMaxHeight(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::maxheight, aValue, rv);
    }
    void SetPersist(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::persist, aValue, rv);
    }
    void SetLeft(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::left, aValue, rv);
    }
    void SetTop(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::top, aValue, rv);
    }
    void SetDatasources(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::datasources, aValue, rv);
    }
    void SetRef(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::ref, aValue, rv);
    }
    void SetTooltipText(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::tooltiptext, aValue, rv);
    }
    void SetStatusText(const nsAString& aValue, mozilla::ErrorResult& rv)
    {
        SetXULAttr(nsGkAtoms::statustext, aValue, rv);
    }
    bool AllowEvents() const
    {
        return BoolAttrIsTrue(nsGkAtoms::allowevents);
    }
    already_AddRefed<nsIRDFCompositeDataSource> GetDatabase();
    already_AddRefed<nsIXULTemplateBuilder> GetBuilder();
    already_AddRefed<nsIRDFResource> GetResource(mozilla::ErrorResult& rv);
    nsIControllers* GetControllers(mozilla::ErrorResult& rv);
    already_AddRefed<nsIBoxObject> GetBoxObject(mozilla::ErrorResult& rv);
    void Focus(mozilla::ErrorResult& rv);
    void Blur(mozilla::ErrorResult& rv);
    void Click(mozilla::ErrorResult& rv);
    
    already_AddRefed<nsINodeList>
      GetElementsByAttribute(const nsAString& aAttribute,
                             const nsAString& aValue);
    already_AddRefed<nsINodeList>
      GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                               const nsAString& aAttribute,
                               const nsAString& aValue,
                               mozilla::ErrorResult& rv);
    
    already_AddRefed<nsFrameLoader> GetFrameLoader();
    void SwapFrameLoaders(nsXULElement& aOtherOwner, mozilla::ErrorResult& rv);

    
    nsINode* GetParentObject() const
    {
        Element* parent = GetParentElement();
        if (parent) {
            return parent;
        }
        return nsStyledElement::GetParentObject();
    }

protected:

    
    friend class nsNSElementTearoff;

    
    nsresult EnsureContentsGenerated(void) const;

    nsresult ExecuteOnBroadcastHandler(nsIDOMElement* anElement, const nsAString& attrName);

    static nsresult
    ExecuteJSCode(nsIDOMElement* anElement, nsEvent* aEvent);

    
    NS_IMETHOD GetParentTree(nsIDOMXULMultiSelectControlElement** aTreeElement);

    nsresult AddPopupListener(nsIAtom* aName);

    class nsXULSlots : public mozilla::dom::Element::nsDOMSlots
    {
    public:
        nsXULSlots();
        virtual ~nsXULSlots();

        void Traverse(nsCycleCollectionTraversalCallback &cb);

        nsRefPtr<nsFrameLoader> mFrameLoader;
    };

    virtual nsINode::nsSlots* CreateSlots();

    nsresult LoadSrc();

    



    nsIContent*                         mBindingParent;

    


    nsresult MakeHeavyweight(nsXULPrototypeElement* aPrototype);

    virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                   const nsAttrValueOrString* aValue,
                                   bool aNotify);
    virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify);

    virtual void UpdateEditableState(bool aNotify);

    virtual bool ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult);

    virtual nsEventListenerManager*
      GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer);
  
    


    void AddListenerFor(const nsAttrName& aName,
                        bool aCompileEventHandlers);
    void MaybeAddPopupListener(nsIAtom* aLocalName);

    nsIWidget* GetWindowWidget();

    
    nsresult HideWindowChrome(bool aShouldHide);
    void SetChromeMargins(const nsAttrValue* aValue);
    void ResetChromeMargins();
    void SetTitlebarColor(nscolor aColor, bool aActive);

    void SetDrawsInTitlebar(bool aState);

    void RemoveBroadcaster(const nsAString & broadcasterId);

protected:
    
    
    nsIControllers *Controllers() {
      nsDOMSlots* slots = GetExistingDOMSlots();
      return slots ? slots->mControllers : nullptr; 
    }

    void UnregisterAccessKey(const nsAString& aOldValue);
    bool BoolAttrIsTrue(nsIAtom* aName) const;

    friend nsresult
    NS_NewXULElement(nsIContent** aResult, nsINodeInfo *aNodeInfo);
    friend void
    NS_TrustedNewXULElement(nsIContent** aResult, nsINodeInfo *aNodeInfo);

    static already_AddRefed<nsXULElement>
    Create(nsXULPrototypeElement* aPrototype, nsINodeInfo *aNodeInfo,
           bool aIsScriptable, bool aIsRoot);

    bool IsReadWriteTextElement() const
    {
        const nsIAtom* tag = Tag();
        return
            GetNameSpaceID() == kNameSpaceID_XUL &&
            (tag == nsGkAtoms::textbox || tag == nsGkAtoms::textarea) &&
            !HasAttr(kNameSpaceID_None, nsGkAtoms::readonly);
    }

    virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

    void MaybeUpdatePrivateLifetime();
};

#endif
