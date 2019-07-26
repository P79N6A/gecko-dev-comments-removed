





#ifndef mozilla_dom_HTMLAllCollection_h
#define mozilla_dom_HTMLAllCollection_h

#include "js/RootingAPI.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"

#include <stdint.h>

class nsContentList;
class nsHTMLDocument;
class nsIContent;
class nsWrapperCache;

namespace mozilla {
class ErrorResult;

namespace dom {

class HTMLAllCollection
{
public:
  HTMLAllCollection(nsHTMLDocument* aDocument);
  ~HTMLAllCollection();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(HTMLAllCollection)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(HTMLAllCollection)

  uint32_t Length();
  nsIContent* Item(uint32_t aIndex);

  JSObject* GetObject(JSContext* aCx, ErrorResult& aRv);

  nsISupports* GetNamedItem(const nsAString& aID, nsWrapperCache** aCache);

private:
  nsContentList* Collection();

  JS::Heap<JSObject*> mObject;
  nsRefPtr<nsHTMLDocument> mDocument;
  nsRefPtr<nsContentList> mCollection;
};

} 
} 

#endif 
