




#ifndef mozilla_dom_FetchIPCUtils_h
#define mozilla_dom_FetchIPCUtils_h

#include "ipc/IPCMessageUtils.h"


#undef None

#include "mozilla/dom/HeadersBinding.h"
#include "mozilla/dom/Request.h"
#include "mozilla/dom/Response.h"

namespace IPC {
  template<>
  struct ParamTraits<mozilla::dom::HeadersGuardEnum> :
    public ContiguousEnumSerializer<mozilla::dom::HeadersGuardEnum,
                                    mozilla::dom::HeadersGuardEnum::None,
                                    mozilla::dom::HeadersGuardEnum::EndGuard_> {};
  template<>
  struct ParamTraits<mozilla::dom::RequestMode> :
    public ContiguousEnumSerializer<mozilla::dom::RequestMode,
                                    mozilla::dom::RequestMode::Same_origin,
                                    mozilla::dom::RequestMode::EndGuard_> {};
  template<>
  struct ParamTraits<mozilla::dom::RequestCredentials> :
    public ContiguousEnumSerializer<mozilla::dom::RequestCredentials,
                                    mozilla::dom::RequestCredentials::Omit,
                                    mozilla::dom::RequestCredentials::EndGuard_> {};
  template<>
  struct ParamTraits<mozilla::dom::ResponseType> :
    public ContiguousEnumSerializer<mozilla::dom::ResponseType,
                                    mozilla::dom::ResponseType::Basic,
                                    mozilla::dom::ResponseType::EndGuard_> {};
}

#endif 
