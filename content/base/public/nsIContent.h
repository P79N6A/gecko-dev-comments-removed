



































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
class nsISupportsArray;
class nsIEventListenerManager;
class nsIURI;
class nsICSSStyleRule;
class nsRuleWalker;
class nsAttrValue;
class nsAttrName;
class nsTextFragment;
class nsIDocShell;


#define NS_ICONTENT_IID       \
{ 0xb6408b0, 0x20c6, 0x4d60, \
  { 0xb7, 0x2f, 0x90, 0xb7, 0x7a, 0x9d, 0xb9, 0xb6 } }



class nsIContent_base : public nsINode {
public:
#ifdef MOZILLA_INTERNAL_API
  
  

  nsIContent_base(nsINodeInfo *aNodeInfo)
    : nsINode(aNodeInfo)
  {
  }
#endif

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_IID)
};





class nsIContent : public nsIContent_base {
public:
#ifdef MOZILLA_INTERNAL_API
  
  

  nsIContent(nsINodeInfo *aNodeInfo)
    : nsIContent_base(aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo,
                 "No nsINodeInfo passed to nsIContent, PREPARE TO CRASH!!!");
  }
#endif 

  
























  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers) = 0;

  













  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE) = 0;
  
  




  nsIDocument *GetDocument() const
  {
    return GetCurrentDoc();
  }

  




  PRBool IsNativeAnonymous() const
  {
    return HasFlag(NODE_IS_ANONYMOUS);
  }

  


  PRBool IsAnonymousForEvents() const
  {
    return HasFlag(NODE_IS_ANONYMOUS_FOR_EVENTS);
  }

  





  virtual void SetNativeAnonymous(PRBool aAnonymous);

  



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
    IME_STATUS_NONE    = 0x0000,
    IME_STATUS_ENABLE  = 0x0001,
    IME_STATUS_DISABLE = 0x0002,
    IME_STATUS_OPEN    = 0x0004,
    IME_STATUS_CLOSE   = 0x0008
  };
  enum {
    IME_STATUS_MASK_ENABLED = IME_STATUS_ENABLE | IME_STATUS_DISABLE,
    IME_STATUS_MASK_OPENED  = IME_STATUS_OPEN | IME_STATUS_CLOSE
  };
  virtual PRUint32 GetDesiredIMEState()
  {
    return IME_STATUS_DISABLE;
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

  





  
  
  virtual PRInt32 IntrinsicState() const
  {
    return 0;
  }
    
  


  virtual PRUint32 GetScriptTypeID() const
  { return nsIProgrammingLanguage::JAVASCRIPT; }

  
  virtual nsresult SetScriptTypeID(PRUint32 aLang)
  {
    NS_NOTREACHED("SetScriptTypeID not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  




  virtual nsIAtom* GetID() const = 0;

  





  virtual const nsAttrValue* GetClasses() const = 0;

  



  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) = 0;

  


  virtual nsICSSStyleRule* GetInlineStyleRule() = 0;

  



  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify) = 0;

  






  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const = 0;

  





  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const = 0;

  




  virtual nsIAtom *GetClassAttributeName() const = 0;


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
    cb.NoteXPCOMChild(preservedWrapper);                         \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_LISTENERMANAGER \
  if (tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {         \
    nsContentUtils::RemoveListenerManager(tmp);         \
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);          \
  }

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  if (tmp->GetOwnerDoc())                                 \
    tmp->GetOwnerDoc()->RemoveReference(tmp);


#endif
