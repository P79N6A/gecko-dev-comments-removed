




#include "nscore.h"

#define EXPORT_CDECL(type)   NS_EXPORT type
#define EXPORT_STDCALL(type) NS_EXPORT type NS_STDCALL

NS_EXTERN_C
{
  EXPORT_CDECL(void) set_errno(int status);
  EXPORT_CDECL(int) get_errno();

#if defined(XP_WIN)
  EXPORT_CDECL(void) set_last_error(int status);
  EXPORT_CDECL(int) get_last_error();
#endif 
}
