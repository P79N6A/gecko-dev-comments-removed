




#include "mozilla/Types.h"
#include "utils/RefBase.h"
#include "utils/String16.h"
#include "utils/String8.h"

namespace android {
MOZ_EXPORT RefBase::RefBase() : mRefs(0)
{
}

MOZ_EXPORT RefBase::~RefBase()
{
}

MOZ_EXPORT void RefBase::incStrong(const void *id) const
{
}

MOZ_EXPORT void RefBase::decStrong(const void *id) const
{
}

MOZ_EXPORT void RefBase::onFirstRef()
{
}

MOZ_EXPORT void RefBase::onLastStrongRef(const void* id)
{
}

MOZ_EXPORT bool RefBase::onIncStrongAttempted(uint32_t flags, const void* id)
{
  return false;
}

MOZ_EXPORT void RefBase::onLastWeakRef(void const* id)
{
}

MOZ_EXPORT String16::String16(char const*)
{
}

MOZ_EXPORT String16::~String16()
{
}

MOZ_EXPORT String8::String8()
{
}

MOZ_EXPORT String8::~String8()
{
}
}
