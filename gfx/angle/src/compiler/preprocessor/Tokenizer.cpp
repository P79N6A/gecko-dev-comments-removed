#line 16 "./Tokenizer.l"










#line 13 "./Tokenizer.cpp"

#define  YY_INT_ALIGNED short int



#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 5
#define YY_FLEX_SUBMINOR_VERSION 35
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif




#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>





#ifndef FLEXINT_H
#define FLEXINT_H



#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L




#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
typedef uint64_t flex_uint64_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;
#endif 


#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#endif 

#ifdef __cplusplus


#define YY_USE_CONST

#else	


#if defined (__STDC__)

#define YY_USE_CONST

#endif	
#endif	

#ifdef YY_USE_CONST
#define yyconst const
#else
#define yyconst
#endif


#define YY_NULL 0






#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)


#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif



#define yyin yyg->yyin_r
#define yyout yyg->yyout_r
#define yyextra yyg->yyextra_r
#define yyleng yyg->yyleng_r
#define yytext yyg->yytext_r
#define yylineno (YY_CURRENT_BUFFER_LVALUE->yy_bs_lineno)
#define yycolumn (YY_CURRENT_BUFFER_LVALUE->yy_bs_column)
#define yy_flex_debug yyg->yy_flex_debug_r





#define BEGIN yyg->yy_start = 1 + 2 *





#define YY_START ((yyg->yy_start - 1) / 2)
#define YYSTATE YY_START


#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)


#define YY_NEW_FILE pprestart(yyin ,yyscanner )

#define YY_END_OF_BUFFER_CHAR 0


#ifndef YY_BUF_SIZE
#define YY_BUF_SIZE 16384
#endif



#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    

#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = yyg->yy_hold_char; \
		YY_RESTORE_YY_MORE_OFFSET \
		yyg->yy_c_buf_p = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up yytext again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, yyg->yytext_ptr , yyscanner )

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		
	char *yy_buf_pos;		

	


	yy_size_t yy_buf_size;

	


	yy_size_t yy_n_chars;

	



	int yy_is_our_buffer;

	




	int yy_is_interactive;

	



	int yy_at_bol;

    int yy_bs_lineno; 
    int yy_bs_column; 
    
	


	int yy_fill_buffer;

	int yy_buffer_status;

#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	









#define YY_BUFFER_EOF_PENDING 2

	};
#endif 







#define YY_CURRENT_BUFFER ( yyg->yy_buffer_stack \
                          ? yyg->yy_buffer_stack[yyg->yy_buffer_stack_top] \
                          : NULL)




#define YY_CURRENT_BUFFER_LVALUE yyg->yy_buffer_stack[yyg->yy_buffer_stack_top]

void pprestart (FILE *input_file ,yyscan_t yyscanner );
void pp_switch_to_buffer (YY_BUFFER_STATE new_buffer ,yyscan_t yyscanner );
YY_BUFFER_STATE pp_create_buffer (FILE *file,int size ,yyscan_t yyscanner );
void pp_delete_buffer (YY_BUFFER_STATE b ,yyscan_t yyscanner );
void pp_flush_buffer (YY_BUFFER_STATE b ,yyscan_t yyscanner );
void pppush_buffer_state (YY_BUFFER_STATE new_buffer ,yyscan_t yyscanner );
void pppop_buffer_state (yyscan_t yyscanner );

static void ppensure_buffer_stack (yyscan_t yyscanner );
static void pp_load_buffer_state (yyscan_t yyscanner );
static void pp_init_buffer (YY_BUFFER_STATE b,FILE *file ,yyscan_t yyscanner );

#define YY_FLUSH_BUFFER pp_flush_buffer(YY_CURRENT_BUFFER ,yyscanner)

YY_BUFFER_STATE pp_scan_buffer (char *base,yy_size_t size ,yyscan_t yyscanner );
YY_BUFFER_STATE pp_scan_string (yyconst char *yy_str ,yyscan_t yyscanner );
YY_BUFFER_STATE pp_scan_bytes (yyconst char *bytes,yy_size_t len ,yyscan_t yyscanner );

void *ppalloc (yy_size_t ,yyscan_t yyscanner );
void *pprealloc (void *,yy_size_t ,yyscan_t yyscanner );
void ppfree (void * ,yyscan_t yyscanner );

#define yy_new_buffer pp_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        ppensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            pp_create_buffer(yyin,YY_BUF_SIZE ,yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        ppensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            pp_create_buffer(yyin,YY_BUF_SIZE ,yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)



#define ppwrap(n) 1
#define YY_SKIP_YYWRAP

typedef unsigned char YY_CHAR;

typedef int yy_state_type;

#define yytext_ptr yytext_r

static yy_state_type yy_get_previous_state (yyscan_t yyscanner );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  ,yyscan_t yyscanner);
static int yy_get_next_buffer (yyscan_t yyscanner );
static void yy_fatal_error (yyconst char msg[] ,yyscan_t yyscanner );




