
#line 3 "generated/glslang.cpp"

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





#define BEGIN (yy_start) = 1 + 2 *





#define YY_START (((yy_start) - 1) / 2)
#define YYSTATE YY_START


#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)


#define YY_NEW_FILE yyrestart(yyin  )

#define YY_END_OF_BUFFER_CHAR 0


#ifndef YY_BUF_SIZE
#ifdef __ia64__




#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif 
#endif



#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

extern int yyleng;

extern FILE *yyin, *yyout;

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
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up yytext again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, (yytext_ptr)  )

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


static size_t yy_buffer_stack_top = 0; 
static size_t yy_buffer_stack_max = 0; 
static YY_BUFFER_STATE * yy_buffer_stack = 0; 







#define YY_CURRENT_BUFFER ( (yy_buffer_stack) \
                          ? (yy_buffer_stack)[(yy_buffer_stack_top)] \
                          : NULL)




#define YY_CURRENT_BUFFER_LVALUE (yy_buffer_stack)[(yy_buffer_stack_top)]


static char yy_hold_char;
static int yy_n_chars;		
int yyleng;


static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		
static int yy_start = 0;	




static int yy_did_buffer_switch_on_eof;

void yyrestart (FILE *input_file  );
void yy_switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE yy_create_buffer (FILE *file,int size  );
void yy_delete_buffer (YY_BUFFER_STATE b  );
void yy_flush_buffer (YY_BUFFER_STATE b  );
void yypush_buffer_state (YY_BUFFER_STATE new_buffer  );
void yypop_buffer_state (void );

static void yyensure_buffer_stack (void );
static void yy_load_buffer_state (void );
static void yy_init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER yy_flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE yy_scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE yy_scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE yy_scan_bytes (yyconst char *bytes,int len  );

void *yyalloc (yy_size_t  );
void *yyrealloc (void *,yy_size_t  );
void yyfree (void *  );

#define yy_new_buffer yy_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        yyensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer(yyin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        yyensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer(yyin,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)



#define yywrap(n) 1
#define YY_SKIP_YYWRAP

typedef unsigned char YY_CHAR;

FILE *yyin = (FILE *) 0, *yyout = (FILE *) 0;

typedef int yy_state_type;

extern int yylineno;

int yylineno = 1;

extern char *yytext;
#define yytext_ptr yytext

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
static void yy_fatal_error (yyconst char msg[]  );




#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	yyleng = (size_t) (yy_cp - yy_bp); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 142
#define YY_END_OF_BUFFER 143


struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[407] =
    {   0,
        0,    0,    0,    0,  143,  141,  140,  140,  125,  131,
      136,  120,  121,  129,  128,  117,  126,  124,  130,   88,
       88,  118,  114,  132,  119,  133,  137,   84,  122,  123,
      135,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,  115,  134,  116,  127,  139,  142,  141,  138,  111,
       97,  116,  105,  100,   95,  103,   93,  104,   94,   91,
       92,    0,   96,   90,   86,   87,    0,    0,   88,  123,
      115,  122,  112,  108,  110,  109,  113,   84,  101,  107,
       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,

       13,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   84,   16,   18,   84,   84,   84,   84,
       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,  102,  106,  138,    0,    0,    1,   90,    0,
        0,   89,   85,   98,   99,   44,   84,   84,   84,   84,
       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   14,   84,   84,   84,   84,   84,   84,
       84,   84,   22,   84,   84,   84,   84,   84,   84,   84,
       84,   19,   84,   84,   84,   84,   84,   84,   84,   84,

       84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   84,    0,   91,    0,   90,   84,   24,
       84,   84,   81,   84,   84,   84,   84,   84,   84,   84,
       17,   47,   84,   84,   84,   84,   84,   52,   66,   84,
       84,   84,   84,   84,   84,   84,   84,   63,    5,   29,
       30,   31,   84,   84,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   84,   84,   84,   84,   50,   25,   84,
       84,   84,   84,   84,   84,   32,   33,   34,   23,   84,
       84,   84,   11,   38,   39,   40,   45,    8,   84,   84,
       84,   84,   77,   78,   79,   84,   26,   67,   21,   74,

       75,   76,    3,   71,   72,   73,   84,   20,   69,   84,
       84,   35,   36,   37,   84,   84,   84,   84,   84,   84,
       84,   84,   84,   64,   84,   84,   84,   84,   84,   84,
       84,   46,   84,   83,   84,   84,   15,   84,   84,   84,
       84,   65,   60,   55,   84,   84,   84,   84,   84,   70,
       51,   84,   58,   28,   84,   80,   59,   43,   53,   84,
       84,   84,   84,   84,   84,   84,   84,   54,   27,   84,
       84,   84,    4,   84,   84,   84,   84,   84,   48,    9,
       84,   10,   84,   84,   12,   61,   84,   84,   84,   56,
       84,   84,   84,   49,   68,   57,    7,   62,    2,   82,

        6,   41,   84,   84,   42,    0
    } ;

static yyconst flex_int32_t yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        2,    2,    2,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    4,    1,    1,    1,    5,    6,    1,    7,
        8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
       18,   19,   16,   16,   16,   20,   20,   21,   22,   23,
       24,   25,   26,    1,   27,   27,   28,   29,   30,   27,
       31,   31,   31,   31,   31,   31,   31,   31,   31,   31,
       31,   31,   31,   31,   31,   31,   31,   32,   31,   31,
       33,    1,   34,   35,   31,    1,   36,   37,   38,   39,

       40,   41,   42,   43,   44,   31,   45,   46,   47,   48,
       49,   50,   31,   51,   52,   53,   54,   55,   56,   57,
       58,   59,   60,   61,   62,   63,    1,    1,    1,    1,
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

static yyconst flex_int32_t yy_meta[64] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    2,    2,    2,    2,    2,    2,
        1,    1,    1,    1,    1,    1,    2,    2,    2,    2,
        3,    3,    1,    1,    1,    2,    2,    2,    2,    2,
        2,    3,    3,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,    3,    3,    3,    1,
        1,    1,    1
    } ;

