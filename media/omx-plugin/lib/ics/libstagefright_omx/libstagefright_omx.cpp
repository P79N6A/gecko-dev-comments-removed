




#include "mozilla/Types.h"
#define STAGEFRIGHT_EXPORT __attribute__ ((visibility ("default")))
#include "OMX.h"

namespace android {
MOZ_EXPORT_API(OMX)::OMX()
{
}

MOZ_EXPORT_API(OMX)::~OMX()
{
}

OMX foo;
}