#define YY_DO_BEFORE_ACTION \
	yyg->yytext_ptr = yy_bp; \
	yyleng = (yy_size_t) (yy_cp - yy_bp); \
	yyg->yy_hold_char = *yy_cp; \
	*yy_cp = '\0'; \
	yyg->yy_c_buf_p = yy_cp;

#define YY_NUM_RULES 38
#define YY_END_OF_BUFFER 39


struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[87] =
    {   0,
        0,    0,    0,    0,   39,   37,   34,   35,   35,   33,
        7,   33,   33,   33,   33,   33,   33,   33,   33,    9,
        9,   33,   33,   33,    8,   37,   33,   33,    3,    5,
        5,    4,   34,   35,   19,   27,   20,   30,   25,   12,
       23,   13,   24,   10,    2,    1,   26,   10,    9,   11,
       11,   11,   11,    9,   14,   16,   18,   17,   15,    8,
       36,   36,   31,   21,   32,   22,    3,    5,    6,   11,
       10,   11,    1,   10,   11,    0,   10,    9,   28,   29,
        0,   10,   10,   10,   10,    0
    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        2,    2,    4,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    5,    1,    6,    1,    7,    8,    1,    9,
        9,   10,   11,    9,   12,   13,   14,   15,   16,   16,
       16,   16,   16,   16,   16,   17,   17,    9,    9,   18,
       19,   20,    9,    1,   21,   21,   21,   21,   22,   21,
       23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
       23,   23,   23,   23,   23,   23,   23,   24,   23,   23,
        9,   25,    9,   26,   23,    1,   21,   21,   21,   21,

       22,   21,   23,   23,   23,   23,   23,   23,   23,   23,
       23,   23,   23,   23,   23,   23,   23,   23,   23,   24,
       23,   23,    9,   27,    9,    9,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,

        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1
    } ;

static yyconst flex_int32_t yy_meta[28] =
    {   0,
        1,    1,    2,    2,    1,    1,    1,    1,    1,    3,
        1,    1,    4,    1,    5,    5,    5,    1,    1,    1,
        5,    5,    5,    5,    1,    1,    1
    } ;

static yyconst flex_int16_t yy_base[92] =
    {   0,
        0,    0,   25,   27,  162,  163,  159,  163,  152,  132,
      163,  131,   24,  163,  116,   22,   26,   31,   30,   37,
       40,   44,  115,   46,    0,   64,   50,   15,    0,  163,
      124,   91,   88,  163,  163,  163,  163,  163,  163,  163,
      163,  163,  163,   64,  163,    0,  163,   76,   54,   58,
       79,   91,   91,    0,   56,  163,  163,  163,   32,    0,
      163,   36,  163,  163,  163,  163,    0,  163,  163,   94,
        0,  106,    0,    0,  113,   55,   72,  113,  163,  163,
      116,  101,  108,  123,  126,  163,  143,   31,  148,  153,
      155

    } ;

static yyconst flex_int16_t yy_def[92] =
    {   0,
       86,    1,   87,   87,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
       20,   86,   86,   86,   88,   86,   86,   86,   89,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   90,   86,   86,   20,   20,
       48,   51,   91,   21,   86,   86,   86,   86,   86,   88,
       86,   86,   86,   86,   86,   86,   89,   86,   86,   44,
       44,   70,   90,   48,   51,   86,   52,   91,   86,   86,
       86,   72,   75,   86,   86,    0,   86,   86,   86,   86,
       86

    } ;

static yyconst flex_int16_t yy_nxt[191] =
    {   0,
        6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
       16,   17,   18,   19,   20,   21,   21,   22,   23,   24,
       25,   25,   25,   25,   26,   27,   28,   30,   31,   30,
       31,   37,   40,   65,   32,   60,   32,   42,   61,   45,
       41,   66,   38,   46,   43,   44,   44,   44,   47,   48,
       80,   49,   49,   50,   54,   54,   54,   51,   52,   51,
       53,   55,   56,   51,   58,   59,   61,   62,   63,   84,
       84,   84,   50,   50,   79,   64,   70,   51,   71,   71,
       71,   51,   86,   86,   70,   72,   70,   70,   51,   33,
       74,   74,   74,   51,   51,   51,   51,   75,   51,   51,

       51,   76,   76,   51,   69,   77,   77,   77,   70,   70,
       70,   86,   86,   51,   51,   70,   81,   81,   86,   86,
       82,   82,   82,   81,   81,   51,   68,   83,   83,   83,
       85,   85,   85,   57,   39,   51,   51,   84,   84,   84,
       85,   85,   85,   29,   29,   29,   29,   29,   67,   36,
       35,   67,   67,   73,   34,   73,   73,   73,   78,   78,
       33,   86,    5,   86,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86
    } ;

