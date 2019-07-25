









#ifndef nsGenericDOMDataNode_h___
#define nsGenericDOMDataNode_h___

#include "nsIContent.h"

#include "nsTextFragment.h"
#include "nsError.h"
#include "nsEventListenerManager.h"
#include "nsGenericElement.h"
#include "nsCycleCollectionParticipant.h"

#include "nsISMILAttr.h"




#define NS_CREATE_FRAME_IF_NON_WHITESPACE (1 << NODE_TYPE_SPECIFIC_BITS_OFFSET)



#define NS_REFRAME_IF_WHITESPACE (1 << (NODE_TYPE_SPECIFIC_BITS_OFFSET + 1))


PR_STATIC_ASSERT(NODE_TYPE_SPECIFIC_BITS_OFFSET + 1 < 32);

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

  NS_DECL_SIZEOF_EXCLUDING_THIS

  nsGenericDOMDataNode(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsGenericDOMDataNode();

  
  nsresult GetNodeName(nsAString& aNodeName)
  {
    aNodeName = NodeName();
    return NS_OK;
  }
  nsresult GetNodeType(uint16_t* aNodeType)
  {
    *aNodeType = NodeType();
    return NS_OK;
  }
  nsresult GetNodeValue(nsAString& aNodeValue);
  nsresult SetNodeValue(const nsAString& aNodeValue);
  nsresult GetAttributes(nsIDOMNamedNodeMap** aAttributes)
  {
    NS_ENSURE_ARG_POINTER(aAttributes);
    *aAttributes = nullptr;
    return NS_OK;
  }
  nsresult HasChildNodes(bool* aHasChildNodes)
  {
    NS_ENSURE_ARG_POINTER(aHasChildNodes);
    *aHasChildNodes = false;
    return NS_OK;
  }
  nsresult HasAttributes(bool* aHasAttributes)
  {
    NS_ENSURE_ARG_POINTER(aHasAttributes);
    *aHasAttributes = false;
    return NS_OK;
  }
  nsresult InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aReturn);
  }
  nsresult ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aReturn);
  }
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    return nsINode::RemoveChild(aOldChild, aReturn);
  }
  nsresult AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    return InsertBefore(aNewChild, nullptr, aReturn);
  }
  nsresult GetNamespaceURI(nsAString& aNamespaceURI);
  nsresult GetLocalName(nsAString& aLocalName)
  {
    aLocalName = LocalName();
    return NS_OK;
  }
  nsresult GetPrefix(nsAString& aPrefix);
  nsresult IsSupported(const nsAString& aFeature,
                       const nsAString& aVersion,
                       bool* aReturn);
  nsresult CloneNode(bool aDeep, uint8_t aOptionalArgc, nsIDOMNode** aReturn)
  {
    if (!aOptionalArgc) {
      aDeep = true;
    }
    
    return nsNodeUtils::CloneNodeImpl(this, aDeep, true, aReturn);
  }

  
  nsresult GetData(nsAString& aData) const;
  nsresult SetData(const nsAString& aData);
  nsresult GetLength(uint32_t* aLength);
  nsresult SubstringData(uint32_t aOffset, uint32_t aCount,
                         nsAString& aReturn);
  nsresult AppendData(const nsAString& aArg);
  nsresult InsertData(uint32_t aOffset, const nsAString& aArg);
  nsresult DeleteData(uint32_t aOffset, uint32_t aCount);
  nsresult ReplaceData(uint32_t aOffset, uint32_t aCount,
                       const nsAString& aArg);

  
  virtual uint32_t GetChildCount() const;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const;
  virtual int32_t IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
  NS_IMETHOD GetTextContent(nsAString &aTextContent)
  {
    nsresult rv = GetNodeValue(aTextContent);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetNodeValue() failed?");
    return rv;
  }
  NS_IMETHOD SetTextContent(const nsAString& aTextContent)
  {
    
    mozAutoSubtreeModified subtree(OwnerDoc(), nullptr);
    return SetNodeValue(aTextContent);
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter);

  virtual nsIAtom *GetIDAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual bool GetAttr(int32_t aNameSpaceID, nsIAtom *aAttribute,
                         nsAString& aResult) const;
  virtual bool HasAttr(int32_t aNameSpaceID, nsIAtom *aAttribute) const;
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const;
  virtual uint32_t GetAttrCount() const;
  virtual const nsTextFragment *GetText();
  virtual uint32_t TextLength() const;
  virtual nsresult SetText(const PRUnichar* aBuffer, uint32_t aLength,
                           bool aNotify);
  
  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const PRUnichar* aBuffer, uint32_t aLength,
                              bool aNotify);
  virtual bool TextIsOnlyWhitespace();
  virtual void AppendTextTo(nsAString& aResult);
  virtual void DestroyContent();
  virtual void SaveSubtreeState();

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif

  virtual nsIContent *GetBindingParent() const;
  virtual bool IsNodeOfType(uint32_t aFlags) const;
  virtual bool IsLink(nsIURI** aURI) const;

  virtual nsIAtom* DoGetID() const;
  virtual const nsAttrValue* DoGetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  virtual nsIAtom *GetClassAttributeName() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
  {
    *aResult = CloneDataNode(aNodeInfo, true);
    if (!*aResult) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(*aResult);

    return NS_OK;
  }

  nsresult SplitData(uint32_t aOffset, nsIContent** aReturn,
                     bool aCloneAfterOriginal = true);

  

#ifdef DEBUG
  void ToCString(nsAString& aBuf, int32_t aOffset, int32_t aLen) const;
#endif

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsGenericDOMDataNode)

protected:
  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    nsINode *parent = GetNodeParent();

    return parent && parent->IsElement() ? parent->AsElement() : nullptr;
  }

  







  class nsDataSlots : public nsINode::nsSlots
  {
  public:
    nsDataSlots()
      : nsINode::nsSlots(),
        mBindingParent(nullptr)
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

  nsresult SplitText(uint32_t aOffset, nsIDOMText** aReturn);

  nsresult GetWholeText(nsAString& aWholeText);

  static int32_t FirstLogicallyAdjacentTextNode(nsIContent* aParent,
                                                int32_t aIndex);

  static int32_t LastLogicallyAdjacentTextNode(nsIContent* aParent,
                                               int32_t aIndex,
                                               uint32_t aCount);

  nsresult SetTextInternal(uint32_t aOffset, uint32_t aCount,
                           const PRUnichar* aBuffer, uint32_t aLength,
                           bool aNotify,
                           CharacterDataChangeInfo::Details* aDetails = nullptr);

  







  virtual nsGenericDOMDataNode *CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const = 0;

  nsTextFragment mText;

public:
  virtual bool OwnedOnlyByTheDOMTree()
  {
    return GetParent() && mRefCnt.get() == 1;
  }

  virtual bool IsPurple()
  {
    return mRefCnt.IsPurple();
  }
  virtual void RemovePurple()
  {
    mRefCnt.RemovePurple();
  }
  
private:
  already_AddRefed<nsIAtom> GetCurrentValueAtom();
};

#endif 
