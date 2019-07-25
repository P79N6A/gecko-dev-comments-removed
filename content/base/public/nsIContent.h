



































#ifndef nsIContent_h___
#define nsIContent_h___

#include "nsCOMPtr.h" 
#include "nsStringGlue.h"
#include "nsCaseTreatment.h"
#include "nsChangeHint.h"
#include "nsINode.h"
#include "nsIDocument.h" 
#include "nsDOMMemoryReporter.h"


class nsIAtom;
class nsIDOMEvent;
class nsIContent;
class nsEventListenerManager;
class nsIURI;
class nsRuleWalker;
class nsAttrValue;
class nsAttrName;
class nsTextFragment;
class nsIDocShell;
class nsIFrame;
class nsISMILAttr;
class nsIDOMCSSStyleDeclaration;

namespace mozilla {
namespace css {
class StyleRule;
}
}

enum nsLinkState {
  eLinkState_Unknown    = 0,
  eLinkState_Unvisited  = 1,
  eLinkState_Visited    = 2,
  eLinkState_NotLink    = 3
};


#define NS_ICONTENT_IID       \
{ 0xdec4b381, 0xa3fc, 0x402b, \
 { 0x83, 0x96, 0x0a, 0x7b, 0x37, 0x52, 0xcf, 0x70 } }





class nsIContent : public nsINode {
public:
#ifdef MOZILLA_INTERNAL_API
  
  

  nsIContent(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsINode(aNodeInfo),
      mPrimaryFrame(nsnull)
  {
    NS_ASSERTION(mNodeInfo,
                 "No nsINodeInfo passed to nsIContent, PREPARE TO CRASH!!!");
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_IID)

  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsIContent, nsINode);

  
























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

  