static yyconst flex_int16_t yy_chk[191] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    3,    3,    4,
        4,   13,   16,   28,    3,   88,    4,   17,   62,   19,
       16,   28,   13,   19,   17,   18,   18,   18,   19,   20,
       59,   20,   20,   20,   21,   21,   21,   20,   20,   20,
       20,   22,   22,   21,   24,   24,   26,   26,   27,   76,
       76,   76,   50,   50,   55,   27,   44,   49,   44,   44,
       44,   50,   77,   77,   44,   44,   44,   44,   48,   33,
       48,   48,   48,   51,   51,   51,   48,   48,   48,   48,

       51,   52,   52,   53,   32,   52,   52,   52,   70,   70,
       70,   82,   82,   53,   53,   70,   72,   72,   83,   83,
       72,   72,   72,   75,   75,   78,   31,   75,   75,   75,
       81,   81,   81,   23,   15,   78,   78,   84,   84,   84,
       85,   85,   85,   87,   87,   87,   87,   87,   89,   12,
       10,   89,   89,   90,    9,   90,   90,   90,   91,   91,
        7,    5,   86,   86,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86,
       86,   86,   86,   86,   86,   86,   86,   86,   86,   86
    } ;




#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET














#include "Tokenizer.h"

#include "DiagnosticsBase.h"
#include "Token.h"

#if defined(__GNUC__)

#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif

typedef std::string YYSTYPE;
typedef pp::SourceLocation YYLTYPE;


#define yyfileno yycolumn

#define YY_USER_INIT                   \
    do {                               \
        yyfileno = 0;                  \
        yylineno = 1;                  \
        yyextra->leadingSpace = false; \
        yyextra->lineStart = true;     \
    } while(0);

#define YY_USER_ACTION                                              \
    do                                                              \
    {                                                               \
        pp::Input* input = &yyextra->input;                         \
        pp::Input::Location* scanLoc = &yyextra->scanLoc;           \
        while ((scanLoc->sIndex < input->count()) &&                \
               (scanLoc->cIndex >= input->length(scanLoc->sIndex))) \
        {                                                           \
            scanLoc->cIndex -= input->length(scanLoc->sIndex++);    \
            ++yyfileno; yylineno = 1;                               \
        }                                                           \
        yylloc->file = yyfileno;                                    \
        yylloc->line = yylineno;                                    \
        scanLoc->cIndex += yyleng;                                  \
    } while(0);

#define YY_INPUT(buf, result, maxSize) \
    result = yyextra->input.read(buf, maxSize);

#define INITIAL 0
#define COMMENT 1

#define YY_EXTRA_TYPE pp::Tokenizer::Context*


struct yyguts_t
    {

    
    YY_EXTRA_TYPE yyextra_r;

    
    FILE *yyin_r, *yyout_r;
    size_t yy_buffer_stack_top; 
    size_t yy_buffer_stack_max; 
    YY_BUFFER_STATE * yy_buffer_stack; 
    char yy_hold_char;
    yy_size_t yy_n_chars;
    yy_size_t yyleng_r;
    char *yy_c_buf_p;
    int yy_init;
    int yy_start;
    int yy_did_buffer_switch_on_eof;
    int yy_start_stack_ptr;
    int yy_start_stack_depth;
    int *yy_start_stack;
    yy_state_type yy_last_accepting_state;
    char* yy_last_accepting_cpos;

    int yylineno_r;
    int yy_flex_debug_r;

    char *yytext_r;
    int yy_more_flag;
    int yy_more_len;

    YYSTYPE * yylval_r;

    YYLTYPE * yylloc_r;

    }; 

static int yy_init_globals (yyscan_t yyscanner );

    

    #    define yylval yyg->yylval_r
    
    #    define yylloc yyg->yylloc_r
    
int pplex_init (yyscan_t* scanner);

int pplex_init_extra (YY_EXTRA_TYPE user_defined,yyscan_t* scanner);




int pplex_destroy (yyscan_t yyscanner );

int ppget_debug (yyscan_t yyscanner );

void ppset_debug (int debug_flag ,yyscan_t yyscanner );

YY_EXTRA_TYPE ppget_extra (yyscan_t yyscanner );

void ppset_extra (YY_EXTRA_TYPE user_defined ,yyscan_t yyscanner );

FILE *ppget_in (yyscan_t yyscanner );

void ppset_in  (FILE * in_str ,yyscan_t yyscanner );

FILE *ppget_out (yyscan_t yyscanner );

void ppset_out  (FILE * out_str ,yyscan_t yyscanner );

yy_size_t ppget_leng (yyscan_t yyscanner );

char *ppget_text (yyscan_t yyscanner );

int ppget_lineno (yyscan_t yyscanner );

void ppset_lineno (int line_number ,yyscan_t yyscanner );

YYSTYPE * ppget_lval (yyscan_t yyscanner );

void ppset_lval (YYSTYPE * yylval_param ,yyscan_t yyscanner );

       YYLTYPE *ppget_lloc (yyscan_t yyscanner );
    
        void ppset_lloc (YYLTYPE * yylloc_param ,yyscan_t yyscanner );
    




