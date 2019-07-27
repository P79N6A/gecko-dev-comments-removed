





#ifndef ds_InlineMap_h
#define ds_InlineMap_h

#include "jsalloc.h"

#include "js/HashTable.h"

namespace js {





template <typename T> struct ZeroIsReserved         { static const bool result = false; };
template <typename T> struct ZeroIsReserved<T*>    { static const bool result = true; };

template <typename K, typename V, size_t InlineElems>
class InlineMap
{
  public:
    typedef HashMap<K, V, DefaultHasher<K>, SystemAllocPolicy> WordMap;

    struct InlineElem
    {
        K key;
        V value;
    };

  private:
    typedef typename WordMap::Ptr       WordMapPtr;
    typedef typename WordMap::AddPtr    WordMapAddPtr;
    typedef typename WordMap::Range     WordMapRange;

    size_t          inlNext;
    size_t          inlCount;
    InlineElem      inl[InlineElems];
    WordMap         map;

    static_assert(ZeroIsReserved<K>::result,
                  "zero as tombstone requires that zero keys be invalid");

    bool usingMap() const {
        return inlNext > InlineElems;
    }

    bool switchToMap() {
        MOZ_ASSERT(inlNext == InlineElems);

        if (map.initialized()) {
            map.clear();
        } else {
            if (!map.init(count()))
                return false;
            MOZ_ASSERT(map.initialized());
        }

        InlineElem* end = inl + inlNext;
        for (InlineElem* it = inl; it != end; ++it) {
            if (it->key && !map.putNew(it->key, it->value))
                return false;
        }

        inlNext = InlineElems + 1;
        MOZ_ASSERT(map.count() == inlCount);
        MOZ_ASSERT(usingMap());
        return true;
    }

    MOZ_NEVER_INLINE
    bool switchAndAdd(const K& key, const V& value) {
        if (!switchToMap())
            return false;

        return map.putNew(key, value);
    }

  public:
    explicit InlineMap() : inlNext(0), inlCount(0) { }

    class Entry
    {
        friend class InlineMap;
        const K& key_;
        V& value_;

        Entry(const K& key, V& value) : key_(key), value_(value) {}

      public:
        const K& key() { return key_; }
        V& value() { return value_; }
    }; 

    class Ptr
    {
        friend class InlineMap;

        WordMapPtr  mapPtr;
        InlineElem* inlPtr;
        bool        isInlinePtr;

        explicit Ptr(WordMapPtr p) : mapPtr(p), isInlinePtr(false) {}
        explicit Ptr(InlineElem* ie) : inlPtr(ie), isInlinePtr(true) {}
        void operator==(const Ptr& other);

      public:
        
        Ptr() {
#ifdef DEBUG
            inlPtr = (InlineElem*) 0xbad;
            isInlinePtr = true;
#endif
        }

        

        bool found() const {
            return isInlinePtr ? bool(inlPtr) : mapPtr.found();
        }

        explicit operator bool() const {
            return found();
        }

        K& key() {
            MOZ_ASSERT(found());
            return isInlinePtr ? inlPtr->key : mapPtr->key();
        }

        V& value() {
            MOZ_ASSERT(found());
            return isInlinePtr ? inlPtr->value : mapPtr->value();
        }
    }; 

    class AddPtr
    {
        friend class InlineMap;

        WordMapAddPtr   mapAddPtr;
        InlineElem*     inlAddPtr;
        bool            isInlinePtr;
        
        bool            inlPtrFound;

        AddPtr(InlineElem* ptr, bool found)
          : inlAddPtr(ptr), isInlinePtr(true), inlPtrFound(found)
        {}

        explicit AddPtr(const WordMapAddPtr& p) : mapAddPtr(p), isInlinePtr(false) {}

        void operator==(const AddPtr& other);

      public:
        AddPtr() {}

        bool found() const {
            return isInlinePtr ? inlPtrFound : mapAddPtr.found();
        }

        explicit operator bool() const {
            return found();
        }

        V& value() {
            MOZ_ASSERT(found());
            if (isInlinePtr)
                return inlAddPtr->value;
            return mapAddPtr->value();
        }
    }; 

    size_t count() {
        return usingMap() ? map.count() : inlCount;
    }

    bool empty() const {
        return usingMap() ? map.empty() : !inlCount;
    }

    void clear() {
        inlNext = 0;
        inlCount = 0;
    }

    bool isMap() const {
        return usingMap();
    }

    const WordMap& asMap() const {
        MOZ_ASSERT(isMap());
        return map;
    }

