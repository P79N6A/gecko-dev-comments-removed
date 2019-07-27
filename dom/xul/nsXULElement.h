










#ifndef nsXULElement_h__
#define nsXULElement_h__

#include "js/TracingAPI.h"
#include "mozilla/Attributes.h"
#include "nsIDOMEvent.h"
#include "nsIServiceManager.h"
#include "nsIAtom.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsIControllers.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFResource.h"
#include "nsIURI.h"
#include "nsIXULTemplateBuilder.h"
#include "nsLayoutCID.h"
#include "nsAttrAndChildArray.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyledElement.h"
#include "nsIFrameLoader.h"
#include "nsFrameLoader.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/ElementInlines.h"

class nsIDocument;
class nsString;
class nsXULPrototypeDocument;

class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIOffThreadScriptReceiver;
class nsXULPrototypeNode;
typedef nsTArray<nsRefPtr<nsXULPrototypeNode> > nsPrototypeArray;

namespace mozilla {
class EventChainPreVisitor;
class EventListenerManager;
namespace css {
class StyleRule;
}
namespace dom {
class BoxObject;
}
}

namespace JS {
class SourceBufferHolder;
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

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsXULPrototypeDocument* aProtoDoc,
                               const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) = 0;
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 nsIURI* aDocumentURI,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) = 0;

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() = 0;
    virtual uint32_t ClassSize() = 0;
#endif

    







    virtual void ReleaseSubtree() { }

    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsXULPrototypeNode)
    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsXULPrototypeNode)

protected:
    explicit nsXULPrototypeNode(Type aType)
        : mType(aType) {}
    virtual ~nsXULPrototypeNode() {}
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
    virtual const char* ClassName() override { return "nsXULPrototypeElement"; }
    virtual uint32_t ClassSize() override { return sizeof(*this); }
#endif

    virtual void ReleaseSubtree() override
    {
        for (int32_t i = mChildren.Length() - 1; i >= 0; i--) {
            if (mChildren[i].get())
                mChildren[i]->ReleaseSubtree();
        }
        mChildren.Clear();
        nsXULPrototypeNode::ReleaseSubtree();
    }

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsXULPrototypeDocument* aProtoDoc,
                               const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 nsIURI* aDocumentURI,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;

    nsresult SetAttrAt(uint32_t aPos, const nsAString& aValue, nsIURI* aDocumentURI);

    void Unlink();

    
    void TraceAllScripts(JSTracer* aTrc);

    nsPrototypeArray         mChildren;

    nsRefPtr<mozilla::dom::NodeInfo> mNodeInfo;

    uint32_t                 mNumAttributes:29;
    uint32_t                 mHasIdAttribute:1;
    uint32_t                 mHasClassAttribute:1;
    uint32_t                 mHasStyleAttribute:1;
    nsXULPrototypeAttribute* mAttributes;         
};

namespace mozilla {
namespace dom {
class XULDocument;
} 
} 

class nsXULPrototypeScript : public nsXULPrototypeNode
{
public:
    nsXULPrototypeScript(uint32_t aLineNo, uint32_t version);
    virtual ~nsXULPrototypeScript();

#ifdef NS_BUILD_REFCNT_LOGGING
    virtual const char* ClassName() override { return "nsXULPrototypeScript"; }
    virtual uint32_t ClassSize() override { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsXULPrototypeDocument* aProtoDoc,
                               const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;
    nsresult SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                nsXULPrototypeDocument* aProtoDoc);
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 nsIURI* aDocumentURI,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;
    nsresult DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                  nsXULPrototypeDocument* aProtoDoc);

    nsresult Compile(JS::SourceBufferHolder& aSrcBuf,
                     nsIURI* aURI, uint32_t aLineNo,
                     nsIDocument* aDocument,
                     nsIOffThreadScriptReceiver *aOffThreadReceiver = nullptr);

    nsresult Compile(const char16_t* aText, int32_t aTextLength,
                     nsIURI* aURI, uint32_t aLineNo,
                     nsIDocument* aDocument,
                     nsIOffThreadScriptReceiver *aOffThreadReceiver = nullptr);

    void UnlinkJSObjects();

    void Set(JSScript* aObject);

    
    
    
    
    JS::Handle<JSScript*> GetScriptObject()
    {
        
        
        
        return JS::Handle<JSScript*>::fromMarkedLocation(mScriptObject.address());
    }

    void TraceScriptObject(JSTracer* aTrc)
    {
        if (mScriptObject) {
            JS_CallScriptTracer(aTrc, &mScriptObject, "active window XUL prototype script");
        }
    }

    void Trace(const TraceCallbacks& aCallbacks, void* aClosure)
    {
        if (mScriptObject) {
            aCallbacks.Trace(&mScriptObject, "mScriptObject", aClosure);
        }
    }

    nsCOMPtr<nsIURI>         mSrcURI;
    uint32_t                 mLineNo;
    bool                     mSrcLoading;
    bool                     mOutOfLine;
    mozilla::dom::XULDocument* mSrcLoadWaiters;   
    uint32_t                 mLangVersion;