#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int ppwrap (yyscan_t yyscanner );
#else
extern int ppwrap (yyscan_t yyscanner );
#endif
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy (char *,yyconst char *,int ,yyscan_t yyscanner);
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * ,yyscan_t yyscanner);
#endif

#ifndef YY_NO_INPUT

#ifdef __cplusplus
static int yyinput (yyscan_t yyscanner );
#else
static int input (yyscan_t yyscanner );
#endif

#endif


#ifndef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE 8192
#endif


#ifndef ECHO



#define ECHO fwrite( yytext, yyleng, 1, yyout )
#endif




#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
		{ \
		int c = '*'; \
		yy_size_t n; \
		for ( n = 0; n < max_size && \
			     (c = getc( yyin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( yyin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, yyin))==0 && ferror(yyin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(yyin); \
			} \
		}\
\

#endif





#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif


#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif


#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg , yyscanner)
#endif






#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int pplex \
               (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner);

#define YY_DECL int pplex \
               (YYSTYPE * yylval_param, YYLTYPE * yylloc_param , yyscan_t yyscanner)
#endif 




#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif


#ifndef YY_BREAK
#define YY_BREAK break;
#endif

#define YY_RULE_SETUP \
	YY_USER_ACTION



YY_DECL
{
	register yy_state_type yy_current_state;
	register char *yy_cp, *yy_bp;
	register int yy_act;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

    

    yylval = yylval_param;

    yylloc = yylloc_param;

	if ( !yyg->yy_init )
		{
		yyg->yy_init = 1;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! yyg->yy_start )
			yyg->yy_start = 1;	

		if ( ! yyin )
			yyin = stdin;

		if ( ! yyout )
			yyout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			ppensure_buffer_stack (yyscanner);
			YY_CURRENT_BUFFER_LVALUE =
				pp_create_buffer(yyin,YY_BUF_SIZE ,yyscanner);
		}

		pp_load_buffer_state(yyscanner );
		}

	while ( 1 )		
		{
		yy_cp = yyg->yy_c_buf_p;

		
		*yy_cp = yyg->yy_hold_char;

		


		yy_bp = yy_cp;

		yy_current_state = yyg->yy_start;
yy_match:
		do
			{
			register YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
			if ( yy_accept[yy_current_state] )
				{
				yyg->yy_last_accepting_state = yy_current_state;
				yyg->yy_last_accepting_cpos = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 87 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_current_state != 86 );
		yy_cp = yyg->yy_last_accepting_cpos;
		yy_current_state = yyg->yy_last_accepting_state;

yy_find_action:
		yy_act = yy_accept[yy_current_state];

		YY_DO_BEFORE_ACTION;

do_action:	

		switch ( yy_act )
	{ 
			case 0: 
			
			*yy_cp = yyg->yy_hold_char;
			yy_cp = yyg->yy_last_accepting_cpos;
			yy_current_state = yyg->yy_last_accepting_state;
			goto yy_find_action;

case 1:
YY_RULE_SETUP

	YY_BREAK


 
case 2:
YY_RULE_SETUP
{ BEGIN(COMMENT); }
	YY_BREAK
case 3:
YY_RULE_SETUP

	YY_BREAK
case 4:
YY_RULE_SETUP

	YY_BREAK
case 5:

YY_RULE_SETUP
{ ++yylineno; }
	YY_BREAK
case 6:
YY_RULE_SETUP
{
    yyextra->leadingSpace = true;
    BEGIN(INITIAL);
}
	YY_BREAK
case 7:
YY_RULE_SETUP
{
    
    yylval->assign(1, yytext[0]);
    return yyextra->lineStart ? pp::Token::PP_HASH : pp::Token::PP_OTHER;
}
	YY_BREAK
case 8:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::IDENTIFIER;
}
	YY_BREAK
case 9:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::CONST_INT;
}
	YY_BREAK
case 10:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::CONST_FLOAT;
}
	YY_BREAK


case 11:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::PP_NUMBER;
}
	YY_BREAK
case 12:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_INC;
}
	YY_BREAK
case 13:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_DEC;
}
	YY_BREAK
case 14:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_LEFT;
}
	YY_BREAK
case 15:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_RIGHT;
}
	YY_BREAK
case 16:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_LE;
}
	YY_BREAK
case 17:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_GE;
}
	YY_BREAK
case 18:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_EQ;
}
	YY_BREAK
case 19:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_NE;
}
	YY_BREAK
case 20:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_AND;
}
	YY_BREAK
case 21:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_XOR;
}
	YY_BREAK
case 22:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_OR;
}
	YY_BREAK
case 23:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_ADD_ASSIGN;
}
	YY_BREAK
case 24:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_SUB_ASSIGN;
}
	YY_BREAK
case 25:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_MUL_ASSIGN;
}
	YY_BREAK
