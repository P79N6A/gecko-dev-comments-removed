





#ifndef mozilla_dom_HTMLAllCollection_h
#define mozilla_dom_HTMLAllCollection_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "nsRefPtrHashtable.h"
#include "nsWrapperCache.h"

#include <stdint.h>

class nsContentList;
class nsHTMLDocument;
class nsIContent;
class nsINode;

namespace mozilla {
namespace dom {

class OwningNodeOrHTMLCollection;
template<typename> struct Nullable;

class HTMLAllCollection final : public nsISupports
                              , public nsWrapperCache
{
  ~HTMLAllCollection();

public:
  explicit HTMLAllCollection(nsHTMLDocument* aDocument);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(HTMLAllCollection)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
  nsINode* GetParentObject() const;

  uint32_t Length();
  nsIContent* Item(uint32_t aIndex);
  void Item(const nsAString& aName, Nullable<OwningNodeOrHTMLCollection>& aResult)
  {
    NamedItem(aName, aResult);
  }
  nsIContent* IndexedGetter(uint32_t aIndex, bool& aFound)
  {
    nsIContent* result = Item(aIndex);
    aFound = !!result;
    return result;
  }

  void NamedItem(const nsAString& aName,
                 Nullable<OwningNodeOrHTMLCollection>& aResult)
  {
    bool found = false;
    NamedGetter(aName, found, aResult);
  }
  void NamedGetter(const nsAString& aName,
                   bool& aFound,
                   Nullable<OwningNodeOrHTMLCollection>& aResult);
  void GetSupportedNames(unsigned aFlags, nsTArray<nsString>& aNames);
  bool NameIsEnumerable(const nsAString& aName)
  {
    return false;
  }
  void LegacyCall(JS::Handle<JS::Value>, const nsAString& aName,
                  Nullable<OwningNodeOrHTMLCollection>& aResult)
  {
    NamedItem(aName, aResult);
  }

private:
  nsContentList* Collection();

  


  nsContentList* GetDocumentAllList(const nsAString& aID);

  nsRefPtr<nsHTMLDocument> mDocument;
  nsRefPtr<nsContentList> mCollection;
  nsRefPtrHashtable<nsStringHashKey, nsContentList> mNamedMap;
};

} 
} 

#endif 