static yyconst flex_int16_t yy_base[411] =
    {   0,
        0,    0,   63,    0,  600,  601,  601,  601,  575,  103,
      123,  601,  601,  574,  120,  601,  119,  117,  131,  143,
      151,  572,  601,  169,  572,  114,  601,    0,  601,  601,
      117,   96,  102,  136,  140,  130,  150,  546,  110,  156,
      545,  162,  152,  539,  167,  552,  170,  169,  167,  188,
      548,  601,  118,  601,  601,  601,  601,  576,    0,  601,
      601,  601,  601,  601,  601,  601,  601,  601,  601,  214,
      601,  586,  601,  223,  232,  251,  267,    0,  277,  601,
      601,  601,  564,  601,  601,  601,  563,    0,  601,  601,
      539,  532,  535,  543,  542,  529,  544,  531,  537,  525,

      522,  535,  522,  519,  519,  525,  513,  520,  517,  527,
      513,  519,  522,  523,    0,  253,  522,  160,  508,  521,
      512,  514,  504,  518,  515,  517,  500,  505,  502,  491,
      171,  505,  501,  503,  492,  495,  173,  500,  492,  504,
      211,  497,  601,  601,    0,  303,  537,  601,  309,  325,
      335,  341,    0,  601,  601,    0,  488,  492,  501,  498,
      482,  482,  183,  497,  494,  494,  492,  489,  481,  487,
      474,  485,  488,    0,  485,  473,  480,  477,  481,  474,
      463,  462,  475,  478,  475,  470,  461,  241,  466,  469,
      460,  457,  461,  467,  458,  449,  452,  450,  460,  446,

      444,  444,  446,  443,  454,  453,  224,  448,  443,  432,
      257,  450,  452,  441,  347,  353,  359,  365,  442,    0,
      440,  292,    0,  432,  430,  438,  427,  444,  433,  313,
        0,    0,  427,  437,  437,  422,  329,    0,    0,  424,
      369,  425,  419,  418,  419,  418,  372,    0,    0,    0,
        0,    0,  414,  415,  420,  411,  424,  419,  418,  410,
      414,  406,  409,  413,  418,  417,  408,    0,    0,  414,
      403,  403,  408,  407,  404,    0,    0,    0,    0,  394,
      406,  408,    0,    0,    0,    0,    0,    0,  396,  397,
      391,  401,    0,    0,    0,  392,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,  399,    0,    0,  397,
      393,    0,    0,    0,  389,  385,  390,  380,  393,  379,
      392,  381,  388,    0,  386,  388,  372,  381,  387,  382,
      370,    0,  372,    0,  371,  374,    0,  363,  362,  362,
      375,    0,  377,    0,  376,  375,  360,  373,  360,    0,
        0,  363,    0,    0,  355,    0,    0,    0,    0,  352,
      363,  356,  362,  359,  354,  346,  298,    0,    0,  290,
      296,  285,    0,  278,  274,  263,  261,  265,    0,    0,
      265,    0,  261,  260,    0,    0,  258,  235,  240,    0,
      213,  227,  192,    0,    0,    0,    0,    0,    0,    0,

        0,    0,  173,  137,    0,  601,  390,  392,  395,  148
    } ;

static yyconst flex_int16_t yy_def[411] =
    {   0,
      406,    1,  406,    3,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  407,  406,  406,
      406,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  406,  406,  406,  406,  406,  406,  406,  408,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  409,  406,  406,  406,  406,  406,  410,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  407,  406,  406,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,

      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  406,  406,  408,  406,  409,  406,  406,  406,
      406,  406,  410,  406,  406,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,

      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  406,  406,  406,  406,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,

      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,
      407,  407,  407,  407,  407,  407,  407,  407,  407,  407,

      407,  407,  407,  407,  407,    0,  406,  406,  406,  406
    } ;

static yyconst flex_int16_t yy_nxt[665] =
    {   0,
        6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
       16,   17,   18,   19,   20,   21,   21,   21,   21,   21,
       22,   23,   24,   25,   26,   27,   28,   28,   28,   28,
       28,   28,   29,   30,   31,   32,   33,   34,   35,   36,
       37,   38,   39,   40,   28,   41,   42,   43,   44,   45,
       46,   47,   48,   49,   50,   51,   28,   28,   28,   52,
       53,   54,   55,    6,   56,   57,    6,    6,    6,    6,
        6,    6,    6,    6,    6,    6,   58,    6,    6,    6,
        6,    6,    6,    6,    6,    6,    6,    6,    6,   59,
       59,   59,   59,   59,   59,    6,    6,    6,   59,   59,

       59,   59,   59,   59,   59,   59,   59,   59,   59,   59,
       59,   59,   59,   59,   59,   59,   59,   59,   59,   59,
       59,   59,    6,    6,    6,    6,   61,   62,   63,   66,
       68,   70,   70,   70,   70,   70,   70,   86,   87,   71,
       89,  143,   69,   67,   72,  112,   64,   91,   92,  153,
       93,   90,   94,  113,   73,   74,   95,   75,   75,   75,
       75,   75,   76,   74,  114,   79,   79,   79,   79,   79,
       79,   96,   77,   81,   78,  103,  405,  104,  144,   99,
       77,   97,   77,  100,   98,  106,  105,  121,  101,   82,
       77,   83,   84,  107,  102,  108,  115,  119,  109,   78,

      122,  120,  124,  116,  110,  128,  200,  186,  133,  404,
      117,  134,  129,  130,  137,  187,  207,  125,  138,  135,
      126,  201,  131,  139,  208,  132,  136,  140,   70,   70,
       70,   70,   70,   70,  225,  226,  141,  149,  149,  149,
      149,  149,  149,  146,   74,  403,   75,   75,   75,   75,
       75,   76,  150,  146,  212,  402,  213,  250,  251,  252,
      401,   77,  150,   74,  271,   76,   76,   76,   76,   76,
       76,   77,  272,  276,  277,  278,  151,  392,  151,  400,
       77,  152,  152,  152,  152,  152,  152,  399,  393,   74,
       77,   79,   79,   79,   79,   79,   79,  398,  180,  397,

      396,  181,  182,  395,  394,  183,   77,  184,  284,  285,
      286,  391,  215,  390,  215,  389,   77,  216,  216,  216,
      216,  216,  216,  149,  149,  149,  149,  149,  149,  293,
      294,  295,  388,  387,  217,  386,  217,  385,  150,  218,
      218,  218,  218,  218,  218,  300,  301,  302,  150,  152,
      152,  152,  152,  152,  152,  152,  152,  152,  152,  152,
      152,  216,  216,  216,  216,  216,  216,  216,  216,  216,
      216,  216,  216,  218,  218,  218,  218,  218,  218,  218,
      218,  218,  218,  218,  218,  304,  305,  306,  312,  313,
      314,   88,   88,  145,  145,  147,  147,  147,  384,  383,

      382,  381,  380,  379,  378,  377,  376,  375,  374,  373,
      372,  371,  370,  369,  368,  367,  366,  365,  364,  363,
      362,  361,  360,  359,  358,  357,  356,  355,  354,  353,
      352,  351,  350,  349,  348,  347,  346,  345,  344,  343,
      342,  341,  340,  339,  338,  337,  336,  335,  334,  333,
      332,  331,  330,  329,  328,  327,  326,  325,  324,  323,
      322,  321,  320,  319,  318,  317,  316,  315,  311,  310,
      309,  308,  307,  303,  299,  298,  297,  296,  292,  291,
      290,  289,  288,  287,  283,  282,  281,  280,  279,  275,
      274,  273,  270,  269,  268,  267,  266,  265,  264,  263,

      262,  261,  260,  259,  258,  257,  256,  255,  254,  253,
      249,  248,  247,  246,  245,  244,  243,  242,  241,  240,
      239,  238,  237,  236,  235,  234,  233,  232,  231,  230,
      229,  228,  227,  224,  223,  222,  221,  220,  219,  148,
      214,  211,  210,  209,  206,  205,  204,  203,  202,  199,
      198,  197,  196,  195,  194,  193,  192,  191,  190,  189,
      188,  185,  179,  178,  177,  176,  175,  174,  173,  172,
      171,  170,  169,  168,  167,  166,  165,  164,  163,  162,
      161,  160,  159,  158,  157,  156,  155,  154,  148,   72,
      142,  127,  123,  118,  111,   85,   80,   65,   60,  406,

        5,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406
    } ;

