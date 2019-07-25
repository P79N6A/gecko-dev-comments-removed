



#ifndef nsIContent_h___
#define nsIContent_h___

#include "nsCaseTreatment.h" 
#include "nsCOMPtr.h"        
#include "nsIDocument.h"     
#include "nsINode.h"         


class nsAString;
class nsIAtom;
class nsIURI;
class nsRuleWalker;
class nsAttrValue;
class nsAttrName;
class nsTextFragment;
class nsIFrame;

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 

enum nsLinkState {
  eLinkState_Unknown    = 0,
  eLinkState_Unvisited  = 1,
  eLinkState_Visited    = 2,
  eLinkState_NotLink    = 3
};


#define NS_ICONTENT_IID \
{ 0x98fb308d, 0xc6dd, 0x4c6d, \
  { 0xb7, 0x7c, 0x91, 0x18, 0x0c, 0xf0, 0x6f, 0x23 } }





class nsIContent : public nsINode {
public:
  typedef mozilla::widget::IMEState IMEState;

#ifdef MOZILLA_INTERNAL_API
  
  

  nsIContent(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsINode(aNodeInfo)
  {
    NS_ASSERTION(mNodeInfo,
                 "No nsINodeInfo passed to nsIContent, PREPARE TO CRASH!!!");
    SetNodeIsContent();
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_IID)

  

























  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) = 0;

  













  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) = 0;
  
  




  nsIDocument *GetDocument() const
  {
    return GetCurrentDoc();
  }

  enum {
    









    eAllChildren = 0,

    










    eAllButXBL = 1,

    



    eSkipPlaceholderContent = 2
  };

  









  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter) = 0;

  




  bool IsRootOfNativeAnonymousSubtree() const
  {
    NS_ASSERTION(!HasFlag(NODE_IS_NATIVE_ANONYMOUS_ROOT) ||
                 (HasFlag(NODE_IS_ANONYMOUS) &&
                  HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE)),
                 "Some flags seem to be missing!");
    return HasFlag(NODE_IS_NATIVE_ANONYMOUS_ROOT);
  }

  



  void SetNativeAnonymous()
  {
    SetFlags(NODE_IS_ANONYMOUS | NODE_IS_IN_ANONYMOUS_SUBTREE |
             NODE_IS_NATIVE_ANONYMOUS_ROOT);
  }

  



  virtual nsIContent* FindFirstNonNativeAnonymous() const;

  



  bool IsRootOfAnonymousSubtree() const
  {
    NS_ASSERTION(!IsRootOfNativeAnonymousSubtree() ||
                 (GetParent() && GetBindingParent() == GetParent()),
                 "root of native anonymous subtree must have parent equal "
                 "to binding parent");
    NS_ASSERTION(!GetParent() ||
                 ((GetBindingParent() == GetParent()) ==
                  HasFlag(NODE_IS_ANONYMOUS)) ||
                 
                 
                 
                 
                 
                 (GetBindingParent() &&
                  (GetBindingParent() == GetParent()->GetBindingParent()) ==
                  HasFlag(NODE_IS_ANONYMOUS)),
                 "For nodes with parent, flag and GetBindingParent() check "
                 "should match");
    return HasFlag(NODE_IS_ANONYMOUS);
  }

  




  bool IsInAnonymousSubtree() const
  {
    NS_ASSERTION(!IsInNativeAnonymousSubtree() || GetBindingParent() || !GetParent(),
                 "must have binding parent when in native anonymous subtree with a parent node");
    return IsInNativeAnonymousSubtree() || GetBindingParent() != nullptr;
  }

  



  inline bool IsInHTMLDocument() const
  {
    return OwnerDoc()->IsHTML();
  }

  



  inline int32_t GetNameSpaceID() const
  {
    return mNodeInfo->NamespaceID();
  }

  



  inline nsINodeInfo* NodeInfo() const
  {
    return mNodeInfo;
  }

  inline bool IsInNamespace(int32_t aNamespace) const
  {
    return mNodeInfo->NamespaceID() == aNamespace;
  }

  inline bool IsHTML() const
  {
    return IsInNamespace(kNameSpaceID_XHTML);
  }

  inline bool IsHTML(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_XHTML);
  }

  inline bool IsSVG() const
  {
    return IsInNamespace(kNameSpaceID_SVG);
  }

  inline bool IsSVG(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_SVG);
  }

  inline bool IsXUL() const
  {
    return IsInNamespace(kNameSpaceID_XUL);
  }

  inline bool IsMathML() const
  {
    return IsInNamespace(kNameSpaceID_MathML);
  }

  inline bool IsMathML(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag, kNameSpaceID_MathML);
  }

  




  virtual nsIAtom *GetIDAttributeName() const = 0;

  









  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const = 0;

  












  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }

  













  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) = 0;

  









  virtual bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName, 
                         nsAString& aResult) const = 0;

  






  virtual bool HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const = 0;

  









  virtual bool AttrValueIs(int32_t aNameSpaceID,
                             nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const
  {
    return false;
  }
  
  









  virtual bool AttrValueIs(int32_t aNameSpaceID,
                             nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const
  {
    return false;
  }
  
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

  




  virtual nsresult SetText(const PRUnichar* aBuffer, uint32_t aLength,
                           bool aNotify) = 0;

  




  virtual nsresult AppendText(const PRUnichar* aBuffer, uint32_t aLength,
                              bool aNotify) = 0;

  




  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }

  



  virtual bool TextIsOnlyWhitespace() = 0;

  



  virtual void AppendTextTo(nsAString& aResult) = 0;

  





















  virtual bool IsFocusable(int32_t *aTabIndex = nullptr, bool aWithMouse = false)
  {
    if (aTabIndex) 
      *aTabIndex = -1; 
    return false;
  }

  







  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent)
  {
  }

  














  virtual IMEState GetDesiredIMEState();

  









  virtual nsIContent *GetBindingParent() const = 0;

  





  nsIContent *GetFlattenedTreeParent() const;

  












  virtual bool IsLink(nsIURI** aURI) const = 0;

  





  virtual nsLinkState GetLinkState() const
  {
    return eLinkState_NotLink;
  }

  






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
    return IsInDoc() ? mPrimaryFrame : nullptr;
  }
  void SetPrimaryFrame(nsIFrame* aFrame) {
    NS_ASSERTION(IsInDoc(), "This will end badly!");
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
        if (!hasAttr && (content->IsHTML() || content->IsSVG())) {
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

  
  virtual already_AddRefed<nsIURI> GetBaseURI() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual bool IsPurple() = 0;
  virtual void RemovePurple() = 0;

  virtual bool OwnedOnlyByTheDOMTree() { return false; }
protected:
  



  virtual nsIAtom* DoGetID() const = 0;

private:
  



  virtual const nsAttrValue* DoGetClasses() const = 0;

public:
#ifdef DEBUG
  



  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const = 0;

  



  virtual void DumpContent(FILE* out = stdout, int32_t aIndent = 0,
                           bool aDumpAll = true) const = 0;
#endif

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

#endif 