case 26:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_DIV_ASSIGN;
}
	YY_BREAK
case 27:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_MOD_ASSIGN;
}
	YY_BREAK
case 28:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_LEFT_ASSIGN;
}
	YY_BREAK
case 29:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_RIGHT_ASSIGN;
}
	YY_BREAK
case 30:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_AND_ASSIGN;
}
	YY_BREAK
case 31:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_XOR_ASSIGN;
}
	YY_BREAK
case 32:
YY_RULE_SETUP
{
    yylval->assign(yytext, yyleng);
    return pp::Token::OP_OR_ASSIGN;
}
	YY_BREAK
case 33:
YY_RULE_SETUP
{
    yylval->assign(1, yytext[0]);
    return yytext[0];
}
	YY_BREAK
case 34:
YY_RULE_SETUP
{ yyextra->leadingSpace = true; }
	YY_BREAK
case 35:

YY_RULE_SETUP
{
    ++yylineno;
    yylval->assign(1, '\n');
    return '\n';
}
	YY_BREAK
case 36:

YY_RULE_SETUP
{ ++yylineno; }
	YY_BREAK
case 37:
YY_RULE_SETUP
{
    yylval->assign(1, yytext[0]);
    return pp::Token::PP_OTHER;
}
	YY_BREAK
case YY_STATE_EOF(INITIAL):
case YY_STATE_EOF(COMMENT):
{
    
    
    pp::Input* input = &yyextra->input;
    pp::Input::Location* scanLoc = &yyextra->scanLoc;
    yy_size_t sIndexMax = input->count() ? input->count() - 1 : 0;
    if (scanLoc->sIndex != sIndexMax)
    {
        
        
        scanLoc->sIndex = sIndexMax; scanLoc->cIndex = 0;
        
        yyfileno = static_cast<int>(sIndexMax); yylineno = 1;
    }
    yylloc->file = yyfileno;
    yylloc->line = yylineno;
    yylval->clear();

    if (YY_START == COMMENT)
    {
        yyextra->diagnostics->report(pp::Diagnostics::EOF_IN_COMMENT,
                                     pp::SourceLocation(yyfileno, yylineno),
                                     "");
    }
    yyterminate();
}
	YY_BREAK
case 38:
YY_RULE_SETUP
ECHO;
	YY_BREAK

	case YY_END_OF_BUFFER:
		{
		
		int yy_amount_of_matched_text = (int) (yy_cp - yyg->yytext_ptr) - 1;

		
		*yy_cp = yyg->yy_hold_char;
		YY_RESTORE_YY_MORE_OFFSET

		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
			{
			








			yyg->yy_n_chars = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = yyin;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		






		if ( yyg->yy_c_buf_p <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] )
			{ 
			yy_state_type yy_next_state;

			yyg->yy_c_buf_p = yyg->yytext_ptr + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state( yyscanner );

			








			yy_next_state = yy_try_NUL_trans( yy_current_state , yyscanner);

			yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				
				yy_cp = ++yyg->yy_c_buf_p;
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = yyg->yy_last_accepting_cpos;
				yy_current_state = yyg->yy_last_accepting_state;
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer( yyscanner ) )
			{
			case EOB_ACT_END_OF_FILE:
				{
				yyg->yy_did_buffer_switch_on_eof = 0;

				if ( ppwrap(yyscanner ) )
					{
					








					yyg->yy_c_buf_p = yyg->yytext_ptr + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! yyg->yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				yyg->yy_c_buf_p =
					yyg->yytext_ptr + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state( yyscanner );

				yy_cp = yyg->yy_c_buf_p;
				yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				yyg->yy_c_buf_p =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars];

				yy_current_state = yy_get_previous_state( yyscanner );

				yy_cp = yyg->yy_c_buf_p;
				yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;
				goto yy_find_action;
			}
		break;
		}

	default:
		YY_FATAL_ERROR(
			"fatal flex scanner internal error--no action found" );
	} 
		} 
} 








