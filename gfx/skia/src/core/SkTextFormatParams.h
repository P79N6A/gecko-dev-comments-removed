








#ifndef SkTextFormatParams_DEFINES
#define SkTextFormatParams_DEFINES

#include "SkScalar.h"
#include "SkTypes.h"


#define kStdStrikeThru_Offset       (-SK_Scalar1 * 6 / 21)

#define kStdUnderline_Offset        (SK_Scalar1 / 9)

#define kStdUnderline_Thickness     (SK_Scalar1 / 18)





static const SkScalar kStdFakeBoldInterpKeys[] = {
    SkIntToScalar(9),
    SkIntToScalar(36)
};
static const SkScalar kStdFakeBoldInterpValues[] = {
    SK_Scalar1/24,
    SK_Scalar1/32
};
SK_COMPILE_ASSERT(SK_ARRAY_COUNT(kStdFakeBoldInterpKeys) ==
                  SK_ARRAY_COUNT(kStdFakeBoldInterpValues),
                  mismatched_array_size);
static const int kStdFakeBoldInterpLength =
    SK_ARRAY_COUNT(kStdFakeBoldInterpKeys);

#endif  