static yyconst flex_int16_t yy_chk[665] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,    3,    3,    3,    3,

        3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
        3,    3,    3,    3,    3,    3,   10,   10,   11,   15,
       17,   18,   18,   18,   18,   18,   18,   26,   26,   19,
       31,   53,   17,   15,   19,   39,   11,   32,   32,  410,
       33,   31,   33,   39,   19,   20,   33,   20,   20,   20,
       20,   20,   20,   21,   39,   21,   21,   21,   21,   21,
       21,   34,   20,   24,   20,   36,  404,   36,   53,   35,
       21,   34,   20,   35,   34,   37,   36,   43,   35,   24,
       21,   24,   24,   37,   35,   37,   40,   42,   37,   20,

       43,   42,   45,   40,   37,   47,  131,  118,   48,  403,
       40,   48,   47,   47,   49,  118,  137,   45,   49,   48,
       45,  131,   47,   50,  137,   47,   48,   50,   70,   70,
       70,   70,   70,   70,  163,  163,   50,   74,   74,   74,
       74,   74,   74,   70,   75,  393,   75,   75,   75,   75,
       75,   75,   74,   70,  141,  392,  141,  188,  188,  188,
      391,   75,   74,   76,  207,   76,   76,   76,   76,   76,
       76,   75,  207,  211,  211,  211,   77,  377,   77,  389,
       76,   77,   77,   77,   77,   77,   77,  388,  377,   79,
       76,   79,   79,   79,   79,   79,   79,  387,  116,  384,

      383,  116,  116,  381,  378,  116,   79,  116,  222,  222,
      222,  376,  146,  375,  146,  374,   79,  146,  146,  146,
      146,  146,  146,  149,  149,  149,  149,  149,  149,  230,
      230,  230,  372,  371,  150,  370,  150,  367,  149,  150,
      150,  150,  150,  150,  150,  237,  237,  237,  149,  151,
      151,  151,  151,  151,  151,  152,  152,  152,  152,  152,
      152,  215,  215,  215,  215,  215,  215,  216,  216,  216,
      216,  216,  216,  217,  217,  217,  217,  217,  217,  218,
      218,  218,  218,  218,  218,  241,  241,  241,  247,  247,
      247,  407,  407,  408,  408,  409,  409,  409,  366,  365,

      364,  363,  362,  361,  360,  355,  352,  349,  348,  347,
      346,  345,  343,  341,  340,  339,  338,  336,  335,  333,
      331,  330,  329,  328,  327,  326,  325,  323,  322,  321,
      320,  319,  318,  317,  316,  315,  311,  310,  307,  296,
      292,  291,  290,  289,  282,  281,  280,  275,  274,  273,
      272,  271,  270,  267,  266,  265,  264,  263,  262,  261,
      260,  259,  258,  257,  256,  255,  254,  253,  246,  245,
      244,  243,  242,  240,  236,  235,  234,  233,  229,  228,
      227,  226,  225,  224,  221,  219,  214,  213,  212,  210,
      209,  208,  206,  205,  204,  203,  202,  201,  200,  199,

      198,  197,  196,  195,  194,  193,  192,  191,  190,  189,
      187,  186,  185,  184,  183,  182,  181,  180,  179,  178,
      177,  176,  175,  173,  172,  171,  170,  169,  168,  167,
      166,  165,  164,  162,  161,  160,  159,  158,  157,  147,
      142,  140,  139,  138,  136,  135,  134,  133,  132,  130,
      129,  128,  127,  126,  125,  124,  123,  122,  121,  120,
      119,  117,  114,  113,  112,  111,  110,  109,  108,  107,
      106,  105,  104,  103,  102,  101,  100,   99,   98,   97,
       96,   95,   94,   93,   92,   91,   87,   83,   72,   58,
       51,   46,   44,   41,   38,   25,   22,   14,    9,    5,

      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406,  406,  406,  406,  406,  406,  406,
      406,  406,  406,  406
    } ;


static yyconst flex_int32_t yy_rule_can_match_eol[143] =
    {   0,
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 0, 0,     };

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int yy_flex_debug;
int yy_flex_debug = 0;




#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
char *yytext;





















#include <stdio.h>
#include <stdlib.h>

