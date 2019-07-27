





#ifndef mozilla_dom_cache_IPCUtils_h
#define mozilla_dom_cache_IPCUtils_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/cache/Types.h"

namespace IPC {
  template<>
  struct ParamTraits<mozilla::dom::cache::Namespace> :
    public ContiguousEnumSerializer<mozilla::dom::cache::Namespace,
                                    mozilla::dom::cache::DEFAULT_NAMESPACE,
                                    mozilla::dom::cache::NUMBER_OF_NAMESPACES>
  {};
}

#endif 
