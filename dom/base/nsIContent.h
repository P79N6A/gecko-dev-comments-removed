



#ifndef nsIContent_h___
#define nsIContent_h___

#include "mozilla/Attributes.h"
#include "nsCaseTreatment.h" 
#include "nsINode.h"


class nsAString;
class nsIAtom;
class nsIURI;
class nsRuleWalker;
class nsAttrValue;
class nsAttrName;
class nsTextFragment;
class nsIFrame;
class nsXBLBinding;

namespace mozilla {
class EventChainPreVisitor;
namespace dom {
class ShadowRoot;
struct CustomElementData;
} 
namespace widget {
struct IMEState;
} 
} 

enum nsLinkState {
  eLinkState_Unvisited  = 1,
  eLinkState_Visited    = 2,
  eLinkState_NotLink    = 3 
};


#define NS_ICONTENT_IID \
{ 0x70f7e9ea, 0xa9bf, 0x48cc, \
  { 0xad, 0x9d, 0x8a, 0xca, 0xee, 0xd2, 0x9b, 0x68 } }





class nsIContent : public nsINode {
public:
  typedef mozilla::widget::IMEState IMEState;

#ifdef MOZILLA_INTERNAL_API
  
  

  explicit nsIContent(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsINode(aNodeInfo)
  {
    MOZ_ASSERT(mNodeInfo);
    SetNodeIsContent();
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_IID)

  



























  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) = 0;

  













  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) = 0;

  enum {
    









    eAllChildren = 0,

    










    eAllButXBL = 1,

    



    eSkipPlaceholderContent = 2
  };

  









  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter) = 0;

  




  bool IsRootOfNativeAnonymousSubtree() const
  {
    NS_ASSERTION(!HasFlag(NODE_IS_NATIVE_ANONYMOUS_ROOT) ||
                 (HasFlag(NODE_IS_ANONYMOUS_ROOT) &&
                  HasFlag(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE)),
                 "Some flags seem to be missing!");
    return HasFlag(NODE_IS_NATIVE_ANONYMOUS_ROOT);
  }

  bool IsRootOfChromeAccessOnlySubtree() const
  {
    return HasFlag(NODE_IS_NATIVE_ANONYMOUS_ROOT |
                   NODE_IS_ROOT_OF_CHROME_ONLY_ACCESS);
  }

  



  void SetIsNativeAnonymousRoot()
  {
    SetFlags(NODE_IS_ANONYMOUS_ROOT | NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE |
             NODE_IS_NATIVE_ANONYMOUS_ROOT);
  }

  



  virtual nsIContent* FindFirstNonChromeOnlyAccessContent() const;

  



  bool IsRootOfAnonymousSubtree() const
  {
    NS_ASSERTION(!IsRootOfNativeAnonymousSubtree() ||
                 (GetParent() && GetBindingParent() == GetParent()),
                 "root of native anonymous subtree must have parent equal "
                 "to binding parent");
    NS_ASSERTION(!GetParent() ||
                 ((GetBindingParent() == GetParent()) ==
                  HasFlag(NODE_IS_ANONYMOUS_ROOT)) ||
                 
                 
                 
                 
                 
                 (GetBindingParent() &&
                  (GetBindingParent() == GetParent()->GetBindingParent()) ==
                  HasFlag(NODE_IS_ANONYMOUS_ROOT)),
                 "For nodes with parent, flag and GetBindingParent() check "
                 "should match");
    return HasFlag(NODE_IS_ANONYMOUS_ROOT);
  }

  




  bool IsInAnonymousSubtree() const
  {
    NS_ASSERTION(!IsInNativeAnonymousSubtree() || GetBindingParent() ||
                 (!IsInDoc() &&
                  static_cast<nsIContent*>(SubtreeRoot())->IsInNativeAnonymousSubtree()),
                 "Must have binding parent when in native anonymous subtree which is in document.\n"
                 "Native anonymous subtree which is not in document must have native anonymous root.");
    return IsInNativeAnonymousSubtree() || (!IsInShadowTree() && GetBindingParent() != nullptr);
  }

  



  inline bool IsInHTMLDocument() const;

  



  inline int32_t GetNameSpaceID() const
  {
    return mNodeInfo->NamespaceID();
  }

  inline bool IsHTMLElement() const
  {
    return IsInNamespace(kNameSpaceID_XHTML);
  }

  inline bool IsHTMLElement(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_XHTML);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfHTMLElements(First aFirst, Args... aArgs) const
  {
    return IsHTMLElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsSVGElement() const
  {
    return IsInNamespace(kNameSpaceID_SVG);
  }

  inline bool IsSVGElement(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_SVG);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfSVGElements(First aFirst, Args... aArgs) const
  {
    return IsSVGElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsXULElement() const
  {
    return IsInNamespace(kNameSpaceID_XUL);
  }

  inline bool IsXULElement(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_XUL);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfXULElements(First aFirst, Args... aArgs) const
  {
    return IsXULElement() && IsNodeInternal(aFirst, aArgs...);
  }

  inline bool IsMathMLElement() const
  {
    return IsInNamespace(kNameSpaceID_MathML);
  }

  inline bool IsMathMLElement(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_MathML);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfMathMLElements(First aFirst, Args... aArgs) const
  {
    return IsMathMLElement() && IsNodeInternal(aFirst, aArgs...);
  }
  inline bool IsActiveChildrenElement() const
  {
    return mNodeInfo->Equals(nsGkAtoms::children, kNameSpaceID_XBL) &&
           GetBindingParent();
  }

  












  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }

  













  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) = 0;

  









  bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
               nsAString& aResult) const;

  






  bool HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const;

  









  bool AttrValueIs(int32_t aNameSpaceID,
                   nsIAtom* aName,
                   const nsAString& aValue,
                   nsCaseTreatment aCaseSensitive) const;
  
  









  bool AttrValueIs(int32_t aNameSpaceID,
                   nsIAtom* aName,
                   nsIAtom* aValue,
                   nsCaseTreatment aCaseSensitive) const;
  
  enum {
    ATTR_MISSING = -1,
    ATTR_VALUE_NO_MATCH = -2
  };
  
















  typedef nsIAtom* const* const AttrValuesArray;
  virtual int32_t FindAttrValueIn(int32_t aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const
  {
    return ATTR_MISSING;
  }

  







  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr, 
                             bool aNotify) = 0;


  










  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const = 0;

  




  virtual uint32_t GetAttrCount() const = 0;

  




  virtual const nsTextFragment *GetText() = 0;

  



  virtual uint32_t TextLength() const = 0;

   






  virtual bool IsEventAttributeName(nsIAtom* aName)
  {
    return false;
  }

  




  virtual nsresult SetText(const char16_t* aBuffer, uint32_t aLength,
                           bool aNotify) = 0;

  




  virtual nsresult AppendText(const char16_t* aBuffer, uint32_t aLength,
                              bool aNotify) = 0;

  




  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }

  



  virtual bool TextIsOnlyWhitespace() = 0;

  





  virtual bool HasTextForTranslation() = 0;

  



  virtual void AppendTextTo(nsAString& aResult) = 0;

  



  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendTextTo(nsAString& aResult, const mozilla::fallible_t&) = 0;

  





















  bool IsFocusable(int32_t* aTabIndex = nullptr, bool aWithMouse = false);
  virtual bool IsFocusableInternal(int32_t* aTabIndex, bool aWithMouse);

  







  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent)
  {
  }

  














  virtual IMEState GetDesiredIMEState();

  









  virtual nsIContent *GetBindingParent() const = 0;

  




  virtual nsXBLBinding *GetXBLBinding() const = 0;

  












  virtual void SetXBLBinding(nsXBLBinding* aBinding,
                             nsBindingManager* aOldBindingManager = nullptr) = 0;

  





  virtual void SetShadowRoot(mozilla::dom::ShadowRoot* aShadowRoot) = 0;

  




  virtual mozilla::dom::ShadowRoot *GetShadowRoot() const = 0;

  






  virtual mozilla::dom::ShadowRoot *GetContainingShadow() const = 0;

  




  virtual nsTArray<nsIContent*> &DestInsertionPoints() = 0;

  




  virtual nsTArray<nsIContent*> *GetExistingDestInsertionPoints() const = 0;

  





  virtual nsIContent *GetXBLInsertionParent() const = 0;

  




  virtual void SetXBLInsertionParent(nsIContent* aContent) = 0;

  






  nsIContent *GetFlattenedTreeParent() const;

  





  virtual mozilla::dom::CustomElementData *GetCustomElementData() const = 0;

  





  virtual void SetCustomElementData(mozilla::dom::CustomElementData* aData) = 0;

  












  virtual bool IsLink(nsIURI** aURI) const = 0;

  






  virtual already_AddRefed<nsIURI> GetHrefURI() const
  {
    return nullptr;
  }

  


























  virtual void DoneCreatingElement()
  {
  }

  





  virtual void BeginAddingChildren()
  {
  }

  


















  virtual void DoneAddingChildren(bool aHaveNotified)
  {
  }

  








  virtual bool IsDoneAddingChildren()
  {
    return true;
  }

  



  nsIAtom* GetID() const {
    if (HasID()) {
      return DoGetID();
    }
    return nullptr;
  }

  




  const nsAttrValue* GetClasses() const {
    if (HasFlag(NODE_MAY_HAVE_CLASS)) {
      return DoGetClasses();
    }
    return nullptr;
  }

  



  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) = 0;

  





  virtual void UpdateEditableState(bool aNotify);

  



  virtual void DestroyContent() = 0;

  


  virtual void SaveSubtreeState() = 0;

  










  nsIFrame* GetPrimaryFrame() const
  {
    return (IsInDoc() || IsInShadowTree()) ? mPrimaryFrame : nullptr;
  }
  void SetPrimaryFrame(nsIFrame* aFrame) {
    MOZ_ASSERT(IsInDoc() || IsInShadowTree(), "This will end badly!");
    NS_PRECONDITION(!aFrame || !mPrimaryFrame || aFrame == mPrimaryFrame,
                    "Losing track of existing primary frame");
    mPrimaryFrame = aFrame;
  }

  nsresult LookupNamespaceURIInternal(const nsAString& aNamespacePrefix,
                                      nsAString& aNamespaceURI) const;

  



  bool HasIndependentSelection();

  




  mozilla::dom::Element* GetEditingHost();

  




  void GetLang(nsAString& aResult) const {
    for (const nsIContent* content = this; content; content = content->GetParent()) {
      if (content->GetAttrCount() > 0) {
        
        
        bool hasAttr = content->GetAttr(kNameSpaceID_XML, nsGkAtoms::lang,
                                          aResult);
        if (!hasAttr && (content->IsHTMLElement() || content->IsSVGElement())) {
          hasAttr = content->GetAttr(kNameSpaceID_None, nsGkAtoms::lang,
                                     aResult);
        }
        NS_ASSERTION(hasAttr || aResult.IsEmpty(),
                     "GetAttr that returns false should not make string non-empty");
        if (hasAttr) {
          return;
        }
      }
    }
  }

  
  virtual already_AddRefed<nsIURI> GetBaseURI(bool aTryUseXHRDocBaseURI = false) const override;

  virtual nsresult PreHandleEvent(
                     mozilla::EventChainPreVisitor& aVisitor) override;

  virtual bool IsPurple() = 0;
  virtual void RemovePurple() = 0;

  virtual bool OwnedOnlyByTheDOMTree() { return false; }
