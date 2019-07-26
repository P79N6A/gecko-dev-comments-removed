






#ifndef SkMorphology_opts_DEFINED
#define SkMorphology_opts_DEFINED

#include <SkMorphologyImageFilter.h>

enum SkMorphologyProcType {
    kDilateX_SkMorphologyProcType,
    kDilateY_SkMorphologyProcType,
    kErodeX_SkMorphologyProcType,
    kErodeY_SkMorphologyProcType
};

SkMorphologyImageFilter::Proc SkMorphologyGetPlatformProc(SkMorphologyProcType type);

#endif
