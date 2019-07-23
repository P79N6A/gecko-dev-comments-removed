


#pragma once
#include <windows.h>
#include <tchar.h>



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
} exec_flags_type;

typedef struct {
  exec_flags_type *exec_flags;
  int (__stdcall *ExecuteCodeSegment)(int, HWND);
  void (__stdcall *validate_filename)(TCHAR *);
} extra_parameters;

static int    __stdcall popstring(TCHAR *str); 
static void   __stdcall pushstring(const TCHAR *str);
static char * __stdcall getuservariable(const int varnum);
static void   __stdcall setuservariable(const int varnum, const TCHAR *var);

#ifdef _UNICODE
#define PopStringW(x) popstring(x)
#define PushStringW(x) pushstring(x)
#define SetUserVariableW(x,y) setuservariable(x,y)

static int  __stdcall PopStringA(char* ansiStr);
static void __stdcall PushStringA(const char* ansiStr);
static void __stdcall GetUserVariableW(const int varnum, wchar_t* wideStr);
static void __stdcall GetUserVariableA(const int varnum, char* ansiStr);
static void __stdcall SetUserVariableA(const int varnum, const char* ansiStr);

#else


#define PopStringA(x) popstring(x)
#define PushStringA(x) pushstring(x)
#define SetUserVariableA(x,y) setuservariable(x,y)

static int  __stdcall PopStringW(wchar_t* wideStr);
static void __stdcall PushStringW(wchar_t* wideStr);
static void __stdcall GetUserVariableW(const int varnum, wchar_t* wideStr);
static void __stdcall GetUserVariableA(const int varnum, char* ansiStr);
static void __stdcall SetUserVariableW(const int varnum, const wchar_t* wideStr);

#endif

static BOOL __stdcall IsUnicode(void)
static TCHAR* __stdcall AllocString();

