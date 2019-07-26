#line 17 "./glslang.l"









#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#elif defined(_MSC_VER)
#pragma warning(disable: 4065)
#pragma warning(disable: 4189)
#pragma warning(disable: 4505)
#pragma warning(disable: 4701)
#endif



#line 25 "./glslang_lex.cpp"

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


#define YY_NEW_FILE yyrestart(yyin ,yyscanner )

#define YY_END_OF_BUFFER_CHAR 0


#ifndef YY_BUF_SIZE
#define YY_BUF_SIZE 16384
#endif



#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    






    #define  YY_LESS_LINENO(n) \
            do { \
                int yyl;\
                for ( yyl = n; yyl < yyleng; ++yyl )\
                    if ( yytext[yyl] == '\n' )\
                        --yylineno;\
            }while(0)
    

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

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		
	char *yy_buf_pos;		

	


	yy_size_t yy_buf_size;

	


	int yy_n_chars;

	



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

void yyrestart (FILE *input_file ,yyscan_t yyscanner );
void yy_switch_to_buffer (YY_BUFFER_STATE new_buffer ,yyscan_t yyscanner );
YY_BUFFER_STATE yy_create_buffer (FILE *file,int size ,yyscan_t yyscanner );
void yy_delete_buffer (YY_BUFFER_STATE b ,yyscan_t yyscanner );
void yy_flush_buffer (YY_BUFFER_STATE b ,yyscan_t yyscanner );
void yypush_buffer_state (YY_BUFFER_STATE new_buffer ,yyscan_t yyscanner );
void yypop_buffer_state (yyscan_t yyscanner );

static void yyensure_buffer_stack (yyscan_t yyscanner );
static void yy_load_buffer_state (yyscan_t yyscanner );
static void yy_init_buffer (YY_BUFFER_STATE b,FILE *file ,yyscan_t yyscanner );

#define YY_FLUSH_BUFFER yy_flush_buffer(YY_CURRENT_BUFFER ,yyscanner)

