





#ifndef mozilla_dom_asmjscache_asmjscache_h
#define mozilla_dom_asmjscache_asmjscache_h

#include "ipc/IPCMessageUtils.h"
#include "js/TypeDecls.h"
#include "js/Vector.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {

namespace quota {
class Client;
}

namespace asmjscache {

class PAsmJSCacheEntryChild;
class PAsmJSCacheEntryParent;

enum OpenMode
{
  eOpenForRead,
  eOpenForWrite,
  NUM_OPEN_MODES
};













bool
OpenEntryForRead(nsIPrincipal* aPrincipal,
                 const jschar* aBegin,
                 const jschar* aLimit,
                 size_t* aSize,
                 const uint8_t** aMemory,
                 intptr_t *aHandle);
void
CloseEntryForRead(JS::Handle<JSObject*> aGlobal,
                  size_t aSize,
                  const uint8_t* aMemory,
                  intptr_t aHandle);
bool
OpenEntryForWrite(nsIPrincipal* aPrincipal,
                  const jschar* aBegin,
                  const jschar* aEnd,
                  size_t aSize,
                  uint8_t** aMemory,
                  intptr_t* aHandle);
void
CloseEntryForWrite(JS::Handle<JSObject*> aGlobal,
                   size_t aSize,
                   uint8_t* aMemory,
                   intptr_t aHandle);
bool
GetBuildId(js::Vector<char>* aBuildId);



quota::Client*
CreateClient();



PAsmJSCacheEntryParent*
AllocEntryParent(OpenMode aOpenMode, uint32_t aSizeToWrite,
                 nsIPrincipal* aPrincipal);

void
DeallocEntryParent(PAsmJSCacheEntryParent* aActor);



void
DeallocEntryChild(PAsmJSCacheEntryChild* aActor);

} 
} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::dom::asmjscache::OpenMode> :
  public EnumSerializer<mozilla::dom::asmjscache::OpenMode,
                        mozilla::dom::asmjscache::eOpenForRead,
                        mozilla::dom::asmjscache::NUM_OPEN_MODES>
{ };

} 

#endif  