#include "compiler/ParseHelper.h"
#include "glslang_tab.h"


#ifdef _MSC_VER
#pragma warning(disable : 4102)
#endif

int yy_input(char* buf, int max_size);

extern int yyparse(void*);
#define YY_DECL int yylex(YYSTYPE* pyylval,void* parseContextLocal)
#define parseContext (*((TParseContext*)(parseContextLocal)))
 
#define YY_INPUT(buf,result,max_size) (result = yy_input(buf, max_size))






#define INITIAL 0
#define FIELDS 1

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

static int yy_init_globals (void );




int yylex_destroy (void );

int yyget_debug (void );

void yyset_debug (int debug_flag  );

YY_EXTRA_TYPE yyget_extra (void );

void yyset_extra (YY_EXTRA_TYPE user_defined  );

FILE *yyget_in (void );

void yyset_in  (FILE * in_str  );

FILE *yyget_out (void );

void yyset_out  (FILE * out_str  );

int yyget_leng (void );

char *yyget_text (void );

int yyget_lineno (void );

void yyset_lineno (int line_number  );





#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int yywrap (void );
#else
extern int yywrap (void );
#endif
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy (char *,yyconst char *,int );
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * );
#endif

#ifndef YY_NO_INPUT

#ifdef __cplusplus
static int yyinput (void );
#else
static int input (void );
#endif

#endif


#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__

#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif 
#endif


#ifndef ECHO



#define ECHO do { if (fwrite( yytext, yyleng, 1, yyout )) {} } while (0)
#endif




#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
		{ \
		int c = '*'; \
		size_t n; \
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
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg )
#endif






#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int yylex (void);

#define YY_DECL int yylex (void)
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
    
	if ( !(yy_init) )
		{
		(yy_init) = 1;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! (yy_start) )
			(yy_start) = 1;	

		if ( ! yyin )
			yyin = stdin;

		if ( ! yyout )
			yyout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			yyensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				yy_create_buffer(yyin,YY_BUF_SIZE );
		}

		yy_load_buffer_state( );
		}

	while ( 1 )		
		{
		yy_cp = (yy_c_buf_p);

		
		*yy_cp = (yy_hold_char);

		


		yy_bp = yy_cp;

		yy_current_state = (yy_start);
yy_match:
		do
			{
			register YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
			if ( yy_accept[yy_current_state] )
				{
				(yy_last_accepting_state) = yy_current_state;
				(yy_last_accepting_cpos) = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 407 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_current_state != 406 );
		yy_cp = (yy_last_accepting_cpos);
		yy_current_state = (yy_last_accepting_state);

yy_find_action:
		yy_act = yy_accept[yy_current_state];

		YY_DO_BEFORE_ACTION;

		if ( yy_act != YY_END_OF_BUFFER && yy_rule_can_match_eol[yy_act] )
			{
			int yyl;
			for ( yyl = 0; yyl < yyleng; ++yyl )
				if ( yytext[yyl] == '\n' )
					   
    yylineno++;
;
			}

do_action:	

		switch ( yy_act )
	{ 
			case 0: 
			
			*yy_cp = (yy_hold_char);
			yy_cp = (yy_last_accepting_cpos);
			yy_current_state = (yy_last_accepting_state);
			goto yy_find_action;

case 1:

YY_RULE_SETUP
{  };
	YY_BREAK
case 2:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(INVARIANT); }
	YY_BREAK
case 3:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(HIGH_PRECISION); }
	YY_BREAK
case 4:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(MEDIUM_PRECISION); }
	YY_BREAK
case 5:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LOW_PRECISION); }
	YY_BREAK
case 6:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(PRECISION); }
	YY_BREAK
case 7:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(ATTRIBUTE); }
	YY_BREAK
case 8:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(CONST_QUAL); }
	YY_BREAK
case 9:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(UNIFORM); }
	YY_BREAK
case 10:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(VARYING); }
	YY_BREAK
case 11:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(BREAK); }
	YY_BREAK
case 12:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(CONTINUE); }
	YY_BREAK
case 13:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(DO); }
	YY_BREAK
case 14:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(FOR); }
	YY_BREAK
case 15:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(WHILE); }
	YY_BREAK
case 16:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(IF); }
	YY_BREAK
case 17:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(ELSE); }
	YY_BREAK
case 18:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(IN_QUAL); }
	YY_BREAK
case 19:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(OUT_QUAL); }
	YY_BREAK
case 20:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(INOUT_QUAL); }
	YY_BREAK
case 21:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(FLOAT_TYPE); }
	YY_BREAK
case 22:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(INT_TYPE); }
	YY_BREAK
case 23:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(VOID_TYPE); }
	YY_BREAK
case 24:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(BOOL_TYPE); }
	YY_BREAK
case 25:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; pyylval->lex.b = true;  return(BOOLCONSTANT); }
	YY_BREAK
case 26:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; pyylval->lex.b = false; return(BOOLCONSTANT); }
	YY_BREAK
case 27:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(DISCARD); }
	YY_BREAK
case 28:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RETURN); }
	YY_BREAK
case 29:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(MATRIX2); }
	YY_BREAK
case 30:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(MATRIX3); }
	YY_BREAK
case 31:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(MATRIX4); }
	YY_BREAK
case 32:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (VEC2); }
	YY_BREAK
case 33:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (VEC3); }
	YY_BREAK
case 34:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (VEC4); }
	YY_BREAK
case 35:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (IVEC2); }
	YY_BREAK
case 36:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (IVEC3); }
	YY_BREAK
case 37:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (IVEC4); }
	YY_BREAK
case 38:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (BVEC2); }
	YY_BREAK
case 39:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (BVEC3); }
	YY_BREAK
case 40:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return (BVEC4); }
	YY_BREAK
case 41:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return SAMPLER2D; }
	YY_BREAK
case 42:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return SAMPLERCUBE; }
	YY_BREAK
case 43:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = true; return(STRUCT); }
	YY_BREAK
case 44:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 45:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 46:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 47:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 48:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 49:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 50:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 51:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 52:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 53:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 54:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 55:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 56:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 57:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 58:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 59:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 60:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 61:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 62:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 63:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 64:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 65:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 66:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 67:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 68:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 69:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 70:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 71:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 72:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 73:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 74:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 75:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 76:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 77:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 78:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 79:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 80:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 81:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 82:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 83:
YY_RULE_SETUP
{  PaReservedWord(); return 0; }
	YY_BREAK
