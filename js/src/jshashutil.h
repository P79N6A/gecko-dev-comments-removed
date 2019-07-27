





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
      , originalGcNumber(cx->zone()->gcNumber())
    {}

    template <class KeyInput, class ValueInput>
    bool add(const ExclusiveContext *cx, T &table, const KeyInput &key, const ValueInput &value) {
        bool gcHappened = originalGcNumber != cx->zone()->gcNumber();
        if (gcHappened)
            addPtr = table.lookupForAdd(key);
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
    const uint64_t originalGcNumber;

    DependentAddPtr() MOZ_DELETE;
    DependentAddPtr(const DependentAddPtr&) MOZ_DELETE;
    DependentAddPtr& operator=(const DependentAddPtr&) MOZ_DELETE;
};

} 

#endif
