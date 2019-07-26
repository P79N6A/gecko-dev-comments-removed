





#ifndef mozilla_dom_Directory_h
#define mozilla_dom_Directory_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"




#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace mozilla {
namespace dom {

class Promise;

class Directory MOZ_FINAL
  : public nsISupports
  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Directory)

public:
  Directory();
  ~Directory();

  

  nsPIDOMWindow*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void
  GetName(nsString& aRetval) const;

  already_AddRefed<Promise>
  CreateDirectory(const nsAString& aPath);

  already_AddRefed<Promise>
  Get(const nsAString& aPath);

  
};

} 
} 

#endif 
