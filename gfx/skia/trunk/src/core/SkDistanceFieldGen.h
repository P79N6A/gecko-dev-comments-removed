





#ifndef SkDistanceFieldGen_DEFINED
#define SkDistanceFieldGen_DEFINED

#include "SkTypes.h"



#define SK_DistanceFieldMagnitude   4


#define SK_DistanceFieldPad         4

#define SK_DistanceFieldInset       2




#define SK_DistanceFieldMultiplier   "7.96875"
#define SK_DistanceFieldThreshold    "0.50196078431"










bool SkGenerateDistanceFieldFromA8Image(unsigned char* distanceField,
                                        const unsigned char* image,
                                        int w, int h, int rowBytes);










bool SkGenerateDistanceFieldFromBWImage(unsigned char* distanceField,
                                        const unsigned char* image,
                                        int w, int h, int rowBytes);





inline size_t SkComputeDistanceFieldSize(int w, int h) {
    return (w + 2*SK_DistanceFieldPad) * (h + 2*SK_DistanceFieldPad) * sizeof(unsigned char);
}

#endif