YY_BUFFER_STATE yy_scan_buffer (char *base,yy_size_t size ,yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_string (yyconst char *yy_str ,yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_bytes (yyconst char *bytes,int len ,yyscan_t yyscanner );

void *yyalloc (yy_size_t ,yyscan_t yyscanner );
void *yyrealloc (void *,yy_size_t ,yyscan_t yyscanner );
void yyfree (void * ,yyscan_t yyscanner );

#define yy_new_buffer yy_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        yyensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer(yyin,YY_BUF_SIZE ,yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        yyensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer(yyin,YY_BUF_SIZE ,yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)



#define yywrap(n) 1
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
	yyleng = (size_t) (yy_cp - yy_bp); \
	yyg->yy_hold_char = *yy_cp; \
	*yy_cp = '\0'; \
	yyg->yy_c_buf_p = yy_cp;

#define YY_NUM_RULES 155
#define YY_END_OF_BUFFER 156


struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[459] =
    {   0,
        0,    0,    0,    0,    0,    0,  156,  154,  153,  153,
      138,  144,  149,  133,  134,  142,  141,  130,  139,  137,
      143,  102,  102,  131,  127,  145,  132,  146,  150,   98,
      135,  136,  148,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,  128,  147,  129,  140,    3,    4,    3,
      152,  155,  151,  124,  110,  129,  118,  113,  108,  116,
      106,  117,  107,  105,    2,    1,  109,  104,  100,  101,
        0,    0,  102,  136,  128,  135,  125,  121,  123,  122,
      126,   98,  114,  120,   98,   98,   98,   98,   98,   98,

       98,   98,   98,   98,   17,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   20,   22,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,  115,  119,    5,
      151,    0,    1,  104,    0,    0,  103,   99,  111,  112,
       50,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   18,
       98,   98,   98,   98,   98,   98,   98,   98,   26,   98,
       98,   98,   98,   98,   98,   98,   98,   23,   98,   98,

       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,    0,  105,    0,  104,   98,   28,   98,   98,   95,
       98,   98,   98,   98,   98,   98,   98,   21,   53,   98,
       98,   98,   69,   98,   98,   58,   73,   98,   98,   98,
       98,   98,   98,   98,   98,   70,    9,   33,   34,   35,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   98,   98,   98,   98,   56,   29,   98,   98,
       98,   98,   98,   98,   36,   37,   38,   27,   98,   98,
       98,   15,   42,   43,   44,   51,   12,   98,   98,   98,

       98,   82,   83,   84,   98,   30,   74,   25,   85,   86,
       87,    7,   79,   80,   81,   98,   24,   77,   98,   98,
       39,   40,   41,   98,   98,   98,   98,   98,   98,   98,
       98,   98,   71,   98,   98,   98,   98,   98,   98,   98,
       98,   52,   98,   97,   98,   98,   19,   98,   98,   98,
       98,   72,   66,   61,   98,   98,   98,   98,   98,   78,
       57,   98,   64,   32,   98,   94,   65,   49,   76,   59,
       98,   98,   98,   98,   98,   98,   98,   98,   60,   31,
       98,   98,   98,    8,   98,   98,   98,   98,   98,   54,
       13,   98,   14,   98,   98,   16,   67,   98,   98,   98,

       62,   98,   98,   98,   98,   98,   98,   55,   75,   63,
       11,   68,    6,   96,   10,   88,   45,   89,   98,   98,
       98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
       46,   98,   98,   98,   98,   98,   98,   98,   48,   98,
       92,   98,   98,   98,   98,   98,   90,   98,   91,   98,
       98,   98,   98,   98,   98,   47,   93,    0
    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        2,    2,    2,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    4,    1,    1,    1,    5,    6,    1,    7,
        8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
       18,   19,   20,   20,   20,   21,   21,   22,   23,   24,
       25,   26,   27,    1,   28,   28,   29,   30,   31,   28,
       32,   32,   32,   32,   32,   32,   32,   32,   33,   32,
       32,   34,   35,   32,   32,   32,   32,   36,   32,   32,
       37,    1,   38,   39,   32,    1,   40,   41,   42,   43,

       44,   45,   46,   47,   48,   32,   49,   50,   51,   52,
       53,   54,   32,   55,   56,   57,   58,   59,   60,   61,
       62,   63,   64,   65,   66,   67,    1,    1,    1,    1,
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

static yyconst flex_int32_t yy_meta[68] =
    {   0,
        1,    1,    2,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    3,    3,    3,    3,    3,    3,
        3,    1,    1,    1,    1,    1,    1,    3,    3,    3,
        3,    4,    4,    4,    4,    4,    1,    1,    1,    3,
        3,    3,    3,    3,    3,    4,    4,    4,    4,    4,
        4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
        4,    4,    4,    1,    1,    1,    1
    } ;

static yyconst flex_int16_t yy_base[464] =
    {   0,
        0,    0,   65,   66,   75,    0,  680,  681,  681,  681,
      654,   45,  137,  681,  681,  653,  134,  681,  133,  131,
      146,  159,  168,  651,  681,  186,  651,   47,  681,    0,
      681,  681,  128,  100,  110,  152,  156,  146,  166,  622,
      173,  109,  621,  126,  177,  615,  178,  628,  187,  184,
      141,  197,  624,  681,  157,  681,  681,  681,  681,  656,
      681,  681,    0,  681,  681,  681,  681,  681,  681,  681,
      681,  681,  681,  236,  681,    0,  681,  243,  273,  282,
      304,    0,  314,  681,  681,  681,  644,  681,  681,  681,
      643,    0,  681,  681,  616,  609,  612,  620,  619,  606,

      621,  608,  614,  602,  599,  612,  599,  596,  596,  602,
      590,  189,  595,  605,  591,  597,  600,  601,    0,  216,
      600,  188,  586,  599,  590,  592,  582,  596,  593,  595,
      578,  583,  580,  569,  183,  577,  582,  578,  580,  569,
      572,  220,  577,  569,  581,  176,  574,  681,  681,  681,
        0,  331,    0,  344,  361,  290,  374,    0,  681,  681,
        0,  566,  570,  579,  576,  560,  560,  215,  575,  572,
      572,  570,  567,  559,  565,  552,  563,  549,  565,    0,
      562,  550,  557,  554,  558,  551,  540,  539,  552,  555,
      552,  547,  538,  260,  543,  546,  537,  534,  538,  544,

      535,  526,  529,  527,  537,  523,  521,  534,  520,  522,
      519,  530,  529,  283,  524,  519,  508,  264,  526,  528,
      517,  381,  388,  395,  402,  518,    0,  516,  320,    0,
      508,  506,  514,  503,  520,  509,  336,    0,    0,  503,
      513,  513,    0,  498,  349,    0,    0,  500,  366,  501,
      495,  494,  495,  494,  407,    0,    0,    0,    0,    0,
      490,  491,  496,  487,  500,  495,  494,  486,  490,  482,
      485,  489,  494,  480,  492,  483,    0,    0,  489,  478,
      478,  483,  482,  479,    0,    0,    0,    0,  469,  481,
      483,    0,    0,    0,    0,    0,    0,  471,  472,  466,

      476,    0,    0,    0,  467,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,  474,    0,    0,  472,  468,
        0,    0,    0,  464,  460,  465,  455,  468,  454,  467,
      456,  463,    0,  461,  463,  447,  449,  455,  461,  456,
      444,    0,  446,    0,  445,  448,    0,  437,  436,  436,
      449,    0,  451,    0,  450,  449,  434,  447,  434,    0,
        0,  437,    0,    0,  429,    0,    0,    0,    0,    0,
      426,  437,  430,  436,  433,  428,  420,  432,    0,    0,
      425,  432,  421,    0,  430,  427,  417,  411,  425,    0,
        0,  425,    0,  423,  422,    0,    0,  421,  407,  419,

        0,  410,  431,  430,  429,  400,  396,    0,    0,    0,
        0,    0,    0,    0,    0,  421,  250,  421,  411,  384,
      392,  394,  390,  392,  391,  390,  393,  390,  391,  388,
        0,  332,  343,  317,  329,  313,  317,  304,  321,  291,
        0,  302,  280,  271,  255,  262,    0,  256,    0,  232,
      206,  212,  148,  159,  113,    0,    0,  681,  442,  444,
      446,  450,  161
    } ;

static yyconst flex_int16_t yy_def[464] =
    {   0,
      458,    1,  459,  459,  458,    5,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  460,
      458,  458,  458,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  461,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  462,  458,  458,  458,  458,
      458,  463,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  460,  458,  458,  460,  460,  460,  460,  460,  460,

      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  458,  458,  458,
      461,  458,  462,  458,  458,  458,  458,  463,  458,  458,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,

      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  458,  458,  458,  458,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,

      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,

      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,  460,  460,  460,
      460,  460,  460,  460,  460,  460,  460,    0,  458,  458,
      458,  458,  458
    } ;

static yyconst flex_int16_t yy_nxt[749] =
    {   0,
        8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
       18,   19,   20,   21,   22,   23,   23,   23,   23,   23,
       23,   24,   25,   26,   27,   28,   29,   30,   30,   30,
       30,   30,   30,   30,   30,   30,   31,   32,   33,   34,
       35,   36,   37,   38,   39,   40,   41,   42,   30,   43,
       44,   45,   46,   47,   48,   49,   50,   51,   52,   53,
       30,   30,   30,   54,   55,   56,   57,   59,   59,   65,
       66,   90,   91,   60,   60,    8,   61,   62,    8,    8,
        8,    8,    8,    8,    8,    8,    8,    8,    8,    8,
        8,    8,    8,    8,    8,    8,    8,    8,    8,    8,

        8,    8,   63,   63,   63,   63,   63,   63,   63,   63,
       63,    8,    8,    8,   63,   63,   63,   63,   63,   63,
       63,   63,   63,   63,   63,   63,   63,   63,   63,   63,
       63,   63,   63,   63,   63,   63,   63,   63,    8,    8,
        8,    8,   67,   70,   72,   74,   74,   74,   74,   74,
       74,   74,   93,  119,   75,   95,   96,   73,   71,   76,
      120,   68,   97,  158,   98,  123,   94,  121,   99,  124,
       77,   78,  457,   79,   79,   79,   79,   79,   79,   80,
       78,  148,   83,   83,   83,   83,   83,   83,   83,   81,
       85,  100,  142,  456,   82,  107,  143,  108,   81,  103,

      455,  101,   81,  104,  102,  110,  109,   86,  105,   87,
       88,   81,  116,  111,  106,  112,  125,  128,  113,   82,
      117,  149,  206,  219,  114,  220,  132,  138,  178,  126,
      139,  118,  129,  133,  134,  130,  144,  207,  140,  192,
      145,  179,  454,  135,  136,  141,  137,  193,  453,  146,
       74,   74,   74,   74,   74,   74,   74,  154,  154,  154,
      154,  154,  154,  154,  452,  186,  152,  214,  187,  188,
      232,  233,  189,  155,  190,  215,  258,  259,  260,  152,
      285,  286,  287,  422,  423,   78,  155,   79,   79,   79,
       79,   79,   79,   80,   78,  451,   80,   80,   80,   80,

       80,   80,   80,   81,  157,  157,  157,  157,  157,  157,
      157,  450,   81,  156,  449,  156,   81,  448,  157,  157,
      157,  157,  157,  157,  157,   81,   78,  280,   83,   83,
       83,   83,   83,   83,   83,  281,  293,  294,  295,  447,
      222,  446,  222,  445,   81,  223,  223,  223,  223,  223,
      223,  223,  302,  303,  304,  444,  443,   81,  154,  154,
      154,  154,  154,  154,  154,  309,  310,  311,  442,  441,
      224,  440,  224,  439,  155,  225,  225,  225,  225,  225,
      225,  225,  313,  314,  315,  438,  437,  155,  157,  157,
      157,  157,  157,  157,  157,  223,  223,  223,  223,  223,

      223,  223,  223,  223,  223,  223,  223,  223,  223,  225,
      225,  225,  225,  225,  225,  225,  225,  225,  225,  225,
      225,  225,  225,  321,  322,  323,  403,  404,  405,  436,
      435,  434,  433,  432,  431,  430,  429,  428,  427,  406,
      426,  407,   58,   58,   58,   58,   92,   92,  151,  151,
      153,  425,  153,  153,  424,  421,  420,  419,  418,  417,
      416,  415,  414,  413,  412,  411,  410,  409,  408,  402,
      401,  400,  399,  398,  397,  396,  395,  394,  393,  392,
      391,  390,  389,  388,  387,  386,  385,  384,  383,  382,
      381,  380,  379,  378,  377,  376,  375,  374,  373,  372,

      371,  370,  369,  368,  367,  366,  365,  364,  363,  362,
      361,  360,  359,  358,  357,  356,  355,  354,  353,  352,
      351,  350,  349,  348,  347,  346,  345,  344,  343,  342,
      341,  340,  339,  338,  337,  336,  335,  334,  333,  332,
      331,  330,  329,  328,  327,  326,  325,  324,  320,  319,
      318,  317,  316,  312,  308,  307,  306,  305,  301,  300,
      299,  298,  297,  296,  292,  291,  290,  289,  288,  284,
      283,  282,  279,  278,  277,  276,  275,  274,  273,  272,
      271,  270,  269,  268,  267,  266,  265,  264,  263,  262,
      261,  257,  256,  255,  254,  253,  252,  251,  250,  249,

      248,  247,  246,  245,  244,  243,  242,  241,  240,  239,
      238,  237,  236,  235,  234,  231,  230,  229,  228,  227,
      226,  221,  218,  217,  216,  213,  212,  211,  210,  209,
      208,  205,  204,  203,  202,  201,  200,  199,  198,  197,
      196,  195,  194,  191,  185,  184,  183,  182,  181,  180,
      177,  176,  175,  174,  173,  172,  171,  170,  169,  168,
      167,  166,  165,  164,  163,  162,  161,  160,  159,  150,
      147,  131,  127,  122,  115,   89,   84,   69,   64,  458,
        7,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,

      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458
    } ;

static yyconst flex_int16_t yy_chk[749] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    3,    4,   12,
       12,   28,   28,    3,    4,    5,    5,    5,    5,    5,
        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,

        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
        5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
        5,    5,   13,   17,   19,   20,   20,   20,   20,   20,
       20,   20,   33,   42,   21,   34,   34,   19,   17,   21,
       42,   13,   35,  463,   35,   44,   33,   42,   35,   44,
       21,   22,  455,   22,   22,   22,   22,   22,   22,   22,
       23,   55,   23,   23,   23,   23,   23,   23,   23,   22,
       26,   36,   51,  454,   22,   38,   51,   38,   23,   37,

      453,   36,   22,   37,   36,   39,   38,   26,   37,   26,
       26,   23,   41,   39,   37,   39,   45,   47,   39,   22,
       41,   55,  135,  146,   39,  146,   49,   50,  112,   45,
       50,   41,   47,   49,   49,   47,   52,  135,   50,  122,
       52,  112,  452,   49,   49,   50,   49,  122,  451,   52,
       74,   74,   74,   74,   74,   74,   74,   78,   78,   78,
       78,   78,   78,   78,  450,  120,   74,  142,  120,  120,
      168,  168,  120,   78,  120,  142,  194,  194,  194,   74,
      218,  218,  218,  417,  417,   79,   78,   79,   79,   79,
       79,   79,   79,   79,   80,  448,   80,   80,   80,   80,

       80,   80,   80,   79,  156,  156,  156,  156,  156,  156,
      156,  446,   80,   81,  445,   81,   79,  444,   81,   81,
       81,   81,   81,   81,   81,   80,   83,  214,   83,   83,
       83,   83,   83,   83,   83,  214,  229,  229,  229,  443,
      152,  442,  152,  440,   83,  152,  152,  152,  152,  152,
      152,  152,  237,  237,  237,  439,  438,   83,  154,  154,
      154,  154,  154,  154,  154,  245,  245,  245,  437,  436,
      155,  435,  155,  434,  154,  155,  155,  155,  155,  155,
      155,  155,  249,  249,  249,  433,  432,  154,  157,  157,
      157,  157,  157,  157,  157,  222,  222,  222,  222,  222,

      222,  222,  223,  223,  223,  223,  223,  223,  223,  224,
      224,  224,  224,  224,  224,  224,  225,  225,  225,  225,
      225,  225,  225,  255,  255,  255,  388,  388,  388,  430,
      429,  428,  427,  426,  425,  424,  423,  422,  421,  388,
      420,  388,  459,  459,  459,  459,  460,  460,  461,  461,
      462,  419,  462,  462,  418,  416,  407,  406,  405,  404,
      403,  402,  400,  399,  398,  395,  394,  392,  389,  387,
      386,  385,  383,  382,  381,  378,  377,  376,  375,  374,
      373,  372,  371,  365,  362,  359,  358,  357,  356,  355,
      353,  351,  350,  349,  348,  346,  345,  343,  341,  340,

      339,  338,  337,  336,  335,  334,  332,  331,  330,  329,
      328,  327,  326,  325,  324,  320,  319,  316,  305,  301,
      300,  299,  298,  291,  290,  289,  284,  283,  282,  281,
      280,  279,  276,  275,  274,  273,  272,  271,  270,  269,
      268,  267,  266,  265,  264,  263,  262,  261,  254,  253,
      252,  251,  250,  248,  244,  242,  241,  240,  236,  235,
      234,  233,  232,  231,  228,  226,  221,  220,  219,  217,
      216,  215,  213,  212,  211,  210,  209,  208,  207,  206,
      205,  204,  203,  202,  201,  200,  199,  198,  197,  196,
      195,  193,  192,  191,  190,  189,  188,  187,  186,  185,

      184,  183,  182,  181,  179,  178,  177,  176,  175,  174,
      173,  172,  171,  170,  169,  167,  166,  165,  164,  163,
      162,  147,  145,  144,  143,  141,  140,  139,  138,  137,
      136,  134,  133,  132,  131,  130,  129,  128,  127,  126,
      125,  124,  123,  121,  118,  117,  116,  115,  114,  113,
      111,  110,  109,  108,  107,  106,  105,  104,  103,  102,
      101,  100,   99,   98,   97,   96,   95,   91,   87,   60,
       53,   48,   46,   43,   40,   27,   24,   16,   11,    7,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,

      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458,  458,  458,
      458,  458,  458,  458,  458,  458,  458,  458
    } ;


static yyconst flex_int32_t yy_rule_can_match_eol[156] =
    {   0,
0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,     };




#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET















#include "compiler/glslang.h"
#include "compiler/ParseHelper.h"
#include "compiler/preprocessor/Token.h"
#include "compiler/util.h"
#include "glslang_tab.h"


#ifdef _MSC_VER
#pragma warning(disable : 4102)
#endif

#define YY_USER_ACTION yylval->lex.line = yylineno;
#define YY_INPUT(buf, result, max_size) \
    result = string_input(buf, max_size, yyscanner);

static int string_input(char* buf, int max_size, yyscan_t yyscanner);
static int check_type(yyscan_t yyscanner);
static int reserved_word(yyscan_t yyscanner);

#define INITIAL 0
#define COMMENT 1
#define FIELDS 2

#define YY_EXTRA_TYPE TParseContext*


struct yyguts_t
    {

    
    YY_EXTRA_TYPE yyextra_r;

    
    FILE *yyin_r, *yyout_r;
    size_t yy_buffer_stack_top; 
    size_t yy_buffer_stack_max; 
    YY_BUFFER_STATE * yy_buffer_stack; 
    char yy_hold_char;
    int yy_n_chars;
    int yyleng_r;
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

    }; 

static int yy_init_globals (yyscan_t yyscanner );

    

    #    define yylval yyg->yylval_r
    
int yylex_init (yyscan_t* scanner);

int yylex_init_extra (YY_EXTRA_TYPE user_defined,yyscan_t* scanner);




int yylex_destroy (yyscan_t yyscanner );

int yyget_debug (yyscan_t yyscanner );

void yyset_debug (int debug_flag ,yyscan_t yyscanner );

YY_EXTRA_TYPE yyget_extra (yyscan_t yyscanner );

void yyset_extra (YY_EXTRA_TYPE user_defined ,yyscan_t yyscanner );

FILE *yyget_in (yyscan_t yyscanner );

void yyset_in  (FILE * in_str ,yyscan_t yyscanner );

FILE *yyget_out (yyscan_t yyscanner );

void yyset_out  (FILE * out_str ,yyscan_t yyscanner );

int yyget_leng (yyscan_t yyscanner );

char *yyget_text (yyscan_t yyscanner );

int yyget_lineno (yyscan_t yyscanner );

void yyset_lineno (int line_number ,yyscan_t yyscanner );

YYSTYPE * yyget_lval (yyscan_t yyscanner );

void yyset_lval (YYSTYPE * yylval_param ,yyscan_t yyscanner );





#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int yywrap (yyscan_t yyscanner );
#else
extern int yywrap (yyscan_t yyscanner );
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

    static void yy_push_state (int new_state ,yyscan_t yyscanner);
    
    static void yy_pop_state (yyscan_t yyscanner );
    
    static int yy_top_state (yyscan_t yyscanner );
    

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
		int n; \
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

extern int yylex \
               (YYSTYPE * yylval_param ,yyscan_t yyscanner);

#define YY_DECL int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner)
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

    TParseContext* context = yyextra;

    

    yylval = yylval_param;

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
			yyensure_buffer_stack (yyscanner);
			YY_CURRENT_BUFFER_LVALUE =
				yy_create_buffer(yyin,YY_BUF_SIZE ,yyscanner);
		}

		yy_load_buffer_state(yyscanner );
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
				if ( yy_current_state >= 459 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_current_state != 458 );
		yy_cp = yyg->yy_last_accepting_cpos;
		yy_current_state = yyg->yy_last_accepting_state;

yy_find_action:
		yy_act = yy_accept[yy_current_state];

		YY_DO_BEFORE_ACTION;

		if ( yy_act != YY_END_OF_BUFFER && yy_rule_can_match_eol[yy_act] )
			{
			int yyl;
			for ( yyl = 0; yyl < yyleng; ++yyl )
				if ( yytext[yyl] == '\n' )
					   
    do{ yylineno++;
        yycolumn=0;
    }while(0)
;
			}

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
;
	YY_BREAK

case 2:
YY_RULE_SETUP
{ yy_push_state(COMMENT, yyscanner); }
	YY_BREAK
case 3:
case 4:

YY_RULE_SETUP
;
	YY_BREAK
case 5:
YY_RULE_SETUP
{ yy_pop_state(yyscanner); }
	YY_BREAK
case 6:
YY_RULE_SETUP
{ return(INVARIANT); }
	YY_BREAK
case 7:
YY_RULE_SETUP
{ return(HIGH_PRECISION); }
	YY_BREAK
case 8:
YY_RULE_SETUP
{ return(MEDIUM_PRECISION); }
	YY_BREAK
case 9:
YY_RULE_SETUP
{ return(LOW_PRECISION); }
	YY_BREAK
case 10:
YY_RULE_SETUP
{ return(PRECISION); }
	YY_BREAK
case 11:
YY_RULE_SETUP
{ return(ATTRIBUTE); }
	YY_BREAK
case 12:
YY_RULE_SETUP
{ return(CONST_QUAL); }
	YY_BREAK
case 13:
YY_RULE_SETUP
{ return(UNIFORM); }
	YY_BREAK
case 14:
YY_RULE_SETUP
{ return(VARYING); }
	YY_BREAK
case 15:
YY_RULE_SETUP
{ return(BREAK); }
	YY_BREAK
case 16:
YY_RULE_SETUP
{ return(CONTINUE); }
	YY_BREAK
case 17:
YY_RULE_SETUP
{ return(DO); }
	YY_BREAK
case 18:
YY_RULE_SETUP
{ return(FOR); }
	YY_BREAK
case 19:
YY_RULE_SETUP
{ return(WHILE); }
	YY_BREAK
case 20:
YY_RULE_SETUP
{ return(IF); }
	YY_BREAK
case 21:
YY_RULE_SETUP
{ return(ELSE); }
	YY_BREAK
case 22:
YY_RULE_SETUP
{ return(IN_QUAL); }
	YY_BREAK
case 23:
YY_RULE_SETUP
{ return(OUT_QUAL); }
	YY_BREAK
case 24:
YY_RULE_SETUP
{ return(INOUT_QUAL); }
	YY_BREAK
case 25:
YY_RULE_SETUP
{ context->lexAfterType = true; return(FLOAT_TYPE); }
	YY_BREAK
case 26:
YY_RULE_SETUP
{ context->lexAfterType = true; return(INT_TYPE); }
	YY_BREAK
case 27:
YY_RULE_SETUP
{ context->lexAfterType = true; return(VOID_TYPE); }
	YY_BREAK
case 28:
YY_RULE_SETUP
{ context->lexAfterType = true; return(BOOL_TYPE); }
	YY_BREAK
case 29:
YY_RULE_SETUP
{ yylval->lex.b = true;  return(BOOLCONSTANT); }
	YY_BREAK
case 30:
YY_RULE_SETUP
{ yylval->lex.b = false; return(BOOLCONSTANT); }
	YY_BREAK
case 31:
YY_RULE_SETUP
{ return(DISCARD); }
	YY_BREAK
case 32:
YY_RULE_SETUP
{ return(RETURN); }
	YY_BREAK
case 33:
YY_RULE_SETUP
{ context->lexAfterType = true; return(MATRIX2); }
	YY_BREAK
case 34:
YY_RULE_SETUP
{ context->lexAfterType = true; return(MATRIX3); }
	YY_BREAK
case 35:
YY_RULE_SETUP
{ context->lexAfterType = true; return(MATRIX4); }
	YY_BREAK
case 36:
YY_RULE_SETUP
{ context->lexAfterType = true; return (VEC2); }
	YY_BREAK
case 37:
YY_RULE_SETUP
{ context->lexAfterType = true; return (VEC3); }
	YY_BREAK
case 38:
YY_RULE_SETUP
{ context->lexAfterType = true; return (VEC4); }
	YY_BREAK
case 39:
YY_RULE_SETUP
{ context->lexAfterType = true; return (IVEC2); }
	YY_BREAK
case 40:
YY_RULE_SETUP
{ context->lexAfterType = true; return (IVEC3); }
	YY_BREAK
case 41:
YY_RULE_SETUP
{ context->lexAfterType = true; return (IVEC4); }
	YY_BREAK
case 42:
YY_RULE_SETUP
{ context->lexAfterType = true; return (BVEC2); }
	YY_BREAK
case 43:
YY_RULE_SETUP
{ context->lexAfterType = true; return (BVEC3); }
	YY_BREAK
case 44:
YY_RULE_SETUP
{ context->lexAfterType = true; return (BVEC4); }
	YY_BREAK
case 45:
YY_RULE_SETUP
{ context->lexAfterType = true; return SAMPLER2D; }
	YY_BREAK
case 46:
YY_RULE_SETUP
{ context->lexAfterType = true; return SAMPLERCUBE; }
	YY_BREAK
case 47:
YY_RULE_SETUP
{ context->lexAfterType = true; return SAMPLER_EXTERNAL_OES; }
	YY_BREAK
case 48:
YY_RULE_SETUP
{ context->lexAfterType = true; return SAMPLER2DRECT; }
	YY_BREAK
case 49:
YY_RULE_SETUP
{ context->lexAfterType = true; return(STRUCT); }
	YY_BREAK
case 50:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 51:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 52:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 53:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 54:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 55:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 56:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 57:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 58:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 59:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 60:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 61:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 62:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 63:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 64:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 65:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 66:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 67:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 68:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 69:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 70:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 71:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 72:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 73:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 74:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 75:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 76:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 77:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 78:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 79:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 80:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 81:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 82:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 83:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 84:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 85:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 86:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 87:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 88:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 89:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 90:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 91:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 92:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 93:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 94:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 95:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 96:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 97:
YY_RULE_SETUP
{ return reserved_word(yyscanner); }
	YY_BREAK
case 98:
YY_RULE_SETUP
{
   yylval->lex.string = NewPoolTString(yytext); 
   return check_type(yyscanner);
}
	YY_BREAK
case 99:
YY_RULE_SETUP
{ yylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 100:
YY_RULE_SETUP
{ yylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 101:
YY_RULE_SETUP
{ context->error(yylineno, "Invalid Octal number.", yytext); context->recover(); return 0;}
	YY_BREAK
case 102:
YY_RULE_SETUP
{ yylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 103:
YY_RULE_SETUP
{ yylval->lex.f = static_cast<float>(atof_dot(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 104:
YY_RULE_SETUP
{ yylval->lex.f = static_cast<float>(atof_dot(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 105:
YY_RULE_SETUP
{ yylval->lex.f = static_cast<float>(atof_dot(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 106:
YY_RULE_SETUP
{  return(ADD_ASSIGN); }
	YY_BREAK
case 107:
YY_RULE_SETUP
{  return(SUB_ASSIGN); }
	YY_BREAK
case 108:
YY_RULE_SETUP
{  return(MUL_ASSIGN); }
	YY_BREAK
case 109:
YY_RULE_SETUP
{  return(DIV_ASSIGN); }
	YY_BREAK
case 110:
YY_RULE_SETUP
{  return(MOD_ASSIGN); }
	YY_BREAK
case 111:
YY_RULE_SETUP
{  return(LEFT_ASSIGN); }
	YY_BREAK
case 112:
YY_RULE_SETUP
{  return(RIGHT_ASSIGN); }
	YY_BREAK
case 113:
YY_RULE_SETUP
{  return(AND_ASSIGN); }
	YY_BREAK
case 114:
YY_RULE_SETUP
{  return(XOR_ASSIGN); }
	YY_BREAK
case 115:
YY_RULE_SETUP
{  return(OR_ASSIGN); }
	YY_BREAK
case 116:
YY_RULE_SETUP
{  return(INC_OP); }
	YY_BREAK
case 117:
YY_RULE_SETUP
{  return(DEC_OP); }
	YY_BREAK
case 118:
YY_RULE_SETUP
{  return(AND_OP); }
	YY_BREAK
case 119:
YY_RULE_SETUP
{  return(OR_OP); }
	YY_BREAK
case 120:
YY_RULE_SETUP
{  return(XOR_OP); }
	YY_BREAK
case 121:
YY_RULE_SETUP
{  return(LE_OP); }
	YY_BREAK
case 122:
YY_RULE_SETUP
{  return(GE_OP); }
	YY_BREAK
case 123:
YY_RULE_SETUP
{  return(EQ_OP); }
	YY_BREAK
case 124:
YY_RULE_SETUP
{  return(NE_OP); }
	YY_BREAK
case 125:
YY_RULE_SETUP
{  return(LEFT_OP); }
	YY_BREAK
case 126:
YY_RULE_SETUP
{  return(RIGHT_OP); }
	YY_BREAK
case 127:
YY_RULE_SETUP
{ context->lexAfterType = false; return(SEMICOLON); }
	YY_BREAK
case 128:
YY_RULE_SETUP
{ context->lexAfterType = false; return(LEFT_BRACE); }
	YY_BREAK
case 129:
YY_RULE_SETUP
{ return(RIGHT_BRACE); }
	YY_BREAK
case 130:
YY_RULE_SETUP
{ if (context->inTypeParen) context->lexAfterType = false; return(COMMA); }
	YY_BREAK
case 131:
YY_RULE_SETUP
{ return(COLON); }
	YY_BREAK
case 132:
YY_RULE_SETUP
{ context->lexAfterType = false; return(EQUAL); }
	YY_BREAK
case 133:
YY_RULE_SETUP
{ context->lexAfterType = false; context->inTypeParen = true; return(LEFT_PAREN); }
	YY_BREAK
case 134:
YY_RULE_SETUP
{ context->inTypeParen = false; return(RIGHT_PAREN); }
	YY_BREAK
case 135:
YY_RULE_SETUP
{ return(LEFT_BRACKET); }
	YY_BREAK
case 136:
YY_RULE_SETUP
{ return(RIGHT_BRACKET); }
	YY_BREAK
case 137:
YY_RULE_SETUP
{ BEGIN(FIELDS);  return(DOT); }
	YY_BREAK
case 138:
YY_RULE_SETUP
{ return(BANG); }
	YY_BREAK
case 139:
YY_RULE_SETUP
{ return(DASH); }
	YY_BREAK
case 140:
YY_RULE_SETUP
{ return(TILDE); }
	YY_BREAK
case 141:
YY_RULE_SETUP
{ return(PLUS); }
	YY_BREAK
case 142:
YY_RULE_SETUP
{ return(STAR); }
	YY_BREAK
case 143:
YY_RULE_SETUP
{ return(SLASH); }
	YY_BREAK
case 144:
YY_RULE_SETUP
{ return(PERCENT); }
	YY_BREAK
case 145:
YY_RULE_SETUP
{ return(LEFT_ANGLE); }
	YY_BREAK
case 146:
YY_RULE_SETUP
{ return(RIGHT_ANGLE); }
	YY_BREAK
case 147:
YY_RULE_SETUP
{ return(VERTICAL_BAR); }
	YY_BREAK
case 148:
YY_RULE_SETUP
{ return(CARET); }
	YY_BREAK
case 149:
YY_RULE_SETUP
{ return(AMPERSAND); }
	YY_BREAK
case 150:
YY_RULE_SETUP
{ return(QUESTION); }
	YY_BREAK
case 151:
YY_RULE_SETUP
{ 
    BEGIN(INITIAL);
    yylval->lex.string = NewPoolTString(yytext); 
    return FIELD_SELECTION;
}
	YY_BREAK
case 152:
YY_RULE_SETUP
{}
	YY_BREAK
case 153:

YY_RULE_SETUP
{  }
	YY_BREAK
case YY_STATE_EOF(INITIAL):
case YY_STATE_EOF(COMMENT):
case YY_STATE_EOF(FIELDS):
{ context->AfterEOF = true; yyterminate(); }
	YY_BREAK
case 154:
YY_RULE_SETUP
{ context->warning(yylineno, "Unknown char", yytext, ""); return 0; }
	YY_BREAK
case 155:
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

				if ( yywrap(yyscanner ) )
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
			int num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ 

			
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER;

			int yy_c_buf_p_offset =
				(int) (yyg->yy_c_buf_p - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					
					yyrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2 ,yyscanner );
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
			yyg->yy_n_chars, (size_t) num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	if ( yyg->yy_n_chars == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			yyrestart(yyin  ,yyscanner);
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
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) yyrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size ,yyscanner );
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
			if ( yy_current_state >= 459 )
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
		if ( yy_current_state >= 459 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 458);

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
			int offset = yyg->yy_c_buf_p - yyg->yytext_ptr;
			++yyg->yy_c_buf_p;

			switch ( yy_get_next_buffer( yyscanner ) )
				{
				case EOB_ACT_LAST_MATCH:
					









					
					yyrestart(yyin ,yyscanner);

					

				case EOB_ACT_END_OF_FILE:
					{
					if ( yywrap(yyscanner ) )
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

	if ( c == '\n' )
		   
    do{ yylineno++;
        yycolumn=0;
    }while(0)
;

	return c;
}
#endif	






    void yyrestart  (FILE * input_file , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! YY_CURRENT_BUFFER ){
        yyensure_buffer_stack (yyscanner);
		YY_CURRENT_BUFFER_LVALUE =
            yy_create_buffer(yyin,YY_BUF_SIZE ,yyscanner);
	}

	yy_init_buffer(YY_CURRENT_BUFFER,input_file ,yyscanner);
	yy_load_buffer_state(yyscanner );
}





    void yy_switch_to_buffer  (YY_BUFFER_STATE  new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	




	yyensure_buffer_stack (yyscanner);
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	yy_load_buffer_state(yyscanner );

	




	yyg->yy_did_buffer_switch_on_eof = 1;
}

static void yy_load_buffer_state  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	yyg->yy_n_chars = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	yyg->yytext_ptr = yyg->yy_c_buf_p = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	yyin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	yyg->yy_hold_char = *yyg->yy_c_buf_p;
}







    YY_BUFFER_STATE yy_create_buffer  (FILE * file, int  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) yyalloc(sizeof( struct yy_buffer_state ) ,yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_buf_size = size;

	


	b->yy_ch_buf = (char *) yyalloc(b->yy_buf_size + 2 ,yyscanner );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_is_our_buffer = 1;

	yy_init_buffer(b,file ,yyscanner);

	return b;
}





    void yy_delete_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) 
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		yyfree((void *) b->yy_ch_buf ,yyscanner );

	yyfree((void *) b ,yyscanner );
}





    static void yy_init_buffer  (YY_BUFFER_STATE  b, FILE * file , yyscan_t yyscanner)

{
	int oerrno = errno;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	yy_flush_buffer(b ,yyscanner);

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    



    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = 0;
    
	errno = oerrno;
}





    void yy_flush_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
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
		yy_load_buffer_state(yyscanner );
}







void yypush_buffer_state (YY_BUFFER_STATE new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (new_buffer == NULL)
		return;

	yyensure_buffer_stack(yyscanner);

	
	if ( YY_CURRENT_BUFFER )
		{
		
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	
	if (YY_CURRENT_BUFFER)
		yyg->yy_buffer_stack_top++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	
	yy_load_buffer_state(yyscanner );
	yyg->yy_did_buffer_switch_on_eof = 1;
}





void yypop_buffer_state (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (!YY_CURRENT_BUFFER)
		return;

	yy_delete_buffer(YY_CURRENT_BUFFER ,yyscanner);
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if (yyg->yy_buffer_stack_top > 0)
		--yyg->yy_buffer_stack_top;

	if (YY_CURRENT_BUFFER) {
		yy_load_buffer_state(yyscanner );
		yyg->yy_did_buffer_switch_on_eof = 1;
	}
}




static void yyensure_buffer_stack (yyscan_t yyscanner)
{
	int num_to_alloc;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if (!yyg->yy_buffer_stack) {

		



		num_to_alloc = 1;
		yyg->yy_buffer_stack = (struct yy_buffer_state**)yyalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );
								  
		memset(yyg->yy_buffer_stack, 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		yyg->yy_buffer_stack_max = num_to_alloc;
		yyg->yy_buffer_stack_top = 0;
		return;
	}

	if (yyg->yy_buffer_stack_top >= (yyg->yy_buffer_stack_max) - 1){

		
		int grow_size = 8 ;

		num_to_alloc = yyg->yy_buffer_stack_max + grow_size;
		yyg->yy_buffer_stack = (struct yy_buffer_state**)yyrealloc
								(yyg->yy_buffer_stack,
								num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );

		
		memset(yyg->yy_buffer_stack + yyg->yy_buffer_stack_max, 0, grow_size * sizeof(struct yy_buffer_state*));
		yyg->yy_buffer_stack_max = num_to_alloc;
	}
}







YY_BUFFER_STATE yy_scan_buffer  (char * base, yy_size_t  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		
		return 0;

	b = (YY_BUFFER_STATE) yyalloc(sizeof( struct yy_buffer_state ) ,yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_buffer()" );

	b->yy_buf_size = size - 2;	
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	yy_switch_to_buffer(b ,yyscanner );

	return b;
}









YY_BUFFER_STATE yy_scan_string (yyconst char * yystr , yyscan_t yyscanner)
{
    
	return yy_scan_bytes(yystr,strlen(yystr) ,yyscanner);
}








YY_BUFFER_STATE yy_scan_bytes  (yyconst char * yybytes, int  _yybytes_len , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	
	n = _yybytes_len + 2;
	buf = (char *) yyalloc(n ,yyscanner );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = yy_scan_buffer(buf,n ,yyscanner);
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in yy_scan_bytes()" );

	


	b->yy_is_our_buffer = 1;

	return b;
}

    static void yy_push_state (int  new_state , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if ( yyg->yy_start_stack_ptr >= yyg->yy_start_stack_depth )
		{
		yy_size_t new_size;

		yyg->yy_start_stack_depth += YY_START_STACK_INCR;
		new_size = yyg->yy_start_stack_depth * sizeof( int );

		if ( ! yyg->yy_start_stack )
			yyg->yy_start_stack = (int *) yyalloc(new_size ,yyscanner );

		else
			yyg->yy_start_stack = (int *) yyrealloc((void *) yyg->yy_start_stack,new_size ,yyscanner );

		if ( ! yyg->yy_start_stack )
			YY_FATAL_ERROR( "out of memory expanding start-condition stack" );
		}

	yyg->yy_start_stack[yyg->yy_start_stack_ptr++] = YY_START;

	BEGIN(new_state);
}

    static void yy_pop_state  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if ( --yyg->yy_start_stack_ptr < 0 )
		YY_FATAL_ERROR( "start-condition stack underflow" );

	BEGIN(yyg->yy_start_stack[yyg->yy_start_stack_ptr]);
}

    static int yy_top_state  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	return yyg->yy_start_stack[yyg->yy_start_stack_ptr - 1];
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






YY_EXTRA_TYPE yyget_extra  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyextra;
}




int yyget_lineno  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    
        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yylineno;
}




int yyget_column  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    
        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yycolumn;
}




FILE *yyget_in  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyin;
}




FILE *yyget_out  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyout;
}




int yyget_leng  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyleng;
}





char *yyget_text  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yytext;
}





void yyset_extra (YY_EXTRA_TYPE  user_defined , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyextra = user_defined ;
}





void yyset_lineno (int  line_number , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        
        if (! YY_CURRENT_BUFFER )
           yy_fatal_error( "yyset_lineno called with no buffer" , yyscanner); 
    
    yylineno = line_number;
}





void yyset_column (int  column_no , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        
        if (! YY_CURRENT_BUFFER )
           yy_fatal_error( "yyset_column called with no buffer" , yyscanner); 
    
    yycolumn = column_no;
}







void yyset_in (FILE *  in_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyin = in_str ;
}

void yyset_out (FILE *  out_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyout = out_str ;
}

int yyget_debug  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yy_flex_debug;
}

void yyset_debug (int  bdebug , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yy_flex_debug = bdebug ;
}



YYSTYPE * yyget_lval  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yylval;
}