private:
    JS::Heap<JSScript*>      mScriptObject;
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
    virtual const char* ClassName() override { return "nsXULPrototypeText"; }
    virtual uint32_t ClassSize() override { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsXULPrototypeDocument* aProtoDoc,
                               const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 nsIURI* aDocumentURI,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;

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
    virtual const char* ClassName() override { return "nsXULPrototypePI"; }
    virtual uint32_t ClassSize() override { return sizeof(*this); }
#endif

    virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                               nsXULPrototypeDocument* aProtoDoc,
                               const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;
    virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 nsIURI* aDocumentURI,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos) override;

    nsString                 mTarget;
    nsString                 mData;
};









#define XUL_ELEMENT_FLAG_BIT(n_) NODE_FLAG_BIT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  XUL_ELEMENT_TEMPLATE_GENERATED =        XUL_ELEMENT_FLAG_BIT(0),
  XUL_ELEMENT_HAS_CONTENTMENU_LISTENER =  XUL_ELEMENT_FLAG_BIT(1),
  XUL_ELEMENT_HAS_POPUP_LISTENER =        XUL_ELEMENT_FLAG_BIT(2)
};

ASSERT_NODE_FLAGS_SPACE(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + 3);

#undef XUL_ELEMENT_FLAG_BIT

class nsXULElement final : public nsStyledElement,
                           public nsIDOMXULElement
{
public:
    explicit nsXULElement(already_AddRefed<mozilla::dom::NodeInfo> aNodeInfo);

    static nsresult
    Create(nsXULPrototypeElement* aPrototype, nsIDocument* aDocument,
           bool aIsScriptable, bool aIsRoot, mozilla::dom::Element** aResult);

    NS_IMPL_FROMCONTENT(nsXULElement, kNameSpaceID_XUL)

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULElement, nsStyledElement)

    
    virtual nsresult PreHandleEvent(
                       mozilla::EventChainPreVisitor& aVisitor) override;

    
    virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers) override;
    virtual void UnbindFromTree(bool aDeep, bool aNullParent) override;
    virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;
    virtual void DestroyContent() override;

#ifdef DEBUG
    virtual void List(FILE* out, int32_t aIndent) const override;
    virtual void DumpContent(FILE* out, int32_t aIndent,bool aDumpAll) const override
    {
    }
