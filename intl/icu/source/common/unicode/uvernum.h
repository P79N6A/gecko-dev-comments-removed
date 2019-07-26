























 




















#ifndef UVERNUM_H
#define UVERNUM_H





#define U_COPYRIGHT_STRING \
  " Copyright (C) 2012, International Business Machines Corporation and others. All Rights Reserved. "





#define U_ICU_VERSION_MAJOR_NUM 50





#define U_ICU_VERSION_MINOR_NUM 1





#define U_ICU_VERSION_PATCHLEVEL_NUM 2





#ifndef U_ICU_VERSION_BUILDLEVEL_NUM
#define U_ICU_VERSION_BUILDLEVEL_NUM 0
#endif





#define U_ICU_VERSION_SUFFIX _50

















#ifndef U_ICU_ENTRY_POINT_RENAME
#ifdef U_HAVE_LIB_SUFFIX
#define U_DEF_ICU_ENTRY_POINT_RENAME(x,y,z) x ## y ##  z
#define U_DEF2_ICU_ENTRY_POINT_RENAME(x,y,z) U_DEF_ICU_ENTRY_POINT_RENAME(x,y,z)
#define U_ICU_ENTRY_POINT_RENAME(x)    U_DEF2_ICU_ENTRY_POINT_RENAME(x,U_ICU_VERSION_SUFFIX,U_LIB_SUFFIX_C_NAME)
#else
#define U_DEF_ICU_ENTRY_POINT_RENAME(x,y) x ## y
#define U_DEF2_ICU_ENTRY_POINT_RENAME(x,y) U_DEF_ICU_ENTRY_POINT_RENAME(x,y)
#define U_ICU_ENTRY_POINT_RENAME(x)    U_DEF2_ICU_ENTRY_POINT_RENAME(x,U_ICU_VERSION_SUFFIX)
#endif
#endif






#define U_ICU_VERSION "50.1.2"





#define U_ICU_VERSION_SHORT "50"

#ifndef U_HIDE_INTERNAL_API



#define U_ICU_DATA_VERSION "50.1"
#endif  














#define UCOL_RUNTIME_VERSION 7








#define UCOL_BUILDER_VERSION 8






#define UCOL_TAILORINGS_VERSION 1

#endif