protected:
  



  nsIAtom* DoGetID() const;

private:
  



  const nsAttrValue* DoGetClasses() const;

public:
#ifdef DEBUG
  



  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const = 0;

  



  virtual void DumpContent(FILE* out = stdout, int32_t aIndent = 0,
                           bool aDumpAll = true) const = 0;
#endif

  




  virtual void Describe(nsAString& aOutDescription) const {
    aOutDescription = NS_LITERAL_STRING("(not an element)");
  }

  enum ETabFocusType {
  
    eTabFocus_formElementsMask = (1<<1),  
    eTabFocus_linksMask = (1<<2),         
    eTabFocus_any = 1 + (1<<1) + (1<<2)   
  };

  
  static int32_t sTabFocusModel;

  
  
  static bool sTabFocusModelAppliesToXUL;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContent, NS_ICONTENT_IID)

inline nsIContent* nsINode::AsContent()
{
  MOZ_ASSERT(IsContent());
  return static_cast<nsIContent*>(this);
}

#define NS_IMPL_FROMCONTENT_HELPER(_class, _check)                             \
  static _class* FromContent(nsIContent* aContent)                             \
  {                                                                            \
    return aContent->_check ? static_cast<_class*>(aContent) : nullptr;        \
  }                                                                            \
  static _class* FromContentOrNull(nsIContent* aContent)                       \
  {                                                                            \
    return aContent ? FromContent(aContent) : nullptr;                         \
  }

#define NS_IMPL_FROMCONTENT(_class, _nsid)                                     \
  NS_IMPL_FROMCONTENT_HELPER(_class, IsInNamespace(_nsid))

#define NS_IMPL_FROMCONTENT_WITH_TAG(_class, _nsid, _tag)                      \
  NS_IMPL_FROMCONTENT_HELPER(_class, NodeInfo()->Equals(nsGkAtoms::_tag, _nsid))

#define NS_IMPL_FROMCONTENT_HTML_WITH_TAG(_class, _tag)                        \
  NS_IMPL_FROMCONTENT_WITH_TAG(_class, kNameSpaceID_XHTML, _tag)

#endif 
