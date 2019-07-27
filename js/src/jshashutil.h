





#ifndef jshashutil_h
#define jshashutil_h

#include "jscntxt.h"

#include "gc/Zone.h"

namespace js {






template <class T>
struct DependentAddPtr
{
    typedef typename T::AddPtr AddPtr;
    typedef typename T::Entry Entry;

    template <class Lookup>
    DependentAddPtr(const ExclusiveContext* cx, const T& table, const Lookup& lookup)
      : addPtr(table.lookupForAdd(lookup))
      , originalGcNumber(cx->zone()->gcNumber())
    {}

    template <class KeyInput, class ValueInput>
    bool add(ExclusiveContext* cx, T& table, const KeyInput& key, const ValueInput& value) {
        bool gcHappened = originalGcNumber != cx->zone()->gcNumber();
        if (gcHappened)
            addPtr = table.lookupForAdd(key);
        if (!table.relookupOrAdd(addPtr, key, value)) {
            ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }


    bool found() const                 { return addPtr.found(); }
    explicit operator bool() const     { return found(); }
    const Entry& operator*() const     { return *addPtr; }
    const Entry* operator->() const    { return &*addPtr; }

  private:
    AddPtr addPtr ;
    const uint64_t originalGcNumber;

    DependentAddPtr() = delete;
    DependentAddPtr(const DependentAddPtr&) = delete;
    DependentAddPtr& operator=(const DependentAddPtr&) = delete;
};

} 

#endif
