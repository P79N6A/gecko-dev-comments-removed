




#ifndef MOZILLA_DOMSVGSTRINGLIST_H__
#define MOZILLA_DOMSVGSTRINGLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class ErrorResult;
class SVGStringList;



























class DOMSVGStringList MOZ_FINAL : public nsISupports
                                 , public nsWrapperCache
{
  friend class AutoChangeStringListNotifier;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGStringList)

  nsSVGElement* GetParentObject() const
  {
    return mElement;
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t NumberOfItems() const;
  uint32_t Length() const;
  void Clear();
  void Initialize(const nsAString& aNewItem, nsAString& aRetval,
                  ErrorResult& aRv);
  void GetItem(uint32_t aIndex, nsAString& aRetval, ErrorResult& aRv);
  void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aRetval);
  void InsertItemBefore(const nsAString& aNewItem, uint32_t aIndex,
                        nsAString& aRetval, ErrorResult& aRv);
  void ReplaceItem(const nsAString& aNewItem, uint32_t aIndex,
                   nsAString& aRetval, ErrorResult& aRv);
  void RemoveItem(uint32_t aIndex, nsAString& aRetval, ErrorResult& aRv);
  void AppendItem(const nsAString& aNewItem, nsAString& aRetval,
                  ErrorResult& aRv);

  









  static already_AddRefed<DOMSVGStringList>
    GetDOMWrapper(SVGStringList *aList,
                  nsSVGElement *aElement,
                  bool aIsConditionalProcessingAttribute,
                  uint8_t aAttrEnum);

private:
  



  DOMSVGStringList(nsSVGElement *aElement,
                   bool aIsConditionalProcessingAttribute, uint8_t aAttrEnum)
    : mElement(aElement)
    , mAttrEnum(aAttrEnum)
    , mIsConditionalProcessingAttribute(aIsConditionalProcessingAttribute)
  {
    SetIsDOMBinding();
  }

  ~DOMSVGStringList();

  SVGStringList &InternalList() const;

  
  nsRefPtr<nsSVGElement> mElement;

  uint8_t mAttrEnum;

  bool    mIsConditionalProcessingAttribute;
};

} 

#endif 
