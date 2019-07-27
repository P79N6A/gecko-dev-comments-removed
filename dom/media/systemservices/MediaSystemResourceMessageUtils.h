





#if !defined(MediaSystemResourceMessageUtils_h_)
#define MediaSystemResourceMessageUtils_h_

#include "ipc/IPCMessageUtils.h"
#include "MediaSystemResourceTypes.h"

namespace IPC {

template <>
struct ParamTraits<mozilla::MediaSystemResourceType>
  : public ContiguousEnumSerializer<
             mozilla::MediaSystemResourceType,
             mozilla::MediaSystemResourceType::VIDEO_DECODER,
             mozilla::MediaSystemResourceType::INVALID_RESOURCE>
{};

} 

#endif