void yyset_lval (YYSTYPE *  yylval_param , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yylval = yylval_param;
}








int yylex_init(yyscan_t* ptr_yy_globals)

{
    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }

    *ptr_yy_globals = (yyscan_t) yyalloc ( sizeof( struct yyguts_t ), NULL );

    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }

    
    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));

    return yy_init_globals ( *ptr_yy_globals );
}









int yylex_init_extra(YY_EXTRA_TYPE yy_user_defined,yyscan_t* ptr_yy_globals )

{
    struct yyguts_t dummy_yyguts;

    yyset_extra (yy_user_defined, &dummy_yyguts);

    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }
	
    *ptr_yy_globals = (yyscan_t) yyalloc ( sizeof( struct yyguts_t ), &dummy_yyguts );
	
    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }
    
    

    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));
    
    yyset_extra (yy_user_defined, *ptr_yy_globals);
    
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


int yylex_destroy  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

    
	while(YY_CURRENT_BUFFER){
		yy_delete_buffer(YY_CURRENT_BUFFER ,yyscanner );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		yypop_buffer_state(yyscanner);
	}

	
	yyfree(yyg->yy_buffer_stack ,yyscanner);
	yyg->yy_buffer_stack = NULL;

    
        yyfree(yyg->yy_start_stack ,yyscanner );
        yyg->yy_start_stack = NULL;

    

    yy_init_globals( yyscanner);

    
    yyfree ( yyscanner , yyscanner );
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

