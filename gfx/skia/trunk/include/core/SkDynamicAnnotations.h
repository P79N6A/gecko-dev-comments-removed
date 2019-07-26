






#ifndef SkDynamicAnnotations_DEFINED
#define SkDynamicAnnotations_DEFINED






#if DYNAMIC_ANNOTATIONS_ENABLED

extern "C" {

void AnnotateIgnoreReadsBegin(const char* file, int line);
void AnnotateIgnoreReadsEnd(const char* file, int line);
}  









template <typename T>
inline T SK_ANNOTATE_UNPROTECTED_READ(const volatile T& x) {
    AnnotateIgnoreReadsBegin(__FILE__, __LINE__);
    T read = x;
    AnnotateIgnoreReadsEnd(__FILE__, __LINE__);
    return read;
}

#else  

#define SK_ANNOTATE_UNPROTECTED_READ(x) (x)

#endif

#endif
