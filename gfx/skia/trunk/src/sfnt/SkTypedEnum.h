






#ifndef SkTypedEnum_DEFINED
#define SkTypedEnum_DEFINED

#include "SkPreprocessorSeq.h"


#ifndef __has_feature
    #define __has_feature(x) 0
#endif
#ifndef __has_extension
    #define __has_extension __has_feature
#endif


#if defined(_MSC_VER) && _MSC_VER >= 1400
    #define SK_TYPED_ENUMS

#elif defined(__clang__) && __has_extension(cxx_strong_enums)
    #define SK_TYPED_ENUMS





#elif defined(__GNUC__) && (((__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__ >= 40501) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || __cplusplus >= 201103L)
    #define SK_TYPED_ENUMS
#endif


#ifdef SK_TYPED_ENUMS

    #define SK_TYPED_ENUM_VALUES(data, elem) \
        SK_PAIR_FIRST(elem) = SK_PAIR_SECOND(elem),

    #define SK_TYPED_ENUM_IDS(data, elem) \
        elem,

    #define SK_TYPED_ENUM_IDS_L(data, elem) \
        elem

    #define SK_TYPED_ENUM(enumName, enumType, enumSeq, idSeq) \
        enum enumName : enumType { \
            SK_SEQ_FOREACH(SK_TYPED_ENUM_VALUES, _, enumSeq) \
        } SK_SEQ_FOREACH_L(SK_TYPED_ENUM_IDS, SK_TYPED_ENUM_IDS_L, _, idSeq);

#else

    #define SK_TYPED_ENUM_VALUES(enumType, elem) \
        static const enumType SK_PAIR_FIRST(elem) = SK_PAIR_SECOND(elem);

    #define SK_TYPED_ENUM_IDS(enumType, elem) \
        enumType elem;

    #define SK_TYPED_ENUM(enumName, enumType, enumSeq, idSeq) \
        typedef enumType enumName; \
        SK_SEQ_FOREACH(SK_TYPED_ENUM_VALUES, enumType, enumSeq) \
        SK_SEQ_FOREACH(SK_TYPED_ENUM_IDS, enumType, idSeq)

#endif

#endif