void *yyalloc (yy_size_t  size , yyscan_t yyscanner)
{
	return (void *) malloc( size );
}

void *yyrealloc  (void * ptr, yy_size_t  size , yyscan_t yyscanner)
{
	






	return (void *) realloc( (char *) ptr, size );
}

void yyfree (void * ptr , yyscan_t yyscanner)
{
	free( (char *) ptr );	
}

#define YYTABLES_NAME "yytables"

int string_input(char* buf, int max_size, yyscan_t yyscanner) {
    pp::Token token;
    yyget_extra(yyscanner)->preprocessor.lex(&token);
    int len = token.type == pp::Token::LAST ? 0 : token.text.size();
    if ((len > 0) && (len < max_size))
        memcpy(buf, token.text.c_str(), len);
    yyset_lineno(EncodeSourceLoc(token.location.file, token.location.line),yyscanner);

    if (len >= max_size)
        YY_FATAL_ERROR("Input buffer overflow");
    else if (len > 0)
        buf[len++] = ' ';
    return len;
}

int check_type(yyscan_t yyscanner) {
    struct yyguts_t* yyg = (struct yyguts_t*) yyscanner;
    
    int token = IDENTIFIER;
    TSymbol* symbol = yyextra->symbolTable.find(yytext);
    if (yyextra->lexAfterType == false && symbol && symbol->isVariable()) {
        TVariable* variable = static_cast<TVariable*>(symbol);
        if (variable->isUserType()) {
            yyextra->lexAfterType = true;
            token = TYPE_NAME;
        }
    }
    yylval->lex.symbol = symbol;
    return token;
}

