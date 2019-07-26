





#ifndef mozilla_dom_HTMLAllCollection_h
#define mozilla_dom_HTMLAllCollection_h

#include "js/RootingAPI.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"

#include <stdint.h>

class nsContentList;
class nsHTMLDocument;

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

  JSObject* GetObject(JSContext* aCx, ErrorResult& aRv);

  nsContentList* Collection();

private:
  JS::Heap<JSObject*> mObject;
  nsRefPtr<nsHTMLDocument> mDocument;
  nsRefPtr<nsContentList> mCollection;
};

} 
} 

#endif 
