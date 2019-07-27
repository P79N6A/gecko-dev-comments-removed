






#ifndef SkDynamicAnnotations_DEFINED
#define SkDynamicAnnotations_DEFINED





#if SK_DYNAMIC_ANNOTATIONS_ENABLED

extern "C" {

void AnnotateIgnoreReadsBegin(const char* file, int line);
void AnnotateIgnoreReadsEnd(const char* file, int line);
void AnnotateIgnoreWritesBegin(const char* file, int line);
void AnnotateIgnoreWritesEnd(const char* file, int line);
void AnnotateBenignRaceSized(const char* file, int line,
                             const volatile void* addr, long size, const char* desc);
}  









template <typename T>
inline T SK_ANNOTATE_UNPROTECTED_READ(const volatile T& x) {
    AnnotateIgnoreReadsBegin(__FILE__, __LINE__);
    T read = x;
    AnnotateIgnoreReadsEnd(__FILE__, __LINE__);
    return read;
}


template <typename T>
inline void SK_ANNOTATE_UNPROTECTED_WRITE(T* ptr, const volatile T& val) {
    AnnotateIgnoreWritesBegin(__FILE__, __LINE__);
    *ptr = val;
    AnnotateIgnoreWritesEnd(__FILE__, __LINE__);
}



template <typename T>
void SK_ANNOTATE_BENIGN_RACE(T* ptr) {
    AnnotateBenignRaceSized(__FILE__, __LINE__, ptr, sizeof(*ptr), "SK_ANNOTATE_BENIGN_RACE");
}

#else  

#define SK_ANNOTATE_UNPROTECTED_READ(x) (x)
#define SK_ANNOTATE_UNPROTECTED_WRITE(ptr, val) *(ptr) = (val)
#define SK_ANNOTATE_BENIGN_RACE(ptr)

#endif




template <typename T>
class SkTRacy {
public:
    operator const T() const {
        return SK_ANNOTATE_UNPROTECTED_READ(fVal);
    }

    SkTRacy& operator=(const T& val) {
        SK_ANNOTATE_UNPROTECTED_WRITE(&fVal, val);
        return *this;
    }

private:
    T fVal;
};





template <typename T>
class SkTRacyReffable {
public:
    SkTRacyReffable() { SK_ANNOTATE_BENIGN_RACE(&fVal); }

    operator const T&() const {
        return fVal;
    }

    SkTRacyReffable& operator=(const T& val) {
        fVal = val;
        return *this;
    }

    const T* get() const { return &fVal; }
          T* get()       { return &fVal; }

    const T* operator->() const { return &fVal; }
          T* operator->()       { return &fVal; }

private:
    T fVal;
};

#endif