int reserved_word(yyscan_t yyscanner) {
    struct yyguts_t* yyg = (struct yyguts_t*) yyscanner;

    yyextra->error(yylineno, "Illegal use of reserved word", yytext, "");
    yyextra->recover();
    return 0;
}

void yyerror(TParseContext* context, const char* reason) {
    struct yyguts_t* yyg = (struct yyguts_t*) context->scanner;

    if (context->AfterEOF) {
        context->error(yylineno, reason, "unexpected EOF");
    } else {
        context->error(yylineno, reason, yytext);
    }
    context->recover();
}

int glslang_initialize(TParseContext* context) {
    yyscan_t scanner = NULL;
    if (yylex_init_extra(context,&scanner))
        return 1;

    context->scanner = scanner;
    return 0;
}

int glslang_finalize(TParseContext* context) {
    yyscan_t scanner = context->scanner;
    if (scanner == NULL) return 0;
    
    context->scanner = NULL;
    yylex_destroy(scanner);

    return 0;
}

int glslang_scan(int count, const char* const string[], const int length[],
                 TParseContext* context) {
    yyrestart(NULL,context->scanner);
    yyset_lineno(EncodeSourceLoc(0, 1),context->scanner);
    context->AfterEOF = false;

    
    if (!context->preprocessor.init(count, string, length))
        return 1;

    
    const TExtensionBehavior& extBehavior = context->extensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extBehavior.begin();
         iter != extBehavior.end(); ++iter) {
        context->preprocessor.predefineMacro(iter->first.c_str(), 1);
    }
    return 0;
}