static int yy_get_next_buffer (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	register char *dest = YY_CURRENT_BUFFER_LVALUE->yy_ch_buf;
	register char *source = yyg->yytext_ptr;
	register int number_to_move, i;
	int ret_val;

	if ( yyg->yy_c_buf_p > &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( YY_CURRENT_BUFFER_LVALUE->yy_fill_buffer == 0 )
		{ 
		if ( yyg->yy_c_buf_p - yyg->yytext_ptr - YY_MORE_ADJ == 1 )
			{
			


			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			


			return EOB_ACT_LAST_MATCH;
			}
		}

	

	
	number_to_move = (int) (yyg->yy_c_buf_p - yyg->yytext_ptr) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		


		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars = 0;

	else
		{
			yy_size_t num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ 

			
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER;

			int yy_c_buf_p_offset =
				(int) (yyg->yy_c_buf_p - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				yy_size_t new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					
					pprealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2 ,yyscanner );
				}
			else
				
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			yyg->yy_c_buf_p = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = YY_CURRENT_BUFFER_LVALUE->yy_buf_size -
						number_to_move - 1;

			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		
		YY_INPUT( (&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move]),
			yyg->yy_n_chars, num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	if ( yyg->yy_n_chars == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			pprestart(yyin  ,yyscanner);
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	if ((yy_size_t) (yyg->yy_n_chars + number_to_move) > YY_CURRENT_BUFFER_LVALUE->yy_buf_size) {
		
		yy_size_t new_size = yyg->yy_n_chars + number_to_move + (yyg->yy_n_chars >> 1);
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) pprealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size ,yyscanner );
		if ( ! YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			YY_FATAL_ERROR( "out of dynamic memory in yy_get_next_buffer()" );
	}

	yyg->yy_n_chars += number_to_move;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] = YY_END_OF_BUFFER_CHAR;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars + 1] = YY_END_OF_BUFFER_CHAR;

	yyg->yytext_ptr = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[0];

	return ret_val;
}



    static yy_state_type yy_get_previous_state (yyscan_t yyscanner)
{
	register yy_state_type yy_current_state;
	register char *yy_cp;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	yy_current_state = yyg->yy_start;

	for ( yy_cp = yyg->yytext_ptr + YY_MORE_ADJ; yy_cp < yyg->yy_c_buf_p; ++yy_cp )
		{
		register YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			yyg->yy_last_accepting_state = yy_current_state;
			yyg->yy_last_accepting_cpos = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 87 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
}






    static yy_state_type yy_try_NUL_trans  (yy_state_type yy_current_state , yyscan_t yyscanner)
{
	register int yy_is_jam;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner; 
	register char *yy_cp = yyg->yy_c_buf_p;

	register YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		yyg->yy_last_accepting_state = yy_current_state;
		yyg->yy_last_accepting_cpos = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 87 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 86);

	return yy_is_jam ? 0 : yy_current_state;
}

#ifndef YY_NO_INPUT
#ifdef __cplusplus
    static int yyinput (yyscan_t yyscanner)
#else
    static int input  (yyscan_t yyscanner)
#endif

{
	int c;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	*yyg->yy_c_buf_p = yyg->yy_hold_char;

	if ( *yyg->yy_c_buf_p == YY_END_OF_BUFFER_CHAR )
		{
		



		if ( yyg->yy_c_buf_p < &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] )
			
			*yyg->yy_c_buf_p = '\0';

		else
			{ 
			yy_size_t offset = yyg->yy_c_buf_p - yyg->yytext_ptr;
			++yyg->yy_c_buf_p;

			switch ( yy_get_next_buffer( yyscanner ) )
				{
				case EOB_ACT_LAST_MATCH:
					









					
					pprestart(yyin ,yyscanner);

					

				case EOB_ACT_END_OF_FILE:
					{
					if ( ppwrap(yyscanner ) )
						return EOF;

					if ( ! yyg->yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput(yyscanner);
#else
					return input(yyscanner);
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					yyg->yy_c_buf_p = yyg->yytext_ptr + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) yyg->yy_c_buf_p;	
	*yyg->yy_c_buf_p = '\0';	
	yyg->yy_hold_char = *++yyg->yy_c_buf_p;

	return c;
}
#endif	






    void pprestart  (FILE * input_file , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! YY_CURRENT_BUFFER ){
        ppensure_buffer_stack (yyscanner);
		YY_CURRENT_BUFFER_LVALUE =
            pp_create_buffer(yyin,YY_BUF_SIZE ,yyscanner);
	}

	pp_init_buffer(YY_CURRENT_BUFFER,input_file ,yyscanner);
	pp_load_buffer_state(yyscanner );
}





    void pp_switch_to_buffer  (YY_BUFFER_STATE  new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	




	ppensure_buffer_stack (yyscanner);
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	pp_load_buffer_state(yyscanner );

	




	yyg->yy_did_buffer_switch_on_eof = 1;
}

static void pp_load_buffer_state  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	yyg->yy_n_chars = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	yyg->yytext_ptr = yyg->yy_c_buf_p = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	yyin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	yyg->yy_hold_char = *yyg->yy_c_buf_p;
}







    YY_BUFFER_STATE pp_create_buffer  (FILE * file, int  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) ppalloc(sizeof( struct yy_buffer_state ) ,yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in pp_create_buffer()" );

	b->yy_buf_size = size;

	


	b->yy_ch_buf = (char *) ppalloc(b->yy_buf_size + 2 ,yyscanner );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in pp_create_buffer()" );

	b->yy_is_our_buffer = 1;

	pp_init_buffer(b,file ,yyscanner);

	return b;
}





    void pp_delete_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) 
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		ppfree((void *) b->yy_ch_buf ,yyscanner );

	ppfree((void *) b ,yyscanner );
}





    static void pp_init_buffer  (YY_BUFFER_STATE  b, FILE * file , yyscan_t yyscanner)

