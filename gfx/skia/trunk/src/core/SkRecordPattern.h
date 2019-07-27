#ifndef SkRecordPattern_DEFINED
#define SkRecordPattern_DEFINED

#include "SkTLogic.h"

namespace SkRecords {





template <typename T>
class Is {
public:
    Is() : fPtr(NULL) {}

    typedef T type;
    type* get() { return fPtr; }

    bool operator()(T* ptr) {
        fPtr = ptr;
        return true;
    }

    template <typename U>
    bool operator()(U*) {
        fPtr = NULL;
        return false;
    }

private:
    type* fPtr;
};


class IsDraw {
    SK_CREATE_MEMBER_DETECTOR(paint);
public:
    IsDraw() : fPaint(NULL) {}

    typedef SkPaint type;
    type* get() { return fPaint; }

    template <typename T>
    SK_WHEN(HasMember_paint<T>, bool) operator()(T* draw) {
        fPaint = AsPtr(draw->paint);
        return true;
    }

    template <typename T>
    SK_WHEN(!HasMember_paint<T>, bool) operator()(T*) {
        fPaint = NULL;
        return false;
    }

    
    bool operator()(SaveLayer*) {
        fPaint = NULL;
        return false;
    }

private:
    
    template <typename T> static T* AsPtr(SkRecords::Optional<T>& x) { return x; }
    template <typename T> static T* AsPtr(T& x) { return &x; }

    type* fPaint;
};


template <typename Matcher>
struct Not {
    template <typename T>
    bool operator()(T* ptr) { return !Matcher()(ptr); }
};


template <typename A, typename B>
struct Or {
    template <typename T>
    bool operator()(T* ptr) { return A()(ptr) || B()(ptr); }
};


template <typename A, typename B, typename C>
struct Or3 : Or<A, Or<B, C> > {};


template <typename Matcher>
struct Star {
    template <typename T>
    bool operator()(T* ptr) { return Matcher()(ptr); }
};









template <typename Matcher, typename Pattern>
class Cons {
public:
    
    
    SK_ALWAYS_INLINE unsigned match(SkRecord* record, unsigned i) {
        i = this->matchHead(&fHead, record, i);
        return i == 0 ? 0 : fTail.match(record, i);
    }

    
    
    SK_ALWAYS_INLINE bool search(SkRecord* record, unsigned* begin, unsigned* end) {
        for (*begin = *end; *begin < record->count(); ++(*begin)) {
            *end = this->match(record, *begin);
            if (*end != 0) {
                return true;
            }
        }
        return false;
    }

    
    
    
    template <typename T> T* first()  { return fHead.get(); }
    template <typename T> T* second() { return fTail.fHead.get(); }
    template <typename T> T* third()  { return fTail.fTail.fHead.get(); }

private:
    
    template <typename T>
    unsigned matchHead(T*, SkRecord* record, unsigned i) {
        if (i < record->count()) {
            if (record->mutate<bool>(i, fHead)) {
                return i+1;
            }
        }
        return 0;
    }

    
    template <typename T>
    unsigned matchHead(Star<T>*, SkRecord* record, unsigned i) {
        while (i < record->count()) {
            if (!record->mutate<bool>(i, fHead)) {
                return i;
            }
            i++;
        }
        return 0;
    }

    Matcher fHead;
    Pattern fTail;

    
    template <typename, typename> friend class Cons;
};


struct Nil {
    
    unsigned match(SkRecord*, unsigned i) { return i; }
};




template <typename A>
struct Pattern1 : Cons<A, Nil> {};

template <typename A, typename B>
struct Pattern2 : Cons<A, Pattern1<B> > {};

template <typename A, typename B, typename C>
struct Pattern3 : Cons<A, Pattern2<B, C> > {};

}  

#endif
