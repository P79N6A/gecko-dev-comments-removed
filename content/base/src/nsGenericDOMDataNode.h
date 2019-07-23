









































#ifndef nsGenericDOMDataNode_h___
#define nsGenericDOMDataNode_h___

#include "nsIContent.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Text.h"
#include "nsTextFragment.h"
#include "nsDOMError.h"
#include "nsIEventListenerManager.h"
#include "nsGenericElement.h"
#include "nsCycleCollectionParticipant.h"
#include "nsContentUtils.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 




#define NS_CREATE_FRAME_IF_NON_WHITESPACE (1 << NODE_TYPE_SPECIFIC_BITS_OFFSET)



#define NS_REFRAME_IF_WHITESPACE (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 1))


#define NS_TEXT_IN_SELECTION (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 2))

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIDOMNodeList;
class nsIFrame;
class nsIDOMText;
class nsINodeInfo;
class nsURI;

class nsGenericDOMDataNode : public nsIContent
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsGenericDOMDataNode(nsINodeInfo *aNodeInfo);
  virtual ~nsGenericDOMDataNode();

  
  nsresult GetNodeValue(nsAString& aNodeValue);
  nsresult SetNodeValue(const nsAString& aNodeValue);
  nsresult GetAttributes(nsIDOMNamedNodeMap** aAttributes)
  {
    NS_ENSURE_ARG_POINTER(aAttributes);
    *aAttributes = nsnull;
    return NS_OK;
  }
  nsresult HasChildNodes(PRBool* aHasChildNodes)
  {
    NS_ENSURE_ARG_POINTER(aHasChildNodes);
    *aHasChildNodes = PR_FALSE;
    return NS_OK;
  }
  nsresult HasAttributes(PRBool* aHasAttributes)
  {
    NS_ENSURE_ARG_POINTER(aHasAttributes);
    *aHasAttributes = PR_FALSE;
    return NS_OK;
  }
  nsresult InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;

    


    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;

    



    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }
  nsresult AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  nsresult GetNamespaceURI(nsAString& aNamespaceURI);
  nsresult GetLocalName(nsAString& aLocalName);
  nsresult GetPrefix(nsAString& aPrefix);
  nsresult SetPrefix(const nsAString& aPrefix);
  nsresult Normalize();
  nsresult IsSupported(const nsAString& aFeature,
                       const nsAString& aVersion,
                       PRBool* aReturn);
  nsresult GetBaseURI(nsAString& aURI);

  nsresult LookupPrefix(const nsAString& aNamespaceURI,
                        nsAString& aPrefix);
  nsresult LookupNamespaceURI(const nsAString& aNamespacePrefix,
                              nsAString& aNamespaceURI);

  
  nsresult GetData(nsAString& aData) const;
  nsresult SetData(const nsAString& aData);
  nsresult GetLength(PRUint32* aLength);
  nsresult SubstringData(PRUint32 aOffset, PRUint32 aCount,
                         nsAString& aReturn);
  nsresult AppendData(const nsAString& aArg);
  nsresult InsertData(PRUint32 aOffset, const nsAString& aArg);
  nsresult DeleteData(PRUint32 aOffset, PRUint32 aCount);
  nsresult ReplaceData(PRUint32 aOffset, PRUint32 aCount,
                       const nsAString& aArg);

  
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsIEventListenerManager* GetListenerManager(PRBool aCreateIfNotFound);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv)
  {
    return nsContentUtils::GetContextForEventHandlers(this, aRv);
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsIAtom *GetIDAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute,
                         nsAString& aResult) const;
  virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute) const;
  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const;
  virtual PRUint32 GetAttrCount() const;
  virtual const nsTextFragment *GetText();
  virtual PRUint32 TextLength();
  virtual nsresult SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                           PRBool aNotify);
  
  nsresult SetText(const nsAString& aStr, PRBool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                              PRBool aNotify);
  virtual PRBool TextIsOnlyWhitespace();
  virtual void AppendTextTo(nsAString& aResult);
  virtual void DestroyContent();
  virtual void SaveSubtreeState();

#ifdef MOZ_SMIL
  virtual nsISMILAttr* GetAnimatedAttr(const nsIAtom* )
  {
    return nsnull;
  }
  virtual nsresult GetSMILOverrideStyle(nsIDOMCSSStyleDeclaration** aStyle);
  virtual nsICSSStyleRule* GetSMILOverrideStyleRule();
  virtual nsresult SetSMILOverrideStyleRule(nsICSSStyleRule* aStyleRule,
                                            PRBool aNotify);
#endif 

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif

  virtual nsIContent *GetBindingParent() const;
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  virtual already_AddRefed<nsIURI> GetBaseURI() const;
  virtual PRBool IsLink(nsIURI** aURI) const;

  virtual PRBool MayHaveFrame() const;

  virtual nsIAtom* GetID() const;
  virtual const nsAttrValue* DoGetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  virtual nsIAtom *GetClassAttributeName() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
  {
    *aResult = CloneDataNode(aNodeInfo, PR_TRUE);
    if (!*aResult) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(*aResult);

    return NS_OK;
  }

  nsresult SplitData(PRUint32 aOffset, nsIContent** aReturn,
                     PRBool aCloneAfterOriginal = PR_TRUE);

  

#ifdef DEBUG
  void ToCString(nsAString& aBuf, PRInt32 aOffset, PRInt32 aLen) const;
#endif

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsGenericDOMDataNode)

