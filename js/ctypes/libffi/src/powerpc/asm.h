
























#define ASM_GLOBAL_DIRECTIVE .globl


#define C_SYMBOL_NAME(name) name

#ifdef	__STDC__
#define C_LABEL(name)		name##:
#else
#define C_LABEL(name)		name/**/:
#endif


#define ALIGNARG(log2) log2

#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name


#ifdef	PROF



#ifdef PIC
#define CALL_MCOUNT							      \
  .pushsection;								      \
  .section ".data";							      \
  .align ALIGNARG(2);							      \
0:.long 0;								      \
  .previous;								      \
  mflr  %r0;								      \
  stw   %r0,4(%r1);							      \
  bl    _GLOBAL_OFFSET_TABLE_@local-4;					      \
  mflr  %r11;								      \
  lwz   %r0,0b@got(%r11);						      \
  bl    JUMPTARGET(_mcount);
#else  
#define CALL_MCOUNT							      \
  .section ".data";							      \
  .align ALIGNARG(2);							      \
0:.long 0;								      \
  .previous;								      \
  mflr  %r0;								      \
  lis   %r11,0b@ha;							      \
  stw   %r0,4(%r1);							      \
  addi  %r0,%r11,0b@l;							      \
  bl    JUMPTARGET(_mcount);
#endif 
#else  
#define CALL_MCOUNT
#endif 

#define	ENTRY(name)							      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  CALL_MCOUNT

#define EALIGN_W_0
#define EALIGN_W_1  nop
#define EALIGN_W_2  nop;nop
#define EALIGN_W_3  nop;nop;nop
#define EALIGN_W_4  EALIGN_W_3;nop
#define EALIGN_W_5  EALIGN_W_4;nop
#define EALIGN_W_6  EALIGN_W_5;nop
#define EALIGN_W_7  EALIGN_W_6;nop



#ifdef PROF
#define EALIGN(name, alignt, words)					      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  CALL_MCOUNT								      \
  b 0f;									      \
  .align ALIGNARG(alignt);						      \
  EALIGN_W_##words;							      \
  0:
#else 
#define EALIGN(name, alignt, words)					      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(alignt);						      \
  EALIGN_W_##words;							      \
  C_LABEL(name)
#endif

#define END(name)							      \
  ASM_SIZE_DIRECTIVE(name)

#ifdef PIC
#define JUMPTARGET(name) name##@plt
#else
#define JUMPTARGET(name) name
#endif


#define L(x) .L##x
