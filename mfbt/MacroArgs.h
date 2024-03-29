









#ifndef mozilla_MacroArgs_h
#define mozilla_MacroArgs_h


























#define MOZ_PASTE_PREFIX_AND_ARG_COUNT(aPrefix, ...) \
  MOZ_MACROARGS_ARG_COUNT_HELPER((__VA_ARGS__, \
    aPrefix##50, aPrefix##49, aPrefix##48, aPrefix##47, aPrefix##46, \
    aPrefix##45, aPrefix##44, aPrefix##43, aPrefix##42, aPrefix##41, \
    aPrefix##40, aPrefix##39, aPrefix##38, aPrefix##37, aPrefix##36, \
    aPrefix##35, aPrefix##34, aPrefix##33, aPrefix##32, aPrefix##31, \
    aPrefix##30, aPrefix##29, aPrefix##28, aPrefix##27, aPrefix##26, \
    aPrefix##25, aPrefix##24, aPrefix##23, aPrefix##22, aPrefix##21, \
    aPrefix##20, aPrefix##19, aPrefix##18, aPrefix##17, aPrefix##16, \
    aPrefix##15, aPrefix##14, aPrefix##13, aPrefix##12, aPrefix##11, \
    aPrefix##10, aPrefix##9,  aPrefix##8,  aPrefix##7,  aPrefix##6,  \
    aPrefix##5,  aPrefix##4,  aPrefix##3,  aPrefix##2,  aPrefix##1, aPrefix##0))

#define MOZ_MACROARGS_ARG_COUNT_HELPER(aArgs) \
  MOZ_MACROARGS_ARG_COUNT_HELPER2 aArgs

#define MOZ_MACROARGS_ARG_COUNT_HELPER2( \
   a1,  a2,  a3,  a4,  a5,  a6,  a7,  a8,  a9, a10, \
  a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
  a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, \
  a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, \
  a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, \
  a51, ...) a51




















#define MOZ_MACROARGS_STRINGIFY_HELPER(x) #x
#define MOZ_STATIC_ASSERT_VALID_ARG_COUNT(...) \
  static_assert( \
    sizeof(MOZ_MACROARGS_STRINGIFY_HELPER((__VA_ARGS__))) != sizeof("()") && \
      (MOZ_PASTE_PREFIX_AND_ARG_COUNT(1, __VA_ARGS__)) > 10 && \
      (int)(MOZ_PASTE_PREFIX_AND_ARG_COUNT(0.0, __VA_ARGS__)) == 0, \
    "MOZ_STATIC_ASSERT_VALID_ARG_COUNT requires 1 to 50 arguments") /* ; */







#define MOZ_ARGS_AFTER_1(a1, ...) __VA_ARGS__
#define MOZ_ARGS_AFTER_2(a1, a2, ...) __VA_ARGS__




#define MOZ_ARG_1(a1, ...) a1
#define MOZ_ARG_2(a1, a2, ...) a2

#endif 
