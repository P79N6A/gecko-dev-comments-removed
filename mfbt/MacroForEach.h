











#ifndef mozilla_MacroForEach_h
#define mozilla_MacroForEach_h

#include "mozilla/MacroArgs.h"


























#define MOZ_FOR_EACH_EXPAND_HELPER(...) __VA_ARGS__
#define MOZ_FOR_EACH_GLUE(a, b) a b
#define MOZ_FOR_EACH(aMacro, aFixedArgs, aArgs) \
  MOZ_FOR_EACH_GLUE( \
    MOZ_PASTE_PREFIX_AND_ARG_COUNT(MOZ_FOR_EACH_, \
                                   MOZ_FOR_EACH_EXPAND_HELPER aArgs), \
    (aMacro, aFixedArgs, aArgs))

#define MOZ_FOR_EACH_HELPER_GLUE(a, b) a b
#define MOZ_FOR_EACH_HELPER(aMacro, aFixedArgs, aArgs) \
  MOZ_FOR_EACH_HELPER_GLUE( \
    aMacro, \
    (MOZ_FOR_EACH_EXPAND_HELPER aFixedArgs MOZ_ARG_1 aArgs))

#define MOZ_FOR_EACH_1(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a)
#define MOZ_FOR_EACH_2(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_1(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_3(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_2(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_4(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_3(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_5(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_4(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_6(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_5(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_7(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_6(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_8(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_7(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_9(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_8(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_10(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_9(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_11(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_10(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_12(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_11(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_13(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_12(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_14(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_13(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_15(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_14(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_16(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_15(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_17(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_16(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_18(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_17(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_19(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_18(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_20(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_19(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_21(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_20(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_22(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_21(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_23(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_22(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_24(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_23(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_25(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_24(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_26(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_25(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_27(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_26(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_28(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_27(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_29(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_28(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_30(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_29(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_31(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_30(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_32(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_31(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_33(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_32(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_34(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_33(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_35(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_34(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_36(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_35(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_37(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_36(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_38(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_37(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_39(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_38(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_40(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_39(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_41(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_40(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_42(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_41(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_43(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_42(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_44(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_43(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_45(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_44(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_46(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_45(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_47(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_46(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_48(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_47(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_49(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_48(m, fa, (MOZ_ARGS_AFTER_1 a))
#define MOZ_FOR_EACH_50(m, fa, a) \
  MOZ_FOR_EACH_HELPER(m, fa, a) MOZ_FOR_EACH_49(m, fa, (MOZ_ARGS_AFTER_1 a))

#endif 
