





#ifndef mozilla_HalImpl_h
#define mozilla_HalImpl_h

#ifdef MOZ_UNIFIED_BUILD
#error Cannot use HalImpl.h in unified builds.
#endif

#define MOZ_HAL_NAMESPACE hal_impl
#undef mozilla_Hal_h
#undef mozilla_HalInternal_h
#include "Hal.h"
#include "HalInternal.h"
#undef MOZ_HAL_NAMESPACE

#endif 
