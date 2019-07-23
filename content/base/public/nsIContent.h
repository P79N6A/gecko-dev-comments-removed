



































#ifndef nsIContent_h___
#define nsIContent_h___

#include "nsCOMPtr.h" 
#include "nsStringGlue.h"
#include "nsCaseTreatment.h"
#include "nsChangeHint.h"
#include "nsINode.h"
#include "nsIDocument.h" 


class nsIAtom;
class nsPresContext;
class nsIDOMEvent;
class nsIContent;
class nsIEventListenerManager;
class nsIURI;
class nsICSSStyleRule;
class nsRuleWalker;
class nsAttrValue;
class nsAttrName;
class nsTextFragment;
class nsIDocShell;
#ifdef MOZ_SMIL
class nsISMILAttr;
class nsIDOMCSSStyleDeclaration;
#endif 

enum nsLinkState {
  eLinkState_Unknown    = 0,
  eLinkState_Unvisited  = 1,
  eLinkState_Visited    = 2,
  eLinkState_NotLink    = 3
};



#define NS_ICONTENT_IID       \
{ 0xd510382f, 0xf5eb, 0x48bb, \
  { 0x9a, 0xd9, 0xb3, 0xdc, 0x48, 0x06, 0xfa, 0xaf } }





class nsIContent : public nsINode {
public:
#ifdef MOZILLA_INTERNAL_API
  
  

  nsIContent(nsINodeInfo *aNodeInfo)
    : nsINode(aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo,
                 "No nsINodeInfo passed to nsIContent, PREPARE TO CRASH!!!");
  }
#endif 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_IID)

  
























  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers) = 0;

  













  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE) = 0;
  
  




  nsIDocument *GetDocument() const
  {
    return GetCurrentDoc();
  }

  




  PRBool IsRootOfNativeAnonymousSubtree() const
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

  



  PRBool IsRootOfAnonymousSubtree() const
  {
    NS_ASSERTION(!IsRootOfNativeAnonymousSubtree() ||
                 (GetParent() && GetBindingParent() == GetParent()),
                 "root of native anonymous subtree must have parent equal "
                 "to binding parent");
    NS_ASSERTION(!GetParent() ||
                 ((GetBindingParent() == GetParent()) ==
                  HasFlag(NODE_IS_ANONYMOUS)),
                 "For nodes with parent, flag and GetBindingParent() check "
                 "should match");
    return HasFlag(NODE_IS_ANONYMOUS);
  }

  




  PRBool IsInAnonymousSubtree() const
  {
    NS_ASSERTION(!IsInNativeAnonymousSubtree() || GetBindingParent() || !GetParent(),
                 "must have binding parent when in native anonymous subtree with a parent node");
    return IsInNativeAnonymousSubtree() || GetBindingParent() != nsnull;
  }

  



  inline PRBool IsInHTMLDocument() const
  {
    nsIDocument* doc = GetOwnerDoc();
    return doc && 
           doc->IsHTML();
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

  inline PRBool IsInNamespace(PRInt32 aNamespace) const {
    return mNodeInfo->NamespaceID() == aNamespace;
  }

  inline PRBool IsHTML() const {
    return IsInNamespace(kNameSpaceID_XHTML);
  }

  inline PRBool IsSVG() const {
    
    return IsNodeOfType(eSVG);
  }

  inline PRBool IsXUL() const {
    return IsInNamespace(kNameSpaceID_XUL);
  }

  inline PRBool IsMathML() const {
    return IsInNamespace(kNameSpaceID_MathML);
  }

  




  virtual nsIAtom *GetIDAttributeName() const = 0;

  









  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const = 0;

  












  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }

  













  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify) = 0;

  









  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                         nsAString& aResult) const = 0;

  






  virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const = 0;

  









  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID,
                             nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const
  {
    return PR_FALSE;
  }
  
  









  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID,
                             nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const
  {
    return PR_FALSE;
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
                             PRBool aNotify) = 0;


  










  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const = 0;

  




  virtual PRUint32 GetAttrCount() const = 0;

  




  virtual const nsTextFragment *GetText() = 0;

  



  virtual PRUint32 TextLength() = 0;

  




  virtual nsresult SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                           PRBool aNotify) = 0;

  




  virtual nsresult AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                              PRBool aNotify) = 0;

  




  nsresult SetText(const nsAString& aStr, PRBool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }

  



  virtual PRBool TextIsOnlyWhitespace() = 0;

  



  virtual void AppendTextTo(nsAString& aResult) = 0;

  





















  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull)
  {
    if (aTabIndex) 
      *aTabIndex = -1; 
    return PR_FALSE;
  }

  







  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent)
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
  virtual PRUint32 GetDesiredIMEState()
  {
    if (!IsEditableInternal())
      return IME_STATUS_DISABLE;
    nsIContent *editableAncestor = nsnull;
    for (nsIContent* parent = GetParent();
         parent && parent->HasFlag(NODE_IS_EDITABLE);
         parent = parent->GetParent())
      editableAncestor = parent;
    
    if (editableAncestor)
      return editableAncestor->GetDesiredIMEState();
    return IME_STATUS_ENABLE;
  }

  









  virtual nsIContent *GetBindingParent() const = 0;

  







  virtual already_AddRefed<nsIURI> GetBaseURI() const = 0;

  












  virtual PRBool IsLink(nsIURI** aURI) const = 0;

   






  virtual void DropCachedHref()
  {
  }

  





  virtual nsLinkState GetLinkState() const
  {
    return eLinkState_NotLink;
  }

  




  virtual void SetLinkState(nsLinkState aState)
  {
    NS_ASSERTION(aState == eLinkState_NotLink,
                 "Need to override SetLinkState?");
  }

  






  virtual already_AddRefed<nsIURI> GetHrefURI() const
  {
    return nsnull;
  }

  














  virtual nsresult MaybeTriggerAutoLink(nsIDocShell *aShell)
  {
    return NS_OK;
  }

  


























  virtual void DoneCreatingElement()
  {
  }

  




  virtual void SetMayHaveFrame(PRBool aMayHaveFrame)
  {
  }

  




  virtual PRBool MayHaveFrame() const
  {
    return PR_TRUE;
  }
    
  





  virtual void BeginAddingChildren()
  {
  }

  



























  virtual nsresult DoneAddingChildren(PRBool aHaveNotified)
  {
    return NS_OK;
  }

  








  virtual PRBool IsDoneAddingChildren()
  {
    return PR_TRUE;
  }

  





  
  
  virtual PRInt32 IntrinsicState() const;

  




  virtual nsIAtom* GetID() const = 0;

  





  const nsAttrValue* GetClasses() const {
    if (HasFlag(NODE_MAY_HAVE_CLASS)) {
      return DoGetClasses();
    }
    return nsnull;
  }

  



  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) = 0;

  


  virtual nsICSSStyleRule* GetInlineStyleRule() = 0;

  



  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify) = 0;

  






  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const = 0;

  





  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const = 0;

  




  virtual nsIAtom *GetClassAttributeName() const = 0;

  




  virtual void UpdateEditableState();

  



  virtual void DestroyContent() = 0;

  


  virtual void SaveSubtreeState() = 0;

