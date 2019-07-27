









#ifndef nsGenericDOMDataNode_h___
#define nsGenericDOMDataNode_h___

#include "mozilla/Attributes.h"
#include "nsIContent.h"

#include "nsTextFragment.h"
#include "nsError.h"
#include "mozilla/dom/Element.h"
#include "nsCycleCollectionParticipant.h"

#include "nsISMILAttr.h"
#include "mozilla/dom/ShadowRoot.h"

class nsIDocument;
class nsIDOMText;

#define DATA_NODE_FLAG_BIT(n_) NODE_FLAG_BIT(NODE_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  
  
  
  NS_CREATE_FRAME_IF_NON_WHITESPACE =     DATA_NODE_FLAG_BIT(0),

  
  
  NS_REFRAME_IF_WHITESPACE =              DATA_NODE_FLAG_BIT(1),

  
  
  NS_CACHED_TEXT_IS_ONLY_WHITESPACE =     DATA_NODE_FLAG_BIT(2),

  
  
  
  NS_TEXT_IS_ONLY_WHITESPACE =            DATA_NODE_FLAG_BIT(3),
};


ASSERT_NODE_FLAGS_SPACE(NODE_TYPE_SPECIFIC_BITS_OFFSET + 4);

#undef DATA_NODE_FLAG_BIT

class nsGenericDOMDataNode : public nsIContent
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  explicit nsGenericDOMDataNode(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  explicit nsGenericDOMDataNode(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

  virtual void GetNodeValueInternal(nsAString& aNodeValue) override;
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError) override;

  
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
  NS_IMETHOD MozRemove();

  
  virtual uint32_t GetChildCount() const override;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const override;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const override;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const override;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) override;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;
  virtual void GetTextContentInternal(nsAString& aTextContent,
                                      mozilla::ErrorResult& aError) override
  {
    GetNodeValue(aTextContent);
  }
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      mozilla::ErrorResult& aError) override
  {
    
    mozAutoSubtreeModified subtree(OwnerDoc(), nullptr);
    return SetNodeValue(aTextContent, aError);
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter) override;


  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) override;
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const override;
  virtual uint32_t GetAttrCount() const override;
  virtual const nsTextFragment *GetText() override;
  virtual uint32_t TextLength() const override;
  virtual nsresult SetText(const char16_t* aBuffer, uint32_t aLength,
                           bool aNotify) override;
  
  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const char16_t* aBuffer, uint32_t aLength,
                              bool aNotify) override;
  virtual bool TextIsOnlyWhitespace() override;
  virtual bool HasTextForTranslation() override;
  virtual void AppendTextTo(nsAString& aResult) override;
  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendTextTo(nsAString& aResult,
                            const mozilla::fallible_t&) override;
  virtual void DestroyContent() override;
  virtual void SaveSubtreeState() override;

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const override;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const override;
#endif

  virtual nsIContent *GetBindingParent() const override;
  virtual nsXBLBinding *GetXBLBinding() const override;
  virtual void SetXBLBinding(nsXBLBinding* aBinding,
                             nsBindingManager* aOldBindingManager = nullptr) override;
  virtual mozilla::dom::ShadowRoot *GetContainingShadow() const override;
  virtual mozilla::dom::ShadowRoot *GetShadowRoot() const override;
  virtual nsTArray<nsIContent*> &DestInsertionPoints() override;
  virtual nsTArray<nsIContent*> *GetExistingDestInsertionPoints() const override;
  virtual void SetShadowRoot(mozilla::dom::ShadowRoot* aShadowRoot) override;
  virtual nsIContent *GetXBLInsertionParent() const override;
  virtual void SetXBLInsertionParent(nsIContent* aContent) override;
  virtual bool IsNodeOfType(uint32_t aFlags) const override;
  virtual bool IsLink(nsIURI** aURI) const override;

  virtual mozilla::dom::CustomElementData* GetCustomElementData() const override;
  virtual void SetCustomElementData(mozilla::dom::CustomElementData* aData) override;

  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override
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

  
  
  virtual void SetData(const nsAString& aData, mozilla::ErrorResult& rv)
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
  virtual ~nsGenericDOMDataNode();

  virtual mozilla::dom::Element* GetNameSpaceElement() override
  {
    nsINode *parent = GetParentNode();

    return parent && parent->IsElement() ? parent->AsElement() : nullptr;
  }

  







  class nsDataSlots : public nsINode::nsSlots
  {
  public:
    nsDataSlots();

    void Traverse(nsCycleCollectionTraversalCallback &cb);
    void Unlink();

    



    nsIContent* mBindingParent;  

    


    nsCOMPtr<nsIContent> mXBLInsertionParent;

    


    nsRefPtr<mozilla::dom::ShadowRoot> mContainingShadow;

    


    nsTArray<nsIContent*> mDestInsertionPoints;
  };

  
  virtual nsINode::nsSlots* CreateSlots() override;

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
                           const char16_t* aBuffer, uint32_t aLength,
                           bool aNotify,
                           CharacterDataChangeInfo::Details* aDetails = nullptr);

  







  virtual nsGenericDOMDataNode *CloneDataNode(mozilla::dom::NodeInfo *aNodeInfo,
                                              bool aCloneText) const = 0;

  nsTextFragment mText;

public:
  virtual bool OwnedOnlyByTheDOMTree() override
  {
    return GetParent() && mRefCnt.get() == 1;
  }

  virtual bool IsPurple() override
  {
    return mRefCnt.IsPurple();
  }
  virtual void RemovePurple() override
  {
    mRefCnt.RemovePurple();
  }
  
private:
  already_AddRefed<nsIAtom> GetCurrentValueAtom();
};

#endif 
