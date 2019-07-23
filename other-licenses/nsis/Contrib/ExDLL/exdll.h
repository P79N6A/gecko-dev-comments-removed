




#ifndef _EXDLL_H_
#define _EXDLL_H_

#include <windows.h>
#include "tchar.h"

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif




#define EXDLL_INIT()           {  \
        g_stringsize=string_size; \
        g_stacktop=stacktop;      \
        g_variables=variables; }


#define WM_NOTIFY_OUTER_NEXT (WM_USER+0x8)
#define WM_NOTIFY_CUSTOM_READY (WM_USER+0xd)




#define NOTIFY_BYE_BYE _T('x')

typedef struct _stack_t {
  struct _stack_t *next;
  TCHAR text[1]; 
} stack_t;


static unsigned int g_stringsize;
static stack_t **g_stacktop;
static TCHAR *g_variables;

static int __stdcall popstring(TCHAR *str) UNUSED; 
static void __stdcall pushstring(const TCHAR *str) UNUSED;
static TCHAR * __stdcall getuservariable(const int varnum) UNUSED;
static void __stdcall setuservariable(const int varnum, const TCHAR *var) UNUSED;

enum
{
INST_0,         
INST_1,         
INST_2,         
INST_3,         
INST_4,         
INST_5,         
INST_6,         
INST_7,         
INST_8,         
INST_9,         
INST_R0,        
INST_R1,        
INST_R2,        
INST_R3,        
INST_R4,        
INST_R5,        
INST_R6,        
INST_R7,        
INST_R8,        
INST_R9,        
INST_CMDLINE,   
INST_INSTDIR,   
INST_OUTDIR,    
INST_EXEDIR,    
INST_LANG,      
__INST_LAST
};

typedef struct {
  int autoclose;
  int all_user_var;
  int exec_error;
  int abort;
  int exec_reboot;
  int reboot_called;
  int XXX_cur_insttype; 
  int XXX_insttype_changed; 
  int silent;
  int instdir_error;
  int rtl;
  int errlvl;
  int alter_reg_view;
  int status_update;
} exec_flags_type;

typedef struct {
  exec_flags_type *exec_flags;
  int (__stdcall *ExecuteCodeSegment)(int, HWND);
  void (__stdcall *validate_filename)(TCHAR *);
} extra_parameters;


static int __stdcall popstring(TCHAR *str)
{
  stack_t *th;
  if (!g_stacktop || !*g_stacktop) return 1;
  th=(*g_stacktop);
  lstrcpy(str,th->text);
  *g_stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}

static void __stdcall pushstring(const TCHAR *str)
{
  stack_t *th;
  if (!g_stacktop) return;
  th=(stack_t*)GlobalAlloc(GPTR,(sizeof(stack_t)+(g_stringsize)*sizeof(TCHAR)));
  lstrcpyn(th->text,str,g_stringsize);
  th->next=*g_stacktop;
  *g_stacktop=th;
}

static TCHAR * __stdcall getuservariable(const int varnum)
{
  if (varnum < 0 || varnum >= __INST_LAST) return NULL;
  return g_variables+varnum*g_stringsize;
}

static void __stdcall setuservariable(const int varnum, const TCHAR *var)
{
	if (var != NULL && varnum >= 0 && varnum < __INST_LAST) 
		lstrcpy(g_variables + varnum*g_stringsize, var);
}

#ifdef _UNICODE
#define PopStringW(x) popstring(x)
#define PushStringW(x) pushstring(x)
#define SetUserVariableW(x,y) setuservariable(x,y)

static int __stdcall PopStringA(char* ansiStr)
{
   wchar_t* wideStr = (wchar_t*) GlobalAlloc(GPTR, g_stringsize*sizeof(wchar_t));
   int rval = popstring(wideStr);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   GlobalFree((HGLOBAL)wideStr);
   return rval;
}

static void __stdcall PushStringA(const char* ansiStr)
{
   wchar_t* wideStr = (wchar_t*) GlobalAlloc(GPTR, g_stringsize*sizeof(wchar_t));
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   pushstring(wideStr);
   GlobalFree((HGLOBAL)wideStr);
   return;
}

static void __stdcall GetUserVariableW(const int varnum, wchar_t* wideStr)
{
   lstrcpyW(wideStr, getuservariable(varnum));
}

static void __stdcall GetUserVariableA(const int varnum, char* ansiStr)
{
   wchar_t* wideStr = getuservariable(varnum);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
}

static void __stdcall SetUserVariableA(const int varnum, const char* ansiStr)
{
   if (ansiStr != NULL && varnum >= 0 && varnum < __INST_LAST)
   {
      wchar_t* wideStr = g_variables + varnum * g_stringsize;
      MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   }
}

#else


#define PopStringA(x) popstring(x)
#define PushStringA(x) pushstring(x)
#define SetUserVariableA(x,y) setuservariable(x,y)

static int __stdcall PopStringW(wchar_t* wideStr)
{
   char* ansiStr = (char*) GlobalAlloc(GPTR, g_stringsize);
   int rval = popstring(ansiStr);
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   GlobalFree((HGLOBAL)ansiStr);
   return rval;
}

static void __stdcall PushStringW(wchar_t* wideStr)
{
   char* ansiStr = (char*) GlobalAlloc(GPTR, g_stringsize);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   pushstring(ansiStr);
   GlobalFree((HGLOBAL)ansiStr);
}

static void __stdcall GetUserVariableW(const int varnum, wchar_t* wideStr)
{
   char* ansiStr = getuservariable(varnum);
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
}

static void __stdcall GetUserVariableA(const int varnum, char* ansiStr)
{
   lstrcpyA(ansiStr, getuservariable(varnum));
}

static void __stdcall SetUserVariableW(const int varnum, const wchar_t* wideStr)
{
   if (wideStr != NULL && varnum >= 0 && varnum < __INST_LAST)
   {
      char* ansiStr = g_variables + varnum * g_stringsize;
      WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   }
}
#endif

static BOOL __stdcall IsUnicode(void)
{
#ifdef _UNICODE
   return TRUE;
#else
   return FALSE;
#endif
}

#endif