{
	int oerrno = errno;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	pp_flush_buffer(b ,yyscanner);

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    



    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = 0;
    
	errno = oerrno;
}





    void pp_flush_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if ( ! b )
		return;

	b->yy_n_chars = 0;

	



	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == YY_CURRENT_BUFFER )
		pp_load_buffer_state(yyscanner );
}







void pppush_buffer_state (YY_BUFFER_STATE new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (new_buffer == NULL)
		return;

	ppensure_buffer_stack(yyscanner);

	
	if ( YY_CURRENT_BUFFER )
		{
		
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	
	if (YY_CURRENT_BUFFER)
		yyg->yy_buffer_stack_top++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	
	pp_load_buffer_state(yyscanner );
	yyg->yy_did_buffer_switch_on_eof = 1;
}





void pppop_buffer_state (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (!YY_CURRENT_BUFFER)
		return;

	pp_delete_buffer(YY_CURRENT_BUFFER ,yyscanner);
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if (yyg->yy_buffer_stack_top > 0)
		--yyg->yy_buffer_stack_top;

	if (YY_CURRENT_BUFFER) {
		pp_load_buffer_state(yyscanner );
		yyg->yy_did_buffer_switch_on_eof = 1;
	}
}




static void ppensure_buffer_stack (yyscan_t yyscanner)
{
	yy_size_t num_to_alloc;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if (!yyg->yy_buffer_stack) {

		



		num_to_alloc = 1;
		yyg->yy_buffer_stack = (struct yy_buffer_state**)ppalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in ppensure_buffer_stack()" );
								  
		memset(yyg->yy_buffer_stack, 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		yyg->yy_buffer_stack_max = num_to_alloc;
		yyg->yy_buffer_stack_top = 0;
		return;
	}

	if (yyg->yy_buffer_stack_top >= (yyg->yy_buffer_stack_max) - 1){

		
		int grow_size = 8 ;

		num_to_alloc = yyg->yy_buffer_stack_max + grow_size;
		yyg->yy_buffer_stack = (struct yy_buffer_state**)pprealloc
								(yyg->yy_buffer_stack,
								num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in ppensure_buffer_stack()" );

		
		memset(yyg->yy_buffer_stack + yyg->yy_buffer_stack_max, 0, grow_size * sizeof(struct yy_buffer_state*));
		yyg->yy_buffer_stack_max = num_to_alloc;
	}
}







YY_BUFFER_STATE pp_scan_buffer  (char * base, yy_size_t  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		
		return 0;

	b = (YY_BUFFER_STATE) ppalloc(sizeof( struct yy_buffer_state ) ,yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in pp_scan_buffer()" );

	b->yy_buf_size = size - 2;	
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	pp_switch_to_buffer(b ,yyscanner );

	return b;
}









YY_BUFFER_STATE pp_scan_string (yyconst char * yystr , yyscan_t yyscanner)
{
    
	return pp_scan_bytes(yystr,strlen(yystr) ,yyscanner);
}








YY_BUFFER_STATE pp_scan_bytes  (yyconst char * yybytes, yy_size_t  _yybytes_len , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n, i;
    
	
	n = _yybytes_len + 2;
	buf = (char *) ppalloc(n ,yyscanner );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in pp_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = pp_scan_buffer(buf,n ,yyscanner);
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in pp_scan_bytes()" );

	


	b->yy_is_our_buffer = 1;

	return b;
}

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yy_fatal_error (yyconst char* msg , yyscan_t yyscanner)
{
    	(void) fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
}



#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		yytext[yyleng] = yyg->yy_hold_char; \
		yyg->yy_c_buf_p = yytext + yyless_macro_arg; \
		yyg->yy_hold_char = *yyg->yy_c_buf_p; \
		*yyg->yy_c_buf_p = '\0'; \
		yyleng = yyless_macro_arg; \
		} \
	while ( 0 )






YY_EXTRA_TYPE ppget_extra  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyextra;
}




int ppget_lineno  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    
        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yylineno;
}




int ppget_column  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    
        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yycolumn;
}




FILE *ppget_in  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyin;
}




FILE *ppget_out  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyout;
}




yy_size_t ppget_leng  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyleng;
}





char *ppget_text  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yytext;
}





void ppset_extra (YY_EXTRA_TYPE  user_defined , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyextra = user_defined ;
}





void ppset_lineno (int  line_number , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        
        if (! YY_CURRENT_BUFFER )
           yy_fatal_error( "ppset_lineno called with no buffer" , yyscanner); 
    
    yylineno = line_number;
}





void ppset_column (int  column_no , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        
        if (! YY_CURRENT_BUFFER )
           yy_fatal_error( "ppset_column called with no buffer" , yyscanner); 
    
    yycolumn = column_no;
}







void ppset_in (FILE *  in_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyin = in_str ;
}

void ppset_out (FILE *  out_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyout = out_str ;
}

