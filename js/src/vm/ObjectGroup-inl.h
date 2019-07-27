





#ifndef vm_ObjectGroup_inl_h
#define vm_ObjectGroup_inl_h

#include "vm/ObjectGroup.h"

namespace js {

inline bool
ObjectGroup::needsSweep()
{
    
    
    return generation() != zoneFromAnyThread()->types.generation;
}

inline void
ObjectGroup::maybeSweep(AutoClearTypeInferenceStateOnOOM* oom)
{
    if (needsSweep())
        sweep(oom);
}

inline ObjectGroupFlags
ObjectGroup::flags()
{
    maybeSweep(nullptr);
    return flagsDontCheckGeneration();
}

inline void
ObjectGroup::addFlags(ObjectGroupFlags flags)
{
    maybeSweep(nullptr);
    flags_ |= flags;
}

inline void
ObjectGroup::clearFlags(ObjectGroupFlags flags)
{
    maybeSweep(nullptr);
    flags_ &= ~flags;
}

inline bool
ObjectGroup::hasAnyFlags(ObjectGroupFlags flags)
{
    MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
    return !!(this->flags() & flags);
}

inline bool
ObjectGroup::hasAllFlags(ObjectGroupFlags flags)
{
    MOZ_ASSERT((flags & OBJECT_FLAG_DYNAMIC_MASK) == flags);
    return (this->flags() & flags) == flags;
}

inline bool
ObjectGroup::unknownProperties()
{
    MOZ_ASSERT_IF(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES,
                  hasAllFlags(OBJECT_FLAG_DYNAMIC_MASK));
    return !!(flags() & OBJECT_FLAG_UNKNOWN_PROPERTIES);
}

inline bool
ObjectGroup::shouldPreTenure()
{
    return hasAnyFlags(OBJECT_FLAG_PRE_TENURE) && !unknownProperties();
}

inline bool
ObjectGroup::canPreTenure()
{
    return !unknownProperties();
}

inline bool
ObjectGroup::fromAllocationSite()
{
    return flags() & OBJECT_FLAG_FROM_ALLOCATION_SITE;
}

inline void
ObjectGroup::setShouldPreTenure(ExclusiveContext* cx)
{
    MOZ_ASSERT(canPreTenure());
    setFlags(cx, OBJECT_FLAG_PRE_TENURE);
}

inline TypeNewScript*
ObjectGroup::newScript()
{
    maybeSweep(nullptr);
    return newScriptDontCheckGeneration();
}

inline PreliminaryObjectArrayWithTemplate*
ObjectGroup::maybePreliminaryObjects()
{
    maybeSweep(nullptr);
    return maybePreliminaryObjectsDontCheckGeneration();
}

inline UnboxedLayout*
ObjectGroup::maybeUnboxedLayout()
{
    maybeSweep(nullptr);
    return maybeUnboxedLayoutDontCheckGeneration();
}

inline UnboxedLayout&
ObjectGroup::unboxedLayout()
{
    maybeSweep(nullptr);
    return unboxedLayoutDontCheckGeneration();
}

} 

#endif 
