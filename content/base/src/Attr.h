








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
#include "nsIDocument.h"

namespace mozilla {
class EventChainPreVisitor;
namespace dom {



class Attr MOZ_FINAL : public nsIAttribute,
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
                                      ErrorResult& aError) MOZ_OVERRIDE;
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      ErrorResult& aError) MOZ_OVERRIDE;
  virtual void GetNodeValueInternal(nsAString& aNodeValue) MOZ_OVERRIDE;
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMATTR

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

  
  void SetMap(nsDOMAttributeMap *aMap) MOZ_OVERRIDE;
  Element* GetElement() const;
  nsresult SetOwnerDocument(nsIDocument* aDocument) MOZ_OVERRIDE;

  
  virtual bool IsNodeOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual uint32_t GetChildCount() const MOZ_OVERRIDE;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const MOZ_OVERRIDE;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const MOZ_OVERRIDE;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
  virtual already_AddRefed<nsIURI> GetBaseURI(bool aTryUseXHRDocBaseURI = false) const MOZ_OVERRIDE;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Attr,
                                                                   nsIAttribute)

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  
  static Attr* FromDOMAttr(nsIDOMAttr* aDOMAttr)
  {
    nsCOMPtr<nsIAttribute> iattr = do_QueryInterface(aDOMAttr);
    return static_cast<mozilla::dom::Attr*>(iattr.get());
  }

  
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  

  void SetValue(const nsAString& aValue, ErrorResult& aRv);

  bool Specified() const;

  
  
  

  Element* GetOwnerElement(ErrorResult& aRv);

protected:
  virtual Element* GetNameSpaceElement()
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