case 84:
YY_RULE_SETUP
{  
   pyylval->lex.line = yylineno; 
   pyylval->lex.string = NewPoolTString(yytext); 
   return PaIdentOrType(*pyylval->lex.string, parseContext, pyylval->lex.symbol); 
}
	YY_BREAK
case 85:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 86:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 87:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; parseContext.error(yylineno, "Invalid Octal number.", yytext, "", ""); parseContext.recover(); return 0;}
	YY_BREAK
case 88:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.i = strtol(yytext, 0, 0); return(INTCONSTANT); }
	YY_BREAK
case 89:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.f = static_cast<float>(atof(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 90:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.f = static_cast<float>(atof(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 91:
YY_RULE_SETUP
{ pyylval->lex.line = yylineno; pyylval->lex.f = static_cast<float>(atof(yytext)); return(FLOATCONSTANT); }
	YY_BREAK
case 92:
YY_RULE_SETUP
{  int ret = PaParseComment(pyylval->lex.line, parseContext); if (!ret) return ret; }   
	YY_BREAK
case 93:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(ADD_ASSIGN); }
	YY_BREAK
case 94:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(SUB_ASSIGN); }
	YY_BREAK
case 95:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(MUL_ASSIGN); }
	YY_BREAK
case 96:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(DIV_ASSIGN); }
	YY_BREAK
case 97:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(MOD_ASSIGN); }
	YY_BREAK
case 98:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LEFT_ASSIGN); }
	YY_BREAK
case 99:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RIGHT_ASSIGN); }
	YY_BREAK
case 100:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(AND_ASSIGN); }
	YY_BREAK
case 101:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(XOR_ASSIGN); }
	YY_BREAK
case 102:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(OR_ASSIGN); }
	YY_BREAK
case 103:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(INC_OP); }
	YY_BREAK
case 104:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(DEC_OP); }
	YY_BREAK
case 105:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(AND_OP); }
	YY_BREAK
case 106:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(OR_OP); }
	YY_BREAK
case 107:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(XOR_OP); }
	YY_BREAK
case 108:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LE_OP); }
	YY_BREAK
case 109:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(GE_OP); }
	YY_BREAK
case 110:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(EQ_OP); }
	YY_BREAK
case 111:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(NE_OP); }
	YY_BREAK
case 112:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LEFT_OP); }
	YY_BREAK
case 113:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RIGHT_OP); }
	YY_BREAK
case 114:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = false; return(SEMICOLON); }
	YY_BREAK
case 115:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = false; return(LEFT_BRACE); }
	YY_BREAK
case 116:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RIGHT_BRACE); }
	YY_BREAK
case 117:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; if (parseContext.inTypeParen) parseContext.lexAfterType = false; return(COMMA); }
	YY_BREAK
case 118:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(COLON); }
	YY_BREAK
case 119:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = false; return(EQUAL); }
	YY_BREAK
case 120:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.lexAfterType = false; parseContext.inTypeParen = true; return(LEFT_PAREN); }
	YY_BREAK
case 121:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; parseContext.inTypeParen = false; return(RIGHT_PAREN); }
	YY_BREAK
case 122:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LEFT_BRACKET); }
	YY_BREAK
case 123:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RIGHT_BRACKET); }
	YY_BREAK
case 124:
YY_RULE_SETUP
{ BEGIN(FIELDS);  return(DOT); }
	YY_BREAK
case 125:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(BANG); }
	YY_BREAK
case 126:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(DASH); }
	YY_BREAK
case 127:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(TILDE); }
	YY_BREAK
case 128:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(PLUS); }
	YY_BREAK
case 129:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(STAR); }
	YY_BREAK
case 130:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(SLASH); }
	YY_BREAK
case 131:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(PERCENT); }
	YY_BREAK
case 132:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(LEFT_ANGLE); }
	YY_BREAK
case 133:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(RIGHT_ANGLE); }
	YY_BREAK
case 134:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(VERTICAL_BAR); }
	YY_BREAK
case 135:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(CARET); }
	YY_BREAK
case 136:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(AMPERSAND); }
	YY_BREAK
case 137:
YY_RULE_SETUP
{  pyylval->lex.line = yylineno; return(QUESTION); }
	YY_BREAK
case 138:
YY_RULE_SETUP
{ 
BEGIN(INITIAL);      
    pyylval->lex.line = yylineno;     
    pyylval->lex.string = NewPoolTString(yytext); 
    return FIELD_SELECTION; }
	YY_BREAK
case 139:
YY_RULE_SETUP
{}
	YY_BREAK
case 140:

YY_RULE_SETUP
{  }
	YY_BREAK
case YY_STATE_EOF(INITIAL):
case YY_STATE_EOF(FIELDS):
{ (&parseContext)->AfterEOF = true; yy_delete_buffer(YY_CURRENT_BUFFER); yyterminate();}
	YY_BREAK
case 141:
YY_RULE_SETUP
{ parseContext.infoSink.info << "FLEX: Unknown char " << yytext << "\n";
          return 0; }
	YY_BREAK
