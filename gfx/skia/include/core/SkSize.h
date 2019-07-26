






#ifndef SkSize_DEFINED
#define SkSize_DEFINED

#include "SkScalar.h"

template <typename T> struct SkTSize {
    T fWidth;
    T fHeight;

    static SkTSize Make(T w, T h) {
        SkTSize s;
        s.fWidth = w;
        s.fHeight = h;
        return s;
    }

    void set(T w, T h) {
        fWidth = w;
        fHeight = h;
    }

    

    bool isZero() const {
        return 0 == fWidth && 0 == fHeight;
    }

    
    bool isEmpty() const {
        return fWidth <= 0 || fHeight <= 0;
    }

    
    void setEmpty() {
        fWidth = fHeight = 0;
    }

    T width() const { return fWidth; }
    T height() const { return fHeight; }

    
    void clampNegToZero() {
        if (fWidth < 0) {
            fWidth = 0;
        }
        if (fHeight < 0) {
            fHeight = 0;
        }
    }

    bool equals(T w, T h) const {
        return fWidth == w && fHeight == h;
    }
};

template <typename T>
static inline bool operator==(const SkTSize<T>& a, const SkTSize<T>& b) {
    return a.fWidth == b.fWidth && a.fHeight == b.fHeight;
}

template <typename T>
static inline bool operator!=(const SkTSize<T>& a, const SkTSize<T>& b) {
    return !(a == b);
}



typedef SkTSize<int32_t> SkISize;

struct SkSize : public SkTSize<SkScalar> {
    static SkSize Make(SkScalar w, SkScalar h) {
        SkSize s;
        s.fWidth = w;
        s.fHeight = h;
        return s;
    }


    SkSize& operator=(const SkISize& src) {
        this->set(SkIntToScalar(src.fWidth), SkIntToScalar(src.fHeight));
        return *this;
    }

    SkISize toRound() const {
        SkISize s;
        s.set(SkScalarRound(fWidth), SkScalarRound(fHeight));
        return s;
    }

    SkISize toCeil() const {
        SkISize s;
        s.set(SkScalarCeil(fWidth), SkScalarCeil(fHeight));
        return s;
    }

    SkISize toFloor() const {
        SkISize s;
        s.set(SkScalarFloor(fWidth), SkScalarFloor(fHeight));
        return s;
    }
};

#endif