int ppget_debug  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yy_flex_debug;
}

void ppset_debug (int  bdebug , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yy_flex_debug = bdebug ;
}



YYSTYPE * ppget_lval  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yylval;
}

void ppset_lval (YYSTYPE *  yylval_param , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yylval = yylval_param;
}

YYLTYPE *ppget_lloc  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yylloc;
}
    
void ppset_lloc (YYLTYPE *  yylloc_param , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yylloc = yylloc_param;
}
    







int pplex_init(yyscan_t* ptr_yy_globals)

{
    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }

    *ptr_yy_globals = (yyscan_t) ppalloc ( sizeof( struct yyguts_t ), NULL );

    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }

    
    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));

    return yy_init_globals ( *ptr_yy_globals );
}









int pplex_init_extra(YY_EXTRA_TYPE yy_user_defined,yyscan_t* ptr_yy_globals )

{
    struct yyguts_t dummy_yyguts;

    ppset_extra (yy_user_defined, &dummy_yyguts);

    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }
	
    *ptr_yy_globals = (yyscan_t) ppalloc ( sizeof( struct yyguts_t ), &dummy_yyguts );
	
    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }
    
    

    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));
    
    ppset_extra (yy_user_defined, *ptr_yy_globals);
    
    return yy_init_globals ( *ptr_yy_globals );
}

static int yy_init_globals (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    



    yyg->yy_buffer_stack = 0;
    yyg->yy_buffer_stack_top = 0;
    yyg->yy_buffer_stack_max = 0;
    yyg->yy_c_buf_p = (char *) 0;
    yyg->yy_init = 0;
    yyg->yy_start = 0;

    yyg->yy_start_stack_ptr = 0;
    yyg->yy_start_stack_depth = 0;
    yyg->yy_start_stack =  NULL;


#ifdef YY_STDINIT
    yyin = stdin;
    yyout = stdout;
#else
    yyin = (FILE *) 0;
    yyout = (FILE *) 0;
#endif

    


    return 0;
}


int pplex_destroy  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

    
	while(YY_CURRENT_BUFFER){
		pp_delete_buffer(YY_CURRENT_BUFFER ,yyscanner );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		pppop_buffer_state(yyscanner);
	}

	
	ppfree(yyg->yy_buffer_stack ,yyscanner);
	yyg->yy_buffer_stack = NULL;

    
        ppfree(yyg->yy_start_stack ,yyscanner );
        yyg->yy_start_stack = NULL;

    

    yy_init_globals( yyscanner);

    
    ppfree ( yyscanner , yyscanner );
    yyscanner = NULL;
    return 0;
}





#ifndef yytext_ptr
static void yy_flex_strncpy (char* s1, yyconst char * s2, int n , yyscan_t yyscanner)
{
	register int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
}
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * s , yyscan_t yyscanner)
{
	register int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
}
#endif

void *ppalloc (yy_size_t  size , yyscan_t yyscanner)
{
	return (void *) malloc( size );
}

void *pprealloc  (void * ptr, yy_size_t  size , yyscan_t yyscanner)
{
	






	return (void *) realloc( (char *) ptr, size );
}

void ppfree (void * ptr , yyscan_t yyscanner)
{
	free( (char *) ptr );	
}

#define YYTABLES_NAME "yytables"

namespace pp {



const size_t Tokenizer::kMaxTokenLength = 256;

Tokenizer::Tokenizer(Diagnostics* diagnostics) : mHandle(0)
{
    mContext.diagnostics = diagnostics;
}

Tokenizer::~Tokenizer()
{
    destroyScanner();
}

bool Tokenizer::init(size_t count, const char* const string[], const int length[])
{
    if ((count > 0) && (string == 0)) return false;

    mContext.input = Input(count, string, length);
    return initScanner();
}

void Tokenizer::setFileNumber(int file)
{
    
    
    ppset_column(file,mHandle);
}

void Tokenizer::setLineNumber(int line)
{
    ppset_lineno(line,mHandle);
}

void Tokenizer::lex(Token* token)
{
    token->type = pplex(&token->text,&token->location,mHandle);
    if (token->text.size() > kMaxTokenLength)
    {
        mContext.diagnostics->report(Diagnostics::TOKEN_TOO_LONG,
                                     token->location, token->text);
        token->text.erase(kMaxTokenLength);
    }

    token->flags = 0;

    token->setAtStartOfLine(mContext.lineStart);
    mContext.lineStart = token->type == '\n';

    token->setHasLeadingSpace(mContext.leadingSpace);
    mContext.leadingSpace = false;
}

bool Tokenizer::initScanner()
{
    if ((mHandle == NULL) && pplex_init_extra(&mContext,&mHandle))
        return false;

    pprestart(0,mHandle);
    return true;
}

void Tokenizer::destroyScanner()
{
    if (mHandle == NULL)
        return;

    pplex_destroy(mHandle);
    mHandle = NULL;
}

}  