case 142:
YY_RULE_SETUP
ECHO;
	YY_BREAK

	case YY_END_OF_BUFFER:
		{
		
		int yy_amount_of_matched_text = (int) (yy_cp - (yytext_ptr)) - 1;

		
		*yy_cp = (yy_hold_char);
		YY_RESTORE_YY_MORE_OFFSET

		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
			{
			








			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = yyin;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		






		if ( (yy_c_buf_p) <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			{ 
			yy_state_type yy_next_state;

			(yy_c_buf_p) = (yytext_ptr) + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state(  );

			








			yy_next_state = yy_try_NUL_trans( yy_current_state );

			yy_bp = (yytext_ptr) + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				
				yy_cp = ++(yy_c_buf_p);
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = (yy_last_accepting_cpos);
				yy_current_state = (yy_last_accepting_state);
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer(  ) )
			{
			case EOB_ACT_END_OF_FILE:
				{
				(yy_did_buffer_switch_on_eof) = 0;

				if ( yywrap( ) )
					{
					








					(yy_c_buf_p) = (yytext_ptr) + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				(yy_c_buf_p) =
					(yytext_ptr) + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				(yy_c_buf_p) =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)];

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
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








static int yy_get_next_buffer (void)
{
    	register char *dest = YY_CURRENT_BUFFER_LVALUE->yy_ch_buf;
	register char *source = (yytext_ptr);
	register int number_to_move, i;
	int ret_val;

	if ( (yy_c_buf_p) > &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( YY_CURRENT_BUFFER_LVALUE->yy_fill_buffer == 0 )
		{ 
		if ( (yy_c_buf_p) - (yytext_ptr) - YY_MORE_ADJ == 1 )
			{
			


			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			


			return EOB_ACT_LAST_MATCH;
			}
		}

	

	
	number_to_move = (int) ((yy_c_buf_p) - (yytext_ptr)) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		


		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars) = 0;

	else
		{
			int num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ 

			
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER;

			int yy_c_buf_p_offset =
				(int) ((yy_c_buf_p) - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					
					yyrealloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
				}
			else
				
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			(yy_c_buf_p) = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = YY_CURRENT_BUFFER_LVALUE->yy_buf_size -
						number_to_move - 1;

			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		
		YY_INPUT( (&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move]),
			(yy_n_chars), (size_t) num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	if ( (yy_n_chars) == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			yyrestart(yyin  );
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

	if ((yy_size_t) ((yy_n_chars) + number_to_move) > YY_CURRENT_BUFFER_LVALUE->yy_buf_size) {
		
		yy_size_t new_size = (yy_n_chars) + number_to_move + ((yy_n_chars) >> 1);
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) yyrealloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
		if ( ! YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			YY_FATAL_ERROR( "out of dynamic memory in yy_get_next_buffer()" );
	}

	(yy_n_chars) += number_to_move;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] = YY_END_OF_BUFFER_CHAR;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] = YY_END_OF_BUFFER_CHAR;

	(yytext_ptr) = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[0];

	return ret_val;
}



    static yy_state_type yy_get_previous_state (void)
{
	register yy_state_type yy_current_state;
	register char *yy_cp;
    
	yy_current_state = (yy_start);

	for ( yy_cp = (yytext_ptr) + YY_MORE_ADJ; yy_cp < (yy_c_buf_p); ++yy_cp )
		{
		register YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			(yy_last_accepting_state) = yy_current_state;
			(yy_last_accepting_cpos) = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 407 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
}






    static yy_state_type yy_try_NUL_trans  (yy_state_type yy_current_state )
{
	register int yy_is_jam;
    	register char *yy_cp = (yy_c_buf_p);

	register YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		(yy_last_accepting_state) = yy_current_state;
		(yy_last_accepting_cpos) = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 407 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 406);

	return yy_is_jam ? 0 : yy_current_state;
}

#ifndef YY_NO_INPUT
#ifdef __cplusplus
    static int yyinput (void)
#else
    static int input  (void)
#endif

{
	int c;
    
	*(yy_c_buf_p) = (yy_hold_char);

	if ( *(yy_c_buf_p) == YY_END_OF_BUFFER_CHAR )
		{
		



		if ( (yy_c_buf_p) < &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			
			*(yy_c_buf_p) = '\0';

		else
			{ 
			int offset = (yy_c_buf_p) - (yytext_ptr);
			++(yy_c_buf_p);

			switch ( yy_get_next_buffer(  ) )
				{
				case EOB_ACT_LAST_MATCH:
					









					
					yyrestart(yyin );

					

				case EOB_ACT_END_OF_FILE:
					{
					if ( yywrap( ) )
						return EOF;

					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput();
#else
					return input();
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					(yy_c_buf_p) = (yytext_ptr) + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) (yy_c_buf_p);	
	*(yy_c_buf_p) = '\0';	
	(yy_hold_char) = *++(yy_c_buf_p);

	if ( c == '\n' )
		   
    yylineno++;
;

	return c;
}
#endif	






    void yyrestart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        yyensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            yy_create_buffer(yyin,YY_BUF_SIZE );
	}

	yy_init_buffer(YY_CURRENT_BUFFER,input_file );
	yy_load_buffer_state( );
}





    void yy_switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	




	yyensure_buffer_stack ();
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	yy_load_buffer_state( );

	




	(yy_did_buffer_switch_on_eof) = 1;
}

static void yy_load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	yyin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}







    YY_BUFFER_STATE yy_create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) yyalloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_buf_size = size;

	


	b->yy_ch_buf = (char *) yyalloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_is_our_buffer = 1;

	yy_init_buffer(b,file );

	return b;
}





    void yy_delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) 
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		yyfree((void *) b->yy_ch_buf  );

	yyfree((void *) b  );
}





    static void yy_init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	yy_flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    



    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = 0;
    
	errno = oerrno;
}





    void yy_flush_buffer (YY_BUFFER_STATE  b )
{
    	if ( ! b )
		return;

	b->yy_n_chars = 0;

	



	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == YY_CURRENT_BUFFER )
		yy_load_buffer_state( );
}







void yypush_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	yyensure_buffer_stack();

	
	if ( YY_CURRENT_BUFFER )
		{
		
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	
	if (YY_CURRENT_BUFFER)
		(yy_buffer_stack_top)++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	
	yy_load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}





void yypop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	yy_delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		yy_load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}




static void yyensure_buffer_stack (void)
{
	int num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		



		num_to_alloc = 1;
		(yy_buffer_stack) = (struct yy_buffer_state**)yyalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		
		int grow_size = 8 ;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)yyrealloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );

		
		memset((yy_buffer_stack) + (yy_buffer_stack_max), 0, grow_size * sizeof(struct yy_buffer_state*));
		(yy_buffer_stack_max) = num_to_alloc;
	}
}







YY_BUFFER_STATE yy_scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		
		return 0;

	b = (YY_BUFFER_STATE) yyalloc(sizeof( struct yy_buffer_state )  );
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

	yy_switch_to_buffer(b  );

	return b;
}









YY_BUFFER_STATE yy_scan_string (yyconst char * yystr )
{
    
	return yy_scan_bytes(yystr,strlen(yystr) );
}








