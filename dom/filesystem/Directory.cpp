





#include "mozilla/dom/Directory.h"

#include "nsString.h"
#include "mozilla/dom/DirectoryBinding.h"




#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Directory)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Directory)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Directory)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Directory)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Directory::Directory()
{
  SetIsDOMBinding();
}

Directory::~Directory()
{
}

nsPIDOMWindow*
Directory::GetParentObject() const
{
  
  return nullptr;
}

JSObject*
Directory::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DirectoryBinding::Wrap(aCx, aScope, this);
}

void
Directory::GetName(nsString& aRetval) const
{
  aRetval.Truncate();
  
}

already_AddRefed<Promise>
Directory::CreateDirectory(const nsAString& aPath)
{
  
  return nullptr;
}

already_AddRefed<Promise>
Directory::Get(const nsAString& aPath)
{
  
  return nullptr;
}

} 
} 