    const InlineElem* asInline() const {
        MOZ_ASSERT(!isMap());
        return inl;
    }

    const InlineElem* inlineEnd() const {
        MOZ_ASSERT(!isMap());
        return inl + inlNext;
    }

    MOZ_ALWAYS_INLINE
    Ptr lookup(const K& key) {
        if (usingMap())
            return Ptr(map.lookup(key));

        InlineElem* end = inl + inlNext;
        for (InlineElem* it = inl; it != end; ++it) {
            if (it->key == key)
                return Ptr(it);
        }

        return Ptr(nullptr);
    }

    MOZ_ALWAYS_INLINE
    AddPtr lookupForAdd(const K& key) {
        if (usingMap())
            return AddPtr(map.lookupForAdd(key));

        InlineElem* end = inl + inlNext;
        for (InlineElem* it = inl; it != end; ++it) {
            if (it->key == key)
                return AddPtr(it, true);
        }

        




        return AddPtr(inl + inlNext, false);
    }

    MOZ_ALWAYS_INLINE
    bool add(AddPtr& p, const K& key, const V& value) {
        MOZ_ASSERT(!p);

        if (p.isInlinePtr) {
            InlineElem* addPtr = p.inlAddPtr;
            MOZ_ASSERT(addPtr == inl + inlNext);

            
            if (addPtr == inl + InlineElems)
                return switchAndAdd(key, value);

            MOZ_ASSERT(!p.found());
            MOZ_ASSERT(uintptr_t(inl + inlNext) == uintptr_t(p.inlAddPtr));
            p.inlAddPtr->key = key;
            p.inlAddPtr->value = value;
            ++inlCount;
            ++inlNext;
            return true;
        }

        return map.add(p.mapAddPtr, key, value);
    }

    MOZ_ALWAYS_INLINE
    bool put(const K& key, const V& value) {
        AddPtr p = lookupForAdd(key);
        if (p) {
            p.value() = value;
            return true;
        }
        return add(p, key, value);
    }

    void remove(Ptr p) {
        MOZ_ASSERT(p);
        if (p.isInlinePtr) {
            MOZ_ASSERT(inlCount > 0);
            MOZ_ASSERT(p.inlPtr->key != nullptr);
            p.inlPtr->key = nullptr;
            --inlCount;
            return;
        }
        MOZ_ASSERT(map.initialized() && usingMap());
        map.remove(p.mapPtr);
    }

    void remove(const K& key) {
        if (Ptr p = lookup(key))
            remove(p);
    }

    class Range
    {
        friend class InlineMap;

        WordMapRange    mapRange;
        InlineElem*     cur;
        InlineElem*     end;
        bool            isInline;

        explicit Range(WordMapRange r)
          : cur(nullptr), end(nullptr), 
            isInline(false) {
            mapRange = r;
            MOZ_ASSERT(!isInlineRange());
        }

        Range(const InlineElem* begin, const InlineElem* end_)
          : cur(const_cast<InlineElem*>(begin)),
            end(const_cast<InlineElem*>(end_)),
            isInline(true) {
            advancePastNulls(cur);
            MOZ_ASSERT(isInlineRange());
        }

        bool checkInlineRangeInvariants() const {
            MOZ_ASSERT(uintptr_t(cur) <= uintptr_t(end));
            MOZ_ASSERT_IF(cur != end, cur->key != nullptr);
            return true;
        }

        bool isInlineRange() const {
            MOZ_ASSERT_IF(isInline, checkInlineRangeInvariants());
            return isInline;
        }

        void advancePastNulls(InlineElem* begin) {
            InlineElem* newCur = begin;
            while (newCur < end && nullptr == newCur->key)
                ++newCur;
            MOZ_ASSERT(uintptr_t(newCur) <= uintptr_t(end));
            cur = newCur;
        }

        void bumpCurPtr() {
            MOZ_ASSERT(isInlineRange());
            advancePastNulls(cur + 1);
        }

        void operator==(const Range& other);

      public:
        bool empty() const {
            return isInlineRange() ? cur == end : mapRange.empty();
        }

        Entry front() {
            MOZ_ASSERT(!empty());
            if (isInlineRange())
                return Entry(cur->key, cur->value);
            return Entry(mapRange.front().key(), mapRange.front().value());
        }

        void popFront() {
            MOZ_ASSERT(!empty());
            if (isInlineRange())
                bumpCurPtr();
            else
                mapRange.popFront();
        }
    }; 

    Range all() const {
        return usingMap() ? Range(map.all()) : Range(inl, inl + inlNext);
    }
}; 

} 

#endif 