YY_BUFFER_STATE yy_scan_bytes  (yyconst char * yybytes, int  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	
	n = _yybytes_len + 2;
	buf = (char *) yyalloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = yy_scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in yy_scan_bytes()" );

	


	b->yy_is_our_buffer = 1;

	return b;
}

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yy_fatal_error (yyconst char* msg )
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
		yytext[yyleng] = (yy_hold_char); \
		(yy_c_buf_p) = yytext + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		yyleng = yyless_macro_arg; \
		} \
	while ( 0 )






int yyget_lineno  (void)
{
        
    return yylineno;
}




FILE *yyget_in  (void)
{
        return yyin;
}




FILE *yyget_out  (void)
{
        return yyout;
}




int yyget_leng  (void)
{
        return yyleng;
}





char *yyget_text  (void)
{
        return yytext;
}





void yyset_lineno (int  line_number )
{
    
    yylineno = line_number;
}







void yyset_in (FILE *  in_str )
{
        yyin = in_str ;
}

void yyset_out (FILE *  out_str )
{
        yyout = out_str ;
}

int yyget_debug  (void)
{
        return yy_flex_debug;
}

void yyset_debug (int  bdebug )
{
        yy_flex_debug = bdebug ;
}

static int yy_init_globals (void)
{
        



    
    yylineno =  1;
    
    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;


#ifdef YY_STDINIT
    yyin = stdin;
    yyout = stdout;
#else
    yyin = (FILE *) 0;
    yyout = (FILE *) 0;
#endif

    


    return 0;
}