#ifdef MOZ_SMIL
  





  virtual nsISMILAttr* GetAnimatedAttr(const nsIAtom* aName) = 0;

   







  virtual nsresult GetSMILOverrideStyle(nsIDOMCSSStyleDeclaration** aStyle) = 0;

  




  virtual nsICSSStyleRule* GetSMILOverrideStyleRule() = 0;

  




  virtual nsresult SetSMILOverrideStyleRule(nsICSSStyleRule* aStyleRule,
                                            PRBool aNotify) = 0;
#endif 

private:
  



  virtual const nsAttrValue* DoGetClasses() const = 0;

public:
#ifdef DEBUG
  



  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;

  



  virtual void DumpContent(FILE* out = stdout, PRInt32 aIndent = 0,
                           PRBool aDumpAll = PR_TRUE) const = 0;
#endif

  enum ETabFocusType {
  
    eTabFocus_formElementsMask = (1<<1),  
    eTabFocus_linksMask = (1<<2),         
    eTabFocus_any = 1 + (1<<1) + (1<<2)   
  };

  
  static PRInt32 sTabFocusModel;

  
  
  static PRBool sTabFocusModelAppliesToXUL;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContent, NS_ICONTENT_IID)



#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_LISTENERMANAGER \
  if (tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {           \
    nsContentUtils::TraverseListenerManager(tmp, cb);     \
  }

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_USERDATA \
  if (tmp->HasProperties()) {                      \
    nsNodeUtils::TraverseUserData(tmp, cb);        \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_LISTENERMANAGER \
  if (tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {         \
    nsContentUtils::RemoveListenerManager(tmp);         \
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);          \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA \
  if (tmp->HasProperties()) {                    \
    nsNodeUtils::UnlinkUserData(tmp);            \
  }


#endif 