#endif

    virtual void PerformAccesskey(bool aKeyCausesActivation,
                                  bool aIsTrustedEvent) override;
    nsresult ClickWithInputSource(uint16_t aInputSource);

    virtual nsIContent *GetBindingParent() const override;
    virtual bool IsNodeOfType(uint32_t aFlags) const override;
    virtual bool IsFocusableInternal(int32_t* aTabIndex, bool aWithMouse) override;

    NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) override;
    virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                                int32_t aModType) const override;
    NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;

    
    



    void SetTemplateGenerated() { SetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    void ClearTemplateGenerated() { UnsetFlags(XUL_ELEMENT_TEMPLATE_GENERATED); }
    bool GetTemplateGenerated() { return HasFlag(XUL_ELEMENT_TEMPLATE_GENERATED); }

    
    NS_FORWARD_NSIDOMNODE_TO_NSINODE
    
    
    using nsStyledElement::GetParentElement;

    
    NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

    
    NS_DECL_NSIDOMXULELEMENT

    virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;
    virtual mozilla::EventStates IntrinsicState() const override;

    nsresult GetFrameLoader(nsIFrameLoader** aFrameLoader);
    nsresult SetIsPrerendered();
    nsresult SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner);

    virtual void RecompileScriptEventListeners() override;

    
    
    
    void SetXULBindingParent(nsIContent* aBindingParent)
    {
      mBindingParent = aBindingParent;
    }

    virtual nsIDOMNode* AsDOMNode() override { return this; }

    virtual bool IsEventAttributeName(nsIAtom* aName) override;

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
    already_AddRefed<mozilla::dom::BoxObject> GetBoxObject(mozilla::ErrorResult& rv);
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

    nsINode* GetScopeChainParent() const override
    {
        
        Element* parent = GetParentElement();
        return parent ? parent : nsStyledElement::GetScopeChainParent();
    }

protected:
    ~nsXULElement();

    
    friend class nsNSElementTearoff;

    
    nsresult EnsureContentsGenerated(void) const;

    nsresult ExecuteOnBroadcastHandler(nsIDOMElement* anElement, const nsAString& attrName);

    static nsresult
    ExecuteJSCode(nsIDOMElement* anElement, mozilla::WidgetEvent* aEvent);

    
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

    virtual nsINode::nsSlots* CreateSlots() override;

    nsresult LoadSrc();

    



    nsIContent*                         mBindingParent;

    


    nsresult MakeHeavyweight(nsXULPrototypeElement* aPrototype);

    virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                   const nsAttrValueOrString* aValue,
                                   bool aNotify) override;
    virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify) override;

    virtual void UpdateEditableState(bool aNotify) override;

    virtual bool ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult) override;

    virtual mozilla::EventListenerManager*
      GetEventListenerManagerForAttr(nsIAtom* aAttrName,
                                     bool* aDefer) override;
  
    


    void AddListenerFor(const nsAttrName& aName,
                        bool aCompileEventHandlers);
    void MaybeAddPopupListener(nsIAtom* aLocalName);

    nsIWidget* GetWindowWidget();

    
    nsresult HideWindowChrome(bool aShouldHide);
    void SetChromeMargins(const nsAttrValue* aValue);
    void ResetChromeMargins();
    void SetTitlebarColor(nscolor aColor, bool aActive);

    void SetDrawsInTitlebar(bool aState);
    void SetDrawsTitle(bool aState);
    void UpdateBrightTitlebarForeground(nsIDocument* aDocument);

    void RemoveBroadcaster(const nsAString & broadcasterId);

protected:
    
    
    nsIControllers *Controllers() {
      nsDOMSlots* slots = GetExistingDOMSlots();
      return slots ? slots->mControllers : nullptr; 
    }

    void UnregisterAccessKey(const nsAString& aOldValue);
    bool BoolAttrIsTrue(nsIAtom* aName) const;

    friend nsresult
    NS_NewXULElement(mozilla::dom::Element** aResult, mozilla::dom::NodeInfo *aNodeInfo);
    friend void
    NS_TrustedNewXULElement(nsIContent** aResult, mozilla::dom::NodeInfo *aNodeInfo);

    static already_AddRefed<nsXULElement>
    Create(nsXULPrototypeElement* aPrototype, mozilla::dom::NodeInfo *aNodeInfo,
           bool aIsScriptable, bool aIsRoot);

    bool IsReadWriteTextElement() const
    {
        return IsAnyOfXULElements(nsGkAtoms::textbox, nsGkAtoms::textarea) &&
               !HasAttr(kNameSpaceID_None, nsGkAtoms::readonly);
    }

    virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

    void MaybeUpdatePrivateLifetime();
};

#endif