int yylex_destroy  (void)
{
    
    
	while(YY_CURRENT_BUFFER){
		yy_delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		yypop_buffer_state();
	}

	
	yyfree((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    

    yy_init_globals( );

    return 0;
}





#ifndef yytext_ptr
static void yy_flex_strncpy (char* s1, yyconst char * s2, int n )
{
	register int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
}
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * s )
{
	register int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
}
#endif

void *yyalloc (yy_size_t  size )
{
	return (void *) malloc( size );
}

void *yyrealloc  (void * ptr, yy_size_t  size )
{
	






	return (void *) realloc( (char *) ptr, size );
}

void yyfree (void * ptr )
{
	free( (char *) ptr );	
}

#define YYTABLES_NAME "yytables"


extern "C" {
  #include "compiler/preprocessor/preprocess.h"
} 






int yy_input(char* buf, int max_size)
{
    int len;

    if ((len = yylex_CPP(buf, max_size)) == 0)
        return 0;
    if (len >= max_size) 
        YY_FATAL_ERROR( "input buffer overflow, can't enlarge buffer because scanner uses REJECT" );

    buf[len] = ' ';
	return len+1;
}







int PaParseStrings(char* argv[], int strLen[], int argc, TParseContext& parseContextLocal)
{
    int argv0len;
    
    ScanFromString(argv[0]); 
    
    
	cpp->pC = (void*)&parseContextLocal;
	
	if (!argv || argc == 0)
        return 1;
    
    for (int i = 0; i < argc; ++i) {
        if (!argv[i]) {
            parseContextLocal.error(0, "Null shader source string", "", "");
            parseContextLocal.recover();
            return 1;
        }
    }
    
    if (!strLen) {
        argv0len = (int) strlen(argv[0]);
        strLen   = &argv0len;
    }
    yyrestart(0);
    (&parseContextLocal)->AfterEOF = false;
    cpp->PaWhichStr = 0;
    cpp->PaArgv     = argv;
    cpp->PaArgc     = argc;
    cpp->PaStrLen   = strLen;
    cpp->pastFirstStatement = 0;
    yylineno   = 1;
   
    if (*cpp->PaStrLen >= 0) {    
        int ret = yyparse((void*)(&parseContextLocal));
        if (ret || cpp->CompileError == 1 || parseContextLocal.recoveredFromError || parseContextLocal.numErrors > 0)
             return 1;
        else
             return 0;
    }
    else
        return 0;
}

void yyerror(const char *s) 
{
    if (((TParseContext *)cpp->pC)->AfterEOF) {
        if (cpp->tokensBeforeEOF == 1) {
            GlobalParseContext->error(yylineno, "syntax error", "pre-mature EOF", s, "");
            GlobalParseContext->recover();
        }
    } else {
        GlobalParseContext->error(yylineno, "syntax error", yytext, s, "");
        GlobalParseContext->recover();
    }            
}

void PaReservedWord()
{
    GlobalParseContext->error(yylineno, "Reserved word.", yytext, "", "");
    GlobalParseContext->recover();
}

int PaIdentOrType(TString& id, TParseContext& parseContextLocal, TSymbol*& symbol)
{
    symbol = parseContextLocal.symbolTable.find(id);
    if (parseContextLocal.lexAfterType == false && symbol && symbol->isVariable()) {
        TVariable* variable = static_cast<TVariable*>(symbol);
        if (variable->isUserType()) {
            parseContextLocal.lexAfterType = true;
            return TYPE_NAME;
        }
    }
    
    return IDENTIFIER;
}

int PaParseComment(int &lineno, TParseContext& parseContextLocal)
{
    int transitionFlag = 0;
    int nextChar;
    
    while (transitionFlag != 2) {
        nextChar = yyinput();
        if (nextChar == '\n')
             lineno++;
        switch (nextChar) {
        case '*' :
            transitionFlag = 1;
            break;
        case '/' :  
            if (transitionFlag == 1) {
                return 1 ;
            }
            break;
        case EOF :
            
            parseContextLocal.error(yylineno, "End of shader found before end of comment.", "", "", "");
            GlobalParseContext->recover();
            return YY_NULL; 
        default :  
            transitionFlag = 0;
        }
    }
    return 1;
}

extern "C" {

void CPPDebugLogMsg(const char *msg)
{
    ((TParseContext *)cpp->pC)->infoSink.debug.message(EPrefixNone, msg);
}

void CPPWarningToInfoLog(const char *msg)
{
    ((TParseContext *)cpp->pC)->infoSink.info.message(EPrefixWarning, msg, yylineno); 
}

void CPPShInfoLogMsg(const char *msg)
{
    ((TParseContext *)cpp->pC)->error(yylineno,"", "",msg,"");
    GlobalParseContext->recover();
}

void CPPErrorToInfoLog(char *msg)
{
    ((TParseContext *)cpp->pC)->error(yylineno,"syntax error", "",msg,"");
    GlobalParseContext->recover();
}

void SetLineNumber(int line)
{
    yylineno &= ~SourceLocLineMask;
    yylineno |= line;
}

void SetStringNumber(int string)
{
    yylineno = (string << SourceLocStringShift) | (yylineno & SourceLocLineMask);
}

int GetStringNumber(void)
{
    return yylineno >> 16;
}

int GetLineNumber(void)
{
    return yylineno & SourceLocLineMask;
}

void IncLineNumber(void)
{
    if ((yylineno & SourceLocLineMask) <= SourceLocLineMask)
        ++yylineno;
}

void DecLineNumber(void)
{
    if ((yylineno & SourceLocLineMask) > 0)
        --yylineno;
}

void HandlePragma(const char **tokens, int numTokens)
{    
    if (!strcmp(tokens[0], "optimize")) {
        if (numTokens != 4) {
            CPPShInfoLogMsg("optimize pragma syntax is incorrect");
            return;
        }
        
        if (strcmp(tokens[1], "(")) {
            CPPShInfoLogMsg("\"(\" expected after 'optimize' keyword");
            return;
        }
            
        if (!strcmp(tokens[2], "on"))
            ((TParseContext *)cpp->pC)->contextPragma.optimize = true;
        else if (!strcmp(tokens[2], "off"))
            ((TParseContext *)cpp->pC)->contextPragma.optimize = false;
        else {
            CPPShInfoLogMsg("\"on\" or \"off\" expected after '(' for 'optimize' pragma");
            return;
        }
        
        if (strcmp(tokens[3], ")")) {
            CPPShInfoLogMsg("\")\" expected to end 'optimize' pragma");
            return;
        }
    } else if (!strcmp(tokens[0], "debug")) {
        if (numTokens != 4) {
            CPPShInfoLogMsg("debug pragma syntax is incorrect");
            return;
        }
        
        if (strcmp(tokens[1], "(")) {
            CPPShInfoLogMsg("\"(\" expected after 'debug' keyword");
            return;
        }
            
        if (!strcmp(tokens[2], "on"))
            ((TParseContext *)cpp->pC)->contextPragma.debug = true;
        else if (!strcmp(tokens[2], "off"))
            ((TParseContext *)cpp->pC)->contextPragma.debug = false;
        else {
            CPPShInfoLogMsg("\"on\" or \"off\" expected after '(' for 'debug' pragma");
            return;
        }
        
        if (strcmp(tokens[3], ")")) {
            CPPShInfoLogMsg("\")\" expected to end 'debug' pragma");
            return;
        }
    } else {

#ifdef PRAGMA_TABLE
        
        
        
        
        
        
        
        
        if (numTokens == 4 && !strcmp(tokens[1], "(") && !strcmp(tokens[3], ")")) {              
            TPragmaTable& pragmaTable = ((TParseContext *)cpp->pC)->contextPragma.pragmaTable;
            TPragmaTable::iterator iter;
            iter = pragmaTable.find(TString(tokens[0]));
            if (iter != pragmaTable.end()) {
                iter->second = tokens[2];
            } else {
                pragmaTable[ tokens[0] ] = tokens[2];
            }        
        } else if (numTokens >= 2) {
            TPragmaTable& pragmaTable = ((TParseContext *)cpp->pC)->contextPragma.pragmaTable;
            TPragmaTable::iterator iter;
            iter = pragmaTable.find(TString(tokens[0]));
            if (iter != pragmaTable.end()) {
                iter->second = tokens[1];
            } else {
                pragmaTable[ tokens[0] ] = tokens[1];
            }
        }
#endif 
    }
}

void StoreStr(char *string)
{
    TString strSrc;
    strSrc = TString(string);

    ((TParseContext *)cpp->pC)->HashErrMsg = ((TParseContext *)cpp->pC)->HashErrMsg + " " + strSrc;
}

const char* GetStrfromTStr(void)
{
    cpp->ErrMsg = (((TParseContext *)cpp->pC)->HashErrMsg).c_str();
    return cpp->ErrMsg;
}

void ResetTString(void)
{
    ((TParseContext *)cpp->pC)->HashErrMsg = "";
}

TBehavior GetBehavior(const char* behavior)
{
    if (!strcmp("require", behavior))
        return EBhRequire;
    else if (!strcmp("enable", behavior))
        return EBhEnable;
    else if (!strcmp("disable", behavior))
        return EBhDisable;
    else if (!strcmp("warn", behavior))
        return EBhWarn;
    else {
        CPPShInfoLogMsg((TString("behavior '") + behavior + "' is not supported").c_str());
        return EBhDisable;
    }        
}

void  updateExtensionBehavior(const char* extName, const char* behavior)
{
    TBehavior behaviorVal = GetBehavior(behavior);
    TMap<TString, TBehavior>:: iterator iter;
    TString msg;
    
    
    if (!strcmp(extName, "all")) {
        if (behaviorVal == EBhRequire || behaviorVal == EBhEnable) {
            CPPShInfoLogMsg("extension 'all' cannot have 'require' or 'enable' behavior");  
            return;
        } else {
            for (iter =  ((TParseContext *)cpp->pC)->extensionBehavior.begin(); iter != ((TParseContext *)cpp->pC)->extensionBehavior.end(); ++iter)
                iter->second = behaviorVal;
        }        
    } else {
        iter = ((TParseContext *)cpp->pC)->extensionBehavior.find(TString(extName));
        if (iter == ((TParseContext *)cpp->pC)->extensionBehavior.end()) {
            switch (behaviorVal) {
            case EBhRequire:
                CPPShInfoLogMsg((TString("extension '") + extName + "' is not supported").c_str());  
                break;
            case EBhEnable:
            case EBhWarn:
            case EBhDisable:
                msg = TString("extension '") + extName + "' is not supported";
                ((TParseContext *)cpp->pC)->infoSink.info.message(EPrefixWarning, msg.c_str(), yylineno); 
                break;
            }
            return;
        } else
            iter->second = behaviorVal;
    }
}
        
}  

void setInitialState()
{
    yy_start = 1;
}

