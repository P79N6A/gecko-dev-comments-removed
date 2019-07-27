









#ifndef mozilla_dom_Attr_h
#define mozilla_dom_Attr_h

#include "mozilla/Attributes.h"
#include "nsIAttribute.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"

class nsIDocument;

namespace mozilla {
class EventChainPreVisitor;
namespace dom {



class Attr final : public nsIAttribute,
                   public nsIDOMAttr
{
  virtual ~Attr() {}

public:
  Attr(nsDOMAttributeMap* aAttrMap,
       already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
       const nsAString& aValue,
       bool aNsAware);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  virtual void GetTextContentInternal(nsAString& aTextContent,
                                      ErrorResult& aError) override;
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      ErrorResult& aError) override;
  virtual void GetNodeValueInternal(nsAString& aNodeValue) override;
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    ErrorResult& aError) override;

  
  NS_DECL_NSIDOMATTR

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;

  
  void SetMap(nsDOMAttributeMap *aMap) override;
  Element* GetElement() const;
  nsresult SetOwnerDocument(nsIDocument* aDocument) override;

  
  virtual bool IsNodeOfType(uint32_t aFlags) const override;
  virtual uint32_t GetChildCount() const override;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const override;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const override;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const override;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) override;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;
  virtual already_AddRefed<nsIURI> GetBaseURI(bool aTryUseXHRDocBaseURI = false) const override;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Attr,
                                                                   nsIAttribute)

  virtual nsIDOMNode* AsDOMNode() override { return this; }

  
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  

  void SetValue(const nsAString& aValue, ErrorResult& aRv);

  bool Specified() const;

  
  
  

  Element* GetOwnerElement(ErrorResult& aRv);

  bool IsNSAware() const { return mNsAware; }

protected:
  virtual Element* GetNameSpaceElement() override
  {
    return GetElement();
  }

  static bool sInitialized;

private:
  already_AddRefed<nsIAtom> GetNameAtom(nsIContent* aContent);

  nsString mValue;
};

} 
} 

#endif 