protected:
  







  class nsDataSlots : public nsINode::nsSlots
  {
  public:
    nsDataSlots(PtrBits aFlags)
      : nsINode::nsSlots(aFlags),
        mBindingParent(nsnull)
    {
    }

    



    nsIContent* mBindingParent;  
  };

  
  virtual nsINode::nsSlots* CreateSlots();

  nsDataSlots *GetDataSlots()
  {
    return static_cast<nsDataSlots*>(GetSlots());
  }

  nsDataSlots *GetExistingDataSlots() const
  {
    return static_cast<nsDataSlots*>(GetExistingSlots());
  }

  nsresult SplitText(PRUint32 aOffset, nsIDOMText** aReturn);

  friend class nsText3Tearoff;

  static PRInt32 FirstLogicallyAdjacentTextNode(nsIContent* aParent,
                                                PRInt32 aIndex);

  static PRInt32 LastLogicallyAdjacentTextNode(nsIContent* aParent,
                                               PRInt32 aIndex,
                                               PRUint32 aCount);

  nsresult GetWholeText(nsAString& aWholeText);

  nsresult ReplaceWholeText(const nsAFlatString& aContent, nsIDOMText **aReturn);

  nsresult SetTextInternal(PRUint32 aOffset, PRUint32 aCount,
                           const PRUnichar* aBuffer, PRUint32 aLength,
                           PRBool aNotify);

  







  virtual nsGenericDOMDataNode *CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const = 0;

  nsTextFragment mText;

private:
  void SetBidiStatus();

  already_AddRefed<nsIAtom> GetCurrentValueAtom();
};


class nsText3Tearoff : public nsIDOM3Text
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOM3TEXT

  NS_DECL_CYCLE_COLLECTION_CLASS(nsText3Tearoff)

  nsText3Tearoff(nsGenericDOMDataNode *aNode) : mNode(aNode)
  {
  }

protected:
  virtual ~nsText3Tearoff() {}

private:
  nsRefPtr<nsGenericDOMDataNode> mNode;
};












#define NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA                           \
  NS_IMETHOD GetNodeName(nsAString& aNodeName);                             \
  NS_IMETHOD GetLocalName(nsAString& aLocalName) {                          \
    return nsGenericDOMDataNode::GetLocalName(aLocalName);                  \
  }                                                                         \
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue);                           \
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue);                     \
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);                              \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) {                      \
    return nsGenericDOMDataNode::GetParentNode(aParentNode);                \
  }                                                                         \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) {                  \
    return nsGenericDOMDataNode::GetChildNodes(aChildNodes);                \
  }                                                                         \
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes) {                        \
    return nsGenericDOMDataNode::HasChildNodes(aHasChildNodes);             \
  }                                                                         \
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes) {                        \
    return nsGenericDOMDataNode::HasAttributes(aHasAttributes);             \
  }                                                                         \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) {                      \
    return nsGenericDOMDataNode::GetFirstChild(aFirstChild);                \
  }                                                                         \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) {                        \
    return nsGenericDOMDataNode::GetLastChild(aLastChild);                  \
  }                                                                         \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) {            \
    return nsGenericDOMDataNode::GetPreviousSibling(aPreviousSibling);      \
  }                                                                         \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) {                    \
    return nsGenericDOMDataNode::GetNextSibling(aNextSibling);              \
  }                                                                         \
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) {              \
    return nsGenericDOMDataNode::GetAttributes(aAttributes);                \
  }                                                                         \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,     \
                             nsIDOMNode** aReturn) {                        \
    return nsGenericDOMDataNode::InsertBefore(aNewChild, aRefChild,         \
                                              aReturn);                     \
  }                                                                         \
  NS_IMETHOD AppendChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) {     \
    return nsGenericDOMDataNode::AppendChild(aOldChild, aReturn);           \
  }                                                                         \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,     \
                             nsIDOMNode** aReturn) {                        \
    return nsGenericDOMDataNode::ReplaceChild(aNewChild, aOldChild,         \
                                              aReturn);                     \
  }                                                                         \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) {     \
    return nsGenericDOMDataNode::RemoveChild(aOldChild, aReturn);           \
  }                                                                         \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) {            \
    return nsGenericDOMDataNode::GetOwnerDocument(aOwnerDocument);          \
  }                                                                         \
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) {                    \
    return nsGenericDOMDataNode::GetNamespaceURI(aNamespaceURI);            \
  }                                                                         \
  NS_IMETHOD GetPrefix(nsAString& aPrefix) {                                \
    return nsGenericDOMDataNode::GetPrefix(aPrefix);                        \
  }                                                                         \
  NS_IMETHOD SetPrefix(const nsAString& aPrefix) {                          \
    return nsGenericDOMDataNode::SetPrefix(aPrefix);                        \
  }                                                                         \
  NS_IMETHOD Normalize() {                                                  \
    return NS_OK;                                                           \
  }                                                                         \
  NS_IMETHOD IsSupported(const nsAString& aFeature,                         \
                      const nsAString& aVersion,                            \
                      PRBool* aReturn) {                                    \
    return nsGenericDOMDataNode::IsSupported(aFeature, aVersion, aReturn);  \
  }                                                                         \
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn) {                \
    return nsNodeUtils::CloneNodeImpl(this, aDeep, aReturn);                \
  }                                                                         \
  virtual nsGenericDOMDataNode *CloneDataNode(nsINodeInfo *aNodeInfo,       \
                                              PRBool aCloneText) const;

#endif 
