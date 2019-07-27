#ifndef _EXDLL_H_
#define _EXDLL_H_




#define EXDLL_INIT()           {  \
        g_stringsize=string_size; \
        g_stacktop=stacktop;      \
        g_variables=variables; }


#define WM_NOTIFY_OUTER_NEXT (WM_USER+0x8)
#define WM_NOTIFY_CUSTOM_READY (WM_USER+0xd)
#define NOTIFY_BYE_BYE 'x'

typedef struct _stack_t {
  struct _stack_t *next;
  TCHAR text[1]; 
} stack_t;


static unsigned int g_stringsize;
static stack_t **g_stacktop;
static TCHAR *g_variables;

static int __stdcall popstring(TCHAR *str); 
static void __stdcall pushstring(const TCHAR *str);

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
  th=(stack_t*)GlobalAlloc(GPTR,sizeof(stack_t)+g_stringsize*sizeof(TCHAR));
  lstrcpyn(th->text,str,g_stringsize);
  th->next=*g_stacktop;
  *g_stacktop=th;
}

static TCHAR * __stdcall getuservariable(int varnum)
{
  if (varnum < 0 || varnum >= __INST_LAST) return NULL;
  return g_variables+varnum*g_stringsize;
}

static void __stdcall setuservariable(int varnum, const TCHAR *var)
{
	if (var != NULL && varnum >= 0 && varnum < __INST_LAST) 
		lstrcpy(g_variables + varnum*g_stringsize, var);
}



#endif