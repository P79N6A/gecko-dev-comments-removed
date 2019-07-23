





#pragma once
#include <TChar.h>


typedef TCHAR NSISCH;
#define NSISCALL      __stdcall

namespace NSIS {

__forceinline void* NSISCALL MemAlloc(SIZE_T cb) {return GlobalAlloc(LPTR,cb);}
__forceinline void NSISCALL MemFree(void* p) {GlobalFree(p);}

enum {
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
__INST_LAST,

VIDX_TEMP=(INST_LANG+1), 
VIDX_PLUGINSDIR,
VIDX_EXEPATH,
VIDX_EXEFILENAME,
VIDX_STATECLICKNEXT,
__VIDX_UNDOCLAST
};



typedef struct _stack_t {
  struct _stack_t *next;
  NSISCH text[ANYSIZE_ARRAY];
} stack_t;

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
  int (NSISCALL *ExecuteCodeSegment)(int, HWND);
  void (NSISCALL *validate_filename)(char *);
} extra_parameters;

extern UINT StrSize;
extern stack_t **StackTop;
extern NSISCH*Vars;

inline bool NSISCALL SetErrLvl(extra_parameters*pExtraParams,int ErrLevel) {return pExtraParams? ((pExtraParams->exec_flags->errlvl=ErrLevel)||true):false;}
inline bool NSISCALL SetErrorFlag(extra_parameters*pExtraParams) {return pExtraParams? ((pExtraParams->exec_flags->exec_error=1)||true):false;}
inline bool NSISCALL ClearErrorFlag(extra_parameters*pExtraParams) {return pExtraParams?((pExtraParams->exec_flags->exec_error=0)||true):false;}

__forceinline int NSISCALL ExecuteCodeSegment(extra_parameters*pExtraParams,int pos,HWND hwndProgress=NULL) {
	return pExtraParams?pExtraParams->ExecuteCodeSegment(pos,hwndProgress):(0x7FFFFFFF);
}

static NSISCH* __fastcall GetVar(const int varnum) 
{
	
	if (varnum < 0 || varnum >= __VIDX_UNDOCLAST) return NULL;
	return NSIS::Vars+(varnum*NSIS::StrSize);
}

inline void NSISCALL SetVarUINT(const int varnum,UINT Value) {
	wsprintf(GetVar(varnum),_T("%u"),Value);
}

static stack_t* NSISCALL StackPop() {
	if (NSIS::StackTop && *NSIS::StackTop) {
		stack_t*s=(*NSIS::StackTop);
		*NSIS::StackTop=(*NSIS::StackTop)->next;
		return s;
	}
	return 0;
}
__forceinline void NSISCALL StackFreeItem(stack_t*pStackItem) {NSIS::MemFree(pStackItem);}

static DWORD NSISCALL StackPush(NSISCH*InStr,UINT StackStrSize=NSIS::StrSize) {
	if (!NSIS::StackTop)return ERROR_INVALID_PARAMETER;
	stack_t*sNew=(stack_t*)NSIS::MemAlloc(sizeof(stack_t)+(StackStrSize*sizeof(NSISCH)));
	if (!sNew)return ERROR_OUTOFMEMORY;
	lstrcpyn(sNew->text,InStr,StackStrSize);
	sNew->next=*NSIS::StackTop;
	*NSIS::StackTop=sNew;
	return NO_ERROR;
}

}; 

#define NSISUTIL_INIT() namespace NSIS {static UINT StrSize;static stack_t **StackTop;static NSISCH*Vars;}//Call in only ONE source file
#define NSISUTIL_INITEXPORT(_v,_strsize,_stackt) NSIS::Vars=_v;NSIS::StrSize=_strsize;NSIS::StackTop=_stackt







#ifdef _MSC_VER
#	define EXPORTNSISFUNC extern "C" void __declspec(dllexport) __cdecl
#	else
#	error EXPORTNSISFUNC needs compiler goo, you are on your own!
#	endif
#define NSISFUNCSTART4(_h,_strsize,_v,_stackt) {NSISUTIL_INITEXPORT(_v,_strsize,_stackt);
#define NSISFUNCSTART5(_h,_strsize,_v,_stackt,_eparams) NSISFUNCSTART4(_h,_strsize,_v,_stackt)
#define NSISFUNCSTART NSISFUNCSTART5
#define NSISFUNCEND() }

