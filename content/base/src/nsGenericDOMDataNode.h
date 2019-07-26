









#ifndef nsGenericDOMDataNode_h___
#define nsGenericDOMDataNode_h___

#include "nsIContent.h"

#include "nsTextFragment.h"
#include "nsError.h"
#include "nsEventListenerManager.h"
#include "mozilla/dom/Element.h"
#include "nsCycleCollectionParticipant.h"

#include "nsISMILAttr.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIDOMNodeList;
class nsIFrame;
class nsIDOMText;
class nsINodeInfo;
class nsURI;

#define DATA_NODE_FLAG_BIT(n_) NODE_FLAG_BIT(NODE_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  
  
  
  NS_CREATE_FRAME_IF_NON_WHITESPACE =     DATA_NODE_FLAG_BIT(0),

  
  
  NS_REFRAME_IF_WHITESPACE =              DATA_NODE_FLAG_BIT(1)
};


PR_STATIC_ASSERT(NODE_TYPE_SPECIFIC_BITS_OFFSET + 1 < 32);

#undef DATA_NODE_FLAG_BIT

class nsGenericDOMDataNode : public nsIContent
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  nsGenericDOMDataNode(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsGenericDOMDataNode();

  virtual void GetNodeValueInternal(nsAString& aNodeValue);
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError);

  
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
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
  virtual void GetTextContentInternal(nsAString& aTextContent)
  {
    GetNodeValue(aTextContent);
  }
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      mozilla::ErrorResult& aError)
  {
    
    mozAutoSubtreeModified subtree(OwnerDoc(), nullptr);
    return SetNodeValue(aTextContent, aError);
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

  
  
  void SetData(const nsAString& aData, mozilla::ErrorResult& rv)
  {
    rv = SetData(aData);
  }
  
  void SubstringData(uint32_t aStart, uint32_t aCount, nsAString& aReturn,
                     mozilla::ErrorResult& rv);
  void AppendData(const nsAString& aData, mozilla::ErrorResult& rv)
  {
    rv = AppendData(aData);
  }
  void InsertData(uint32_t aOffset, const nsAString& aData,
                  mozilla::ErrorResult& rv)
  {
    rv = InsertData(aOffset, aData);
  }
  void DeleteData(uint32_t aOffset, uint32_t aCount, mozilla::ErrorResult& rv)
  {
    rv = DeleteData(aOffset, aCount);
  }
  void ReplaceData(uint32_t aOffset, uint32_t aCount, const nsAString& aData,
                   mozilla::ErrorResult& rv)
  {
    rv = ReplaceData(aOffset, aCount, aData);
  }

  

#ifdef DEBUG
  void ToCString(nsAString& aBuf, int32_t aOffset, int32_t aLen) const;
#endif

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsGenericDOMDataNode)

protected:
  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    nsINode *parent = GetParentNode();

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

  nsDataSlots* DataSlots()
  {
    return static_cast<nsDataSlots*>(Slots());
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
