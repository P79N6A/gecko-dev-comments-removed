















#ifndef __USHAPE_H__
#define __USHAPE_H__

#include "unicode/utypes.h"














































































U_STABLE int32_t U_EXPORT2
u_shapeArabic(const UChar *source, int32_t sourceLength,
              UChar *dest, int32_t destSize,
              uint32_t options,
              UErrorCode *pErrorCode);






#define U_SHAPE_LENGTH_GROW_SHRINK              0







#define U_SHAPE_LAMALEF_RESIZE                  0 






#define U_SHAPE_LENGTH_FIXED_SPACES_NEAR        1








#define U_SHAPE_LAMALEF_NEAR                    1 






#define U_SHAPE_LENGTH_FIXED_SPACES_AT_END      2








#define U_SHAPE_LAMALEF_END                     2 






#define U_SHAPE_LENGTH_FIXED_SPACES_AT_BEGINNING 3








#define U_SHAPE_LAMALEF_BEGIN                    3 















#define U_SHAPE_LAMALEF_AUTO                     0x10000 


#define U_SHAPE_LENGTH_MASK                      0x10003 /* Changed old value 3 */






#define U_SHAPE_LAMALEF_MASK                     0x10003 /* updated */


#define U_SHAPE_TEXT_DIRECTION_LOGICAL          0








#define U_SHAPE_TEXT_DIRECTION_VISUAL_RTL       0







#define U_SHAPE_TEXT_DIRECTION_VISUAL_LTR       4


#define U_SHAPE_TEXT_DIRECTION_MASK             4



#define U_SHAPE_LETTERS_NOOP                    0


#define U_SHAPE_LETTERS_SHAPE                   8


#define U_SHAPE_LETTERS_UNSHAPE                 0x10








#define U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED 0x18



#define U_SHAPE_LETTERS_MASK                        0x18



#define U_SHAPE_DIGITS_NOOP                     0






#define U_SHAPE_DIGITS_EN2AN                    0x20






#define U_SHAPE_DIGITS_AN2EN                    0x40












#define U_SHAPE_DIGITS_ALEN2AN_INIT_LR          0x60











#define U_SHAPE_DIGITS_ALEN2AN_INIT_AL          0x80


#define U_SHAPE_DIGITS_RESERVED                 0xa0


#define U_SHAPE_DIGITS_MASK                     0xe0



#define U_SHAPE_DIGIT_TYPE_AN                   0


#define U_SHAPE_DIGIT_TYPE_AN_EXTENDED          0x100


#define U_SHAPE_DIGIT_TYPE_RESERVED             0x200


#define U_SHAPE_DIGIT_TYPE_MASK                 0x300 /* I need to change this from 0x3f00 to 0x300 */








#define U_SHAPE_AGGREGATE_TASHKEEL              0x4000

#define U_SHAPE_AGGREGATE_TASHKEEL_NOOP         0

#define U_SHAPE_AGGREGATE_TASHKEEL_MASK         0x4000







#define U_SHAPE_PRESERVE_PRESENTATION           0x8000





#define U_SHAPE_PRESERVE_PRESENTATION_NOOP      0

#define U_SHAPE_PRESERVE_PRESENTATION_MASK      0x8000

 












#define U_SHAPE_SEEN_TWOCELL_NEAR     0x200000





#define U_SHAPE_SEEN_MASK             0x700000

 












#define U_SHAPE_YEHHAMZA_TWOCELL_NEAR      0x1000000






#define U_SHAPE_YEHHAMZA_MASK              0x3800000

 









#define U_SHAPE_TASHKEEL_BEGIN                      0x40000










#define U_SHAPE_TASHKEEL_END                        0x60000









#define U_SHAPE_TASHKEEL_RESIZE                     0x80000










#define U_SHAPE_TASHKEEL_REPLACE_BY_TATWEEL         0xC0000





#define U_SHAPE_TASHKEEL_MASK                       0xE0000


 




















#define U_SHAPE_SPACES_RELATIVE_TO_TEXT_BEGIN_END 0x4000000





#define U_SHAPE_SPACES_RELATIVE_TO_TEXT_MASK      0x4000000













#define U_SHAPE_TAIL_NEW_UNICODE        0x8000000





#define U_SHAPE_TAIL_TYPE_MASK          0x8000000

#endif