  virtual already_AddRefed<nsINodeList> GetChildren(PRUint32 aFilter) = 0;

  




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
    return IsInNativeAnonymousSubtree() || GetBindingParent() != nsnull;
  }

  



  inline bool IsInHTMLDocument() const
  {
    return OwnerDoc()->IsHTML();
  }

  



  PRInt32 GetNameSpaceID() const
  {
    return mNodeInfo->NamespaceID();
  }

  



  nsIAtom *Tag() const
  {
    return mNodeInfo->NameAtom();
  }

  



  nsINodeInfo *NodeInfo() const
  {
    return mNodeInfo;
  }

  inline bool IsInNamespace(PRInt32 aNamespace) const {
    return mNodeInfo->NamespaceID() == aNamespace;
  }

  inline bool IsHTML() const {
    return IsInNamespace(kNameSpaceID_XHTML);
  }

  inline bool IsHTML(nsIAtom* aTag) const {
    return mNodeInfo->Equals(aTag, kNameSpaceID_XHTML);
  }

  inline bool IsSVG() const {
    
    return IsNodeOfType(eSVG);
  }

  inline bool IsXUL() const {
    return IsInNamespace(kNameSpaceID_XUL);
  }

  inline bool IsMathML() const {
    return IsInNamespace(kNameSpaceID_MathML);
  }

  




  virtual nsIAtom *GetIDAttributeName() const = 0;

  









  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const = 0;

  












  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }

  













  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) = 0;

  









  virtual bool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                         nsAString& aResult) const = 0;

  






  virtual bool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const = 0;

  









  virtual bool AttrValueIs(PRInt32 aNameSpaceID,
                             nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const
  {
    return false;
  }
  
  









  virtual bool AttrValueIs(PRInt32 aNameSpaceID,
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
  virtual PRInt32 FindAttrValueIn(PRInt32 aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const
  {
    return ATTR_MISSING;
  }

  







  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                             bool aNotify) = 0;


  










  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const = 0;

  




  virtual PRUint32 GetAttrCount() const = 0;

  




  virtual const nsTextFragment *GetText() = 0;

  



  virtual PRUint32 TextLength() = 0;

  




  virtual nsresult SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                           bool aNotify) = 0;

  




  virtual nsresult AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                              bool aNotify) = 0;

  




  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }

  



  virtual bool TextIsOnlyWhitespace() = 0;

  



  virtual void AppendTextTo(nsAString& aResult) = 0;

  





















  virtual bool IsFocusable(PRInt32 *aTabIndex = nsnull, bool aWithMouse = false)
  {
    if (aTabIndex) 
      *aTabIndex = -1; 
    return false;
  }

  







  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent)
  {
  }

  






















  enum {
    IME_STATUS_NONE     = 0x0000,
    IME_STATUS_ENABLE   = 0x0001,
    IME_STATUS_DISABLE  = 0x0002,
    IME_STATUS_PASSWORD = 0x0004,
    IME_STATUS_PLUGIN   = 0x0008,
    IME_STATUS_OPEN     = 0x0010,
    IME_STATUS_CLOSE    = 0x0020
  };
  enum {
    IME_STATUS_MASK_ENABLED = IME_STATUS_ENABLE | IME_STATUS_DISABLE |
                              IME_STATUS_PASSWORD | IME_STATUS_PLUGIN,
    IME_STATUS_MASK_OPENED  = IME_STATUS_OPEN | IME_STATUS_CLOSE
  };
  virtual PRUint32 GetDesiredIMEState();

  









  virtual nsIContent *GetBindingParent() const = 0;

  





  nsIContent *GetFlattenedTreeParent() const;

  












  virtual bool IsLink(nsIURI** aURI) const = 0;

  





  virtual nsLinkState GetLinkState() const
  {
    return eLinkState_NotLink;
  }

  






  virtual already_AddRefed<nsIURI> GetHrefURI() const
  {
    return nsnull;
  }

  


























  virtual void DoneCreatingElement()
  {
  }

  





  virtual void BeginAddingChildren()
  {
  }

  



























  virtual nsresult DoneAddingChildren(bool aHaveNotified)
  {
    return NS_OK;
  }

  








  virtual bool IsDoneAddingChildren()
  {
    return true;
  }

  




  nsIAtom* GetID() const {
    if (HasID()) {
      return DoGetID();
    }
    return nsnull;
  }

  





  const nsAttrValue* GetClasses() const {
    if (HasFlag(NODE_MAY_HAVE_CLASS)) {
      return DoGetClasses();
    }
    return nsnull;
  }

  



  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) = 0;

  


  virtual mozilla::css::StyleRule* GetInlineStyleRule() = 0;

  



  NS_IMETHOD SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule, bool aNotify) = 0;

  






  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const = 0;

  





  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const = 0;

  




  virtual nsIAtom *GetClassAttributeName() const = 0;

  





  virtual void UpdateEditableState(bool aNotify);

  



  virtual void DestroyContent() = 0;

  


  virtual void SaveSubtreeState() = 0;

  










  nsIFrame* GetPrimaryFrame() const { return mPrimaryFrame; }
  void SetPrimaryFrame(nsIFrame* aFrame) {
    NS_PRECONDITION(!aFrame || !mPrimaryFrame || aFrame == mPrimaryFrame,
                    "Losing track of existing primary frame");
    mPrimaryFrame = aFrame;
  }

  





  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName) = 0;

  







  virtual nsIDOMCSSStyleDeclaration* GetSMILOverrideStyle() = 0;

  




  virtual mozilla::css::StyleRule* GetSMILOverrideStyleRule() = 0;

  




  virtual nsresult SetSMILOverrideStyleRule(mozilla::css::StyleRule* aStyleRule,
                                            bool aNotify) = 0;

  nsresult LookupNamespaceURI(const nsAString& aNamespacePrefix,
                              nsAString& aNamespaceURI) const;

  



  bool HasIndependentSelection();

  




  nsIContent* GetEditingHost();

  




  void GetLang(nsAString& aResult) const {
    for (const nsIContent* content = this; content; content = content->GetParent()) {
      if (content->GetAttrCount() > 0) {
        
        
        bool hasAttr = content->GetAttr(kNameSpaceID_XML, nsGkAtoms::lang,
                                          aResult);
        if (!hasAttr && content->IsHTML()) {
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

protected:
  



  virtual nsIAtom* DoGetID() const = 0;

private:
  



  virtual const nsAttrValue* DoGetClasses() const = 0;

  


  nsIFrame* mPrimaryFrame;

public:
#ifdef DEBUG
  



  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;

  



  virtual void DumpContent(FILE* out = stdout, PRInt32 aIndent = 0,
                           bool aDumpAll = true) const = 0;
#endif

  enum ETabFocusType {
  
    eTabFocus_formElementsMask = (1<<1),  
    eTabFocus_linksMask = (1<<2),         
    eTabFocus_any = 1 + (1<<1) + (1<<2)   
  };

  
  static PRInt32 sTabFocusModel;

  
  
  static bool sTabFocusModelAppliesToXUL;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContent, NS_ICONTENT_IID)

#endif 
