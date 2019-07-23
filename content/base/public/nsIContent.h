



































#ifndef nsIContent_h___
#define nsIContent_h___

#include "nsCOMPtr.h" 
#include "nsStringGlue.h"
#include "nsCaseTreatment.h"
#include "nsChangeHint.h"
#include "nsINode.h"
#include "nsIProgrammingLanguage.h" 


class nsIAtom;
class nsIDocument;
class nsPresContext;
class nsVoidArray;
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


#define NS_ICONTENT_IID       \
{ 0x2813b1d9, 0x7fe1, 0x496f, \
 { 0x85, 0x52, 0xa2, 0xc1, 0xc5, 0x6b, 0x15, 0x40 } }





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
    return HasFlag(NODE_IS_ANONYMOUS);
  }

  



  void SetNativeAnonymous()
  {
    SetFlags(NODE_IS_ANONYMOUS);
  }

  



  virtual nsIContent* FindFirstNonNativeAnonymous() const;

  


  PRBool IsInNativeAnonymousSubtree() const
  {
#ifdef DEBUG
    if (HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE)) {
      return PR_TRUE;
    }
    nsIContent* content = GetBindingParent();
    while (content) {
      if (content->IsRootOfNativeAnonymousSubtree()) {
        NS_ERROR("Element not marked to be in native anonymous subtree!");
        break;
      }
      content = content->GetBindingParent();
    }
    return PR_FALSE;
#else
    return HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE);
#endif
  }

  



  PRBool IsRootOfAnonymousSubtree() const
  {
    NS_ASSERTION(!IsRootOfNativeAnonymousSubtree() ||
                 (GetParent() && GetBindingParent() == GetParent()),
                 "root of native anonymous subtree must have parent equal "
                 "to binding parent");
    nsIContent *bindingParent = GetBindingParent();
    return bindingParent && bindingParent == GetParent();
  }

  



  PRBool IsInAnonymousSubtree() const
  {
    NS_ASSERTION(!IsInNativeAnonymousSubtree() || GetBindingParent(),
                 "must have binding parent when in native anonymous subtree");
    return GetBindingParent() != nsnull;
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

  










  virtual void SetFocus(nsPresContext* aPresContext)
  {
  }

  










  virtual void RemoveFocus(nsPresContext* aPresContext)
  {
  }

  





















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
    IME_STATUS_OPEN     = 0x0008,
    IME_STATUS_CLOSE    = 0x0010
  };
  enum {
    IME_STATUS_MASK_ENABLED = IME_STATUS_ENABLE | IME_STATUS_DISABLE |
                              IME_STATUS_PASSWORD,
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

  


  virtual PRUint32 GetScriptTypeID() const
  { return nsIProgrammingLanguage::JAVASCRIPT; }

  
  virtual nsresult SetScriptTypeID(PRUint32 aLang)
  {
    NS_NOTREACHED("SetScriptTypeID not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  




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

#define NS_IMPL_CYCLE_COLLECTION_TRAVERSE_PRESERVED_WRAPPER      \
  {                                                              \
    nsISupports *preservedWrapper = nsnull;                      \
    if (tmp->GetOwnerDoc())                                      \
      preservedWrapper = tmp->GetOwnerDoc()->GetReference(tmp);  \
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[preserved wrapper]");\
    cb.NoteXPCOMChild(preservedWrapper);                         \
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

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  if (tmp->GetOwnerDoc())                                 \
    tmp->GetOwnerDoc()->RemoveReference(tmp);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA \
  if (tmp->HasProperties()) {                    \
    nsNodeUtils::UnlinkUserData(tmp);            \
  }


#endif 
