








#ifndef nsDOMAttribute_h___
#define nsDOMAttribute_h___

#include "nsIAttribute.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsINodeInfo.h"
#include "nsDOMAttributeMap.h"
#include "nsCycleCollectionParticipant.h"
#include "nsStubMutationObserver.h"



class nsDOMAttribute : public nsIAttribute,
                       public nsIDOMAttr
{
public:
  nsDOMAttribute(nsDOMAttributeMap* aAttrMap,
                 already_AddRefed<nsINodeInfo> aNodeInfo,
                 const nsAString& aValue,
                 bool aNsAware);
  virtual ~nsDOMAttribute() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNODE

  
  NS_DECL_NSIDOMATTR

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  
  void SetMap(nsDOMAttributeMap *aMap);
  nsIContent *GetContent() const;
  nsresult SetOwnerDocument(nsIDocument* aDocument);

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;
  virtual uint32_t GetChildCount() const;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const;
  virtual int32_t IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual already_AddRefed<nsIURI> GetBaseURI() const;

  static void Initialize();
  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMAttribute,
                                                         nsIAttribute)

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    return GetContentInternal();
  }

  static bool sInitialized;

private:
  already_AddRefed<nsIAtom> GetNameAtom(nsIContent* aContent);
  mozilla::dom::Element *GetContentInternal() const
  {
    return mAttrMap ? mAttrMap->GetContent() : nullptr;
  }

  nsString mValue;
};


#endif 
