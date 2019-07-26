





#ifndef SkDistanceFieldGen_DEFINED
#define SkDistanceFieldGen_DEFINED











bool SkGenerateDistanceFieldFromImage(unsigned char* distanceField,
                                      const unsigned char* image,
                                      int w, int h,
                                      int distanceMagnitude);

#endif
