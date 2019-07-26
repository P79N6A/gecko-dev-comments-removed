








#ifndef mozilla_dom_Attr_h
#define mozilla_dom_Attr_h

#include "nsIAttribute.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsINodeInfo.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"

namespace mozilla {
namespace dom {



class Attr : public nsIAttribute,
             public nsIDOMAttr
{
public:
  Attr(nsDOMAttributeMap* aAttrMap,
       already_AddRefed<nsINodeInfo> aNodeInfo,
       const nsAString& aValue,
       bool aNsAware);
  virtual ~Attr() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  virtual void GetTextContentInternal(nsAString& aTextContent);
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      ErrorResult& aError);
  virtual void GetNodeValueInternal(nsAString& aNodeValue);
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    ErrorResult& aError);

  
  NS_DECL_NSIDOMATTR

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  
  void SetMap(nsDOMAttributeMap *aMap);
  nsIContent *GetContent() const;
  nsresult SetOwnerDocument(nsIDocument* aDocument);

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;
  virtual uint32_t GetChildCount() const;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const MOZ_OVERRIDE;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual already_AddRefed<nsIURI> GetBaseURI() const;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Attr,
                                                                   nsIAttribute)

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  

  void SetValue(const nsAString& aValue, ErrorResult& aRv);

  bool Specified() const;

  
  
  

  Element* GetOwnerElement(ErrorResult& aRv);

protected:
  virtual Element* GetNameSpaceElement()
  {
    return GetContentInternal();
  }

  static bool sInitialized;

private:
  already_AddRefed<nsIAtom> GetNameAtom(nsIContent* aContent);
  Element* GetContentInternal() const;

  nsString mValue;
};

} 
} 

#endif 
