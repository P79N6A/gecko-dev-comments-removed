





#ifndef jshashutil_h
#define jshashutil_h

#include "jscntxt.h"

namespace js {






template <class T>
struct DependentAddPtr
{
    typedef typename T::AddPtr AddPtr;
    typedef typename T::Entry Entry;

    template <class Lookup>
    DependentAddPtr(const ExclusiveContext *cx, const T &table, const Lookup &lookup)
      : addPtr(table.lookupForAdd(lookup))
#ifdef JSGC_GENERATIONAL
      , cx(cx)
      , originalGcNumber(cx->zone()->gcNumber())
#endif
        {}

    template <class KeyInput, class ValueInput>
    bool add(T &table, const KeyInput &key, const ValueInput &value) {
#ifdef JSGC_GENERATIONAL
        bool gcHappened = originalGcNumber != cx->zone()->gcNumber();
        if (gcHappened)
            return table.putNew(key, value);
#endif
        return table.relookupOrAdd(addPtr, key, value);
    }

    typedef void (DependentAddPtr::* ConvertibleToBool)();
    void nonNull() {}

    bool found() const                 { return addPtr.found(); }
    operator ConvertibleToBool() const { return found() ? &DependentAddPtr::nonNull : 0; }
    const Entry &operator*() const     { return *addPtr; }
    const Entry *operator->() const    { return &*addPtr; }

  private:
    AddPtr addPtr ;
#ifdef JSGC_GENERATIONAL
    const ExclusiveContext *cx;
    const uint64_t originalGcNumber;
#endif

    DependentAddPtr() MOZ_DELETE;
    DependentAddPtr(const DependentAddPtr&) MOZ_DELETE;
    DependentAddPtr& operator=(const DependentAddPtr&) MOZ_DELETE;
};

} 

#endif
