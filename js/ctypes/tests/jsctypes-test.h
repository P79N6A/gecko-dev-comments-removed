







































#include "nscore.h"
#include "prtypes.h"
#include "jsapi.h"

#define EXPORT_CDECL(type)   NS_EXPORT type
#define EXPORT_STDCALL(type) NS_EXPORT type NS_STDCALL

NS_EXTERN_C
{
  EXPORT_CDECL(void) test_void_t_cdecl();

  EXPORT_CDECL(void*) get_voidptr_t_cdecl();
  EXPORT_CDECL(void*) set_voidptr_t_cdecl(void*);

#define DEFINE_TYPE(name, type, ffiType)                                       \
  EXPORT_CDECL(type) get_##name##_cdecl();                                     \
  EXPORT_CDECL(type) set_##name##_cdecl(type);                                 \
  EXPORT_CDECL(type) sum_##name##_cdecl(type, type);                           \
  EXPORT_CDECL(type) sum_alignb_##name##_cdecl(char, type, char, type, char);  \
  EXPORT_CDECL(type) sum_alignf_##name##_cdecl(                                \
    float, type, float, type, float);                                          \
  EXPORT_CDECL(type) sum_many_##name##_cdecl(                                  \
    type, type, type, type, type, type, type, type, type,                      \
    type, type, type, type, type, type, type, type, type);                     \
                                                                               \
  EXPORT_CDECL(void) get_##name##_stats(size_t* align, size_t* size,           \
                                        size_t* nalign, size_t* nsize,         \
                                        size_t offsets[]);

#include "../typedefs.h"

#if defined(_WIN32) && !defined(__WIN64)
  EXPORT_STDCALL(void) test_void_t_stdcall();

  EXPORT_STDCALL(void*) get_voidptr_t_stdcall();
  EXPORT_STDCALL(void*) set_voidptr_t_stdcall(void*);

#define DEFINE_TYPE(name, type, ffiType)                                       \
  EXPORT_STDCALL(type) get_##name##_stdcall();                                 \
  EXPORT_STDCALL(type) set_##name##_stdcall(type);                             \
  EXPORT_STDCALL(type) sum_##name##_stdcall(type, type);                       \
  EXPORT_STDCALL(type) sum_alignb_##name##_stdcall(                            \
    char, type, char, type, char);                                             \
  EXPORT_STDCALL(type) sum_alignf_##name##_stdcall(                            \
    float, type, float, type, float);                                          \
  EXPORT_STDCALL(type) sum_many_##name##_stdcall(                              \
    type, type, type, type, type, type, type, type, type,                      \
    type, type, type, type, type, type, type, type, type);

#include "../typedefs.h"

#endif 

  NS_EXPORT PRInt32 test_ansi_len(const char*);
  NS_EXPORT PRInt32 test_wide_len(const PRUnichar*);
  NS_EXPORT const char* test_ansi_ret();
  NS_EXPORT const PRUnichar* test_wide_ret();
  NS_EXPORT char* test_ansi_echo(const char*);

  struct POINT {
    PRInt32 x;
    PRInt32 y;
  };

  struct RECT {
    PRInt32 top;
    PRInt32 left;
    PRInt32 bottom;
    PRInt32 right;
  };

  struct INNER {
    PRUint8 i1;
    PRInt64 i2;
    PRUint8 i3;
  };

  struct NESTED {
    PRInt32 n1;
    PRInt16 n2;
    INNER   inner;
    PRInt64 n3;
    PRInt32 n4;
  };

  NS_EXPORT PRInt32 test_pt_in_rect(RECT, POINT);
  NS_EXPORT void test_init_pt(POINT* pt, PRInt32 x, PRInt32 y);

  NS_EXPORT PRInt32 test_nested_struct(NESTED);
  NS_EXPORT POINT test_struct_return(RECT);
}

