





#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 5

#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif


#ifdef c_plusplus
#ifndef __cplusplus
#define __cplusplus
#endif
#endif

#ifdef __cplusplus

#include <stdlib.h>


#define YY_USE_PROTOS


#define YY_USE_CONST

#else	

#if __STDC__

#define YY_USE_PROTOS
#define YY_USE_CONST

#endif	
#endif	

#ifdef __TURBOC__
 #pragma warn -rch
 #pragma warn -use
#include <io.h>
#include <stdlib.h>
#define YY_USE_CONST
#define YY_USE_PROTOS
#endif

#ifdef YY_USE_CONST
#define yyconst const
#else
#define yyconst
#endif


#ifdef YY_USE_PROTOS
#define YY_PROTO(proto) proto
#else
#define YY_PROTO(proto) ()
#endif


#define YY_NULL 0






#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)





#define BEGIN yy_start = 1 + 2 *





#define YY_START ((yy_start - 1) / 2)
#define YYSTATE YY_START


#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)


#define YY_NEW_FILE yyrestart( yyin )

#define YY_END_OF_BUFFER_CHAR 0


#define YY_BUF_SIZE 16384

typedef struct yy_buffer_state *YY_BUFFER_STATE;

extern int yyleng;
extern FILE *yyin, *yyout;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

















#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
		*yy_cp = yy_hold_char; \
		YY_RESTORE_YY_MORE_OFFSET \
		yy_c_buf_p = yy_cp = yy_bp + n - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up yytext again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, yytext_ptr )





typedef unsigned int yy_size_t;


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

	


	int yy_fill_buffer;

	int yy_buffer_status;
#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	









#define YY_BUFFER_EOF_PENDING 2
	};

static YY_BUFFER_STATE yy_current_buffer = 0;





#define YY_CURRENT_BUFFER yy_current_buffer



static char yy_hold_char;

static int yy_n_chars;		


int yyleng;


static char *yy_c_buf_p = (char *) 0;
static int yy_init = 1;		
static int yy_start = 0;	




static int yy_did_buffer_switch_on_eof;

void yyrestart YY_PROTO(( FILE *input_file ));

void yy_switch_to_buffer YY_PROTO(( YY_BUFFER_STATE new_buffer ));
void yy_load_buffer_state YY_PROTO(( void ));
YY_BUFFER_STATE yy_create_buffer YY_PROTO(( FILE *file, int size ));
void yy_delete_buffer YY_PROTO(( YY_BUFFER_STATE b ));
void yy_init_buffer YY_PROTO(( YY_BUFFER_STATE b, FILE *file ));
void yy_flush_buffer YY_PROTO(( YY_BUFFER_STATE b ));
#define YY_FLUSH_BUFFER yy_flush_buffer( yy_current_buffer )

YY_BUFFER_STATE yy_scan_buffer YY_PROTO(( char *base, yy_size_t size ));
YY_BUFFER_STATE yy_scan_string YY_PROTO(( yyconst char *yy_str ));
YY_BUFFER_STATE yy_scan_bytes YY_PROTO(( yyconst char *bytes, int len ));

static void *yy_flex_alloc YY_PROTO(( yy_size_t ));
static void *yy_flex_realloc YY_PROTO(( void *, yy_size_t ));
static void yy_flex_free YY_PROTO(( void * ));

#define yy_new_buffer yy_create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! yy_current_buffer ) \
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	yy_current_buffer->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! yy_current_buffer ) \
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	yy_current_buffer->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (yy_current_buffer->yy_at_bol)

typedef unsigned char YY_CHAR;
FILE *yyin = (FILE *) 0, *yyout = (FILE *) 0;
typedef int yy_state_type;
extern char *yytext;
#define yytext_ptr yytext

static yy_state_type yy_get_previous_state YY_PROTO(( void ));
static yy_state_type yy_try_NUL_trans YY_PROTO(( yy_state_type current_state ));
static int yy_get_next_buffer YY_PROTO(( void ));
static void yy_fatal_error YY_PROTO(( yyconst char msg[] ));




#define YY_DO_BEFORE_ACTION \
	yytext_ptr = yy_bp; \
	yytext_ptr -= yy_more_len; \
	yyleng = (int) (yy_cp - yytext_ptr); \
	yy_hold_char = *yy_cp; \
	*yy_cp = '\0'; \
	yy_c_buf_p = yy_cp;

#define YY_NUM_RULES 17
#define YY_END_OF_BUFFER 18
static yyconst short int yy_accept[67] =
    {   0,
        0,    0,   18,   16,   14,   15,   16,   11,   12,    2,
       10,    9,    9,    9,    9,    9,   13,   14,   15,   11,
       12,    0,   12,    2,    9,    9,    9,    9,    9,   13,
        3,    4,    2,    9,    9,    9,    9,    2,    9,    9,
        9,    9,    2,    2,    9,    9,    8,    9,    2,    5,
        9,    6,    2,    9,    2,    9,    2,    9,    2,    7,
        2,    2,    2,    2,    1,    0
    } ;

static yyconst int yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    4,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    5,    6,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    7,    8,    1,    9,    9,   10,
       11,   12,   12,   12,   13,   13,   13,   14,    1,    1,
       15,    1,    1,    1,   16,   16,   16,   16,   16,   16,
       16,   16,   16,   16,   16,   16,   16,   16,   16,   16,
       16,   16,   16,   16,   16,   16,   16,   16,   16,   17,
        1,    1,    1,    1,    1,    1,   18,   16,   16,   19,

       20,   16,   21,   16,   22,   16,   16,   16,   16,   23,
       16,   24,   16,   25,   26,   27,   28,   16,   16,   29,
       16,   16,    1,   14,    1,    1,    1,    1,    1,    1,
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

static yyconst int yy_meta[30] =
    {   0,
        1,    1,    2,    1,    3,    1,    1,    4,    5,    5,
        5,    5,    5,    4,    1,    4,    4,    4,    4,    4,
        4,    4,    4,    4,    4,    4,    4,    4,    4
    } ;

static yyconst short int yy_base[72] =
    {   0,
        0,  149,  154,  205,  138,  205,  103,    0,    0,   23,
      205,   29,   30,   31,   32,   33,    0,   99,  205,    0,
        0,    0,   50,   55,   34,   61,   41,   63,   64,    0,
        0,    0,   79,   65,   68,   86,   66,   99,  105,   88,
      106,   90,  118,   76,  107,  110,   89,  125,   43,   91,
      127,  128,  138,  144,  113,  129,  154,  160,  160,  130,
      172,  166,  177,  144,    0,  205,  190,  192,  194,  199,
       76
    } ;

static yyconst short int yy_def[72] =
    {   0,
       66,    1,   66,   66,   66,   66,   66,   67,   68,   68,
       66,   69,   69,   69,   69,   69,   70,   66,   66,   67,
       68,   71,   68,   10,   69,   69,   69,   69,   69,   70,
       71,   23,   10,   69,   69,   69,   69,   10,   69,   69,
       69,   69,   10,   38,   69,   69,   69,   69,   38,   69,
       69,   69,   38,   69,   38,   69,   38,   69,   38,   69,
       38,   38,   38,   38,   68,    0,   66,   66,   66,   66,
       66
    } ;

static yyconst short int yy_nxt[235] =
    {   0,
        4,    5,    6,    7,    8,    4,    4,    9,   10,   10,
       10,   10,   10,    9,   11,   12,   12,   12,   12,   12,
       12,   13,   14,   12,   15,   12,   12,   16,   12,   22,
       23,   24,   24,   24,   24,   24,   21,   21,   21,   21,
       21,   21,   21,   21,   21,   21,   21,   21,   21,   28,
       27,   53,   53,   53,   21,   26,   29,   32,   32,   32,
       32,   32,   32,   33,   33,   33,   33,   33,   21,   35,
       21,   21,   21,   21,   21,   21,   21,   21,   21,   21,
       31,   21,   37,   42,   44,   36,   34,   38,   38,   38,
       38,   38,   39,   21,   40,   21,   21,   21,   21,   21,

       18,   21,   21,   21,   21,   19,   41,   43,   44,   44,
       44,   44,   21,   21,   21,   46,   48,   21,   21,   21,
       21,   57,   57,   21,   45,   47,   49,   49,   49,   49,
       49,   50,   21,   51,   21,   21,   21,   21,   21,   18,
       21,   21,   21,   21,   52,   54,   55,   55,   55,   55,
       55,   21,   44,   66,   17,   58,   66,   21,   66,   66,
       65,   56,   59,   59,   59,   59,   59,   21,   61,   61,
       61,   61,   66,   21,   63,   63,   63,   63,   66,   60,
       62,   62,   62,   62,   62,   64,   64,   64,   64,   64,
       20,   20,   66,   20,   20,   21,   21,   25,   25,   30,

       66,   30,   30,   30,    3,   66,   66,   66,   66,   66,
       66,   66,   66,   66,   66,   66,   66,   66,   66,   66,
       66,   66,   66,   66,   66,   66,   66,   66,   66,   66,
       66,   66,   66,   66
    } ;

static yyconst short int yy_chk[235] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,   10,
       10,   10,   10,   10,   10,   10,   12,   13,   14,   15,
       16,   25,   12,   13,   14,   15,   16,   25,   27,   15,
       14,   49,   49,   49,   27,   13,   16,   23,   23,   23,
       23,   23,   23,   24,   24,   24,   24,   24,   26,   27,
       28,   29,   34,   37,   26,   35,   28,   29,   34,   37,
       71,   35,   29,   37,   44,   28,   26,   33,   33,   33,
       33,   33,   34,   36,   35,   40,   47,   42,   50,   36,

       18,   40,   47,   42,   50,    7,   36,   38,   38,   38,
       38,   38,   39,   41,   45,   40,   42,   46,   39,   41,
       45,   55,   55,   46,   39,   41,   43,   43,   43,   43,
       43,   45,   48,   46,   51,   52,   56,   60,   48,    5,
       51,   52,   56,   60,   48,   51,   53,   53,   53,   53,
       53,   54,   64,    3,    2,   56,    0,   54,    0,    0,
       64,   54,   57,   57,   57,   57,   57,   58,   59,   59,
       59,   59,    0,   58,   62,   62,   62,   62,    0,   58,
       61,   61,   61,   61,   61,   63,   63,   63,   63,   63,
       67,   67,    0,   67,   67,   68,   68,   69,   69,   70,

        0,   70,   70,   70,   66,   66,   66,   66,   66,   66,
       66,   66,   66,   66,   66,   66,   66,   66,   66,   66,
       66,   66,   66,   66,   66,   66,   66,   66,   66,   66,
       66,   66,   66,   66
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;




#define REJECT reject_used_but_not_detected
static int yy_more_flag = 0;
static int yy_more_len = 0;
#define yymore() (yy_more_flag = 1)
#define YY_MORE_ADJ yy_more_len
#define YY_RESTORE_YY_MORE_OFFSET
char *yytext;
#line 1 "crlgen_lex_orig.l"
#define INITIAL 0
#line 2 "crlgen_lex_orig.l"

#include "crlgen.h"

static SECStatus parserStatus = SECSuccess;
static CRLGENGeneratorData *parserData;
static PRFileDesc *src;

#define YY_INPUT(buf,result,max_size) \
    if ( parserStatus != SECFailure) { \
	 if (((result = PR_Read(src, buf, max_size)) == 0) && \
	     ferror( yyin )) \
	   return SECFailure; \
    } else  { return SECFailure; }
              






#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int yywrap YY_PROTO(( void ));
#else
extern int yywrap YY_PROTO(( void ));
#endif
#endif

#ifndef YY_NO_UNPUT
static void yyunput YY_PROTO(( int c, char *buf_ptr ));
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy YY_PROTO(( char *, yyconst char *, int ));
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen YY_PROTO(( yyconst char * ));
#endif

#ifndef YY_NO_INPUT
#ifdef __cplusplus
static int yyinput YY_PROTO(( void ));
#else
static int input YY_PROTO(( void ));
#endif
#endif

#if YY_STACK_USED
static int yy_start_stack_ptr = 0;
static int yy_start_stack_depth = 0;
static int *yy_start_stack = 0;
#ifndef YY_NO_PUSH_STATE
static void yy_push_state YY_PROTO(( int new_state ));
#endif
#ifndef YY_NO_POP_STATE
static void yy_pop_state YY_PROTO(( void ));
#endif
#ifndef YY_NO_TOP_STATE
static int yy_top_state YY_PROTO(( void ));
#endif

#else
#define YY_NO_PUSH_STATE 1
#define YY_NO_POP_STATE 1
#define YY_NO_TOP_STATE 1
#endif

#ifdef YY_MALLOC_DECL
YY_MALLOC_DECL
#else
#if __STDC__
#ifndef __cplusplus
#include <stdlib.h>
#endif
#else




#endif
#endif


#ifndef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE 8192
#endif



#ifndef ECHO



#define ECHO (void) fwrite( yytext, yyleng, 1, yyout )
#endif




#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( yy_current_buffer->yy_is_interactive ) \
		{ \
		int c = '*', n; \
		for ( n = 0; n < max_size && \
			     (c = getc( yyin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( yyin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else if ( ((result = fread( buf, 1, max_size, yyin )) == 0) \
		  && ferror( yyin ) ) \
		YY_FATAL_ERROR( "input in flex scanner failed" );
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
#define YY_DECL int yylex YY_PROTO(( void ))
#endif




#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif


#ifndef YY_BREAK
#define YY_BREAK break;
#endif

#define YY_RULE_SETUP \
	if ( yyleng > 0 ) \
		yy_current_buffer->yy_at_bol = \
				(yytext[yyleng - 1] == '\n'); \
	YY_USER_ACTION

YY_DECL
	{
	register yy_state_type yy_current_state;
	register char *yy_cp = NULL, *yy_bp = NULL;
	register int yy_act;

#line 28 "crlgen_lex_orig.l"



	if ( yy_init )
		{
		yy_init = 0;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! yy_start )
			yy_start = 1;	

		if ( ! yyin )
			yyin = stdin;

		if ( ! yyout )
			yyout = stdout;

		if ( ! yy_current_buffer )
			yy_current_buffer =
				yy_create_buffer( yyin, YY_BUF_SIZE );

		yy_load_buffer_state();
		}

	while ( 1 )		
		{
		yy_more_len = 0;
		if ( yy_more_flag )
			{
			yy_more_len = yy_c_buf_p - yytext_ptr;
			yy_more_flag = 0;
			}
		yy_cp = yy_c_buf_p;

		
		*yy_cp = yy_hold_char;

		


		yy_bp = yy_cp;

		yy_current_state = yy_start;
		yy_current_state += YY_AT_BOL();
yy_match:
		do
			{
			register YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
			if ( yy_accept[yy_current_state] )
				{
				yy_last_accepting_state = yy_current_state;
				yy_last_accepting_cpos = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 67 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 205 );

yy_find_action:
		yy_act = yy_accept[yy_current_state];
		if ( yy_act == 0 )
			{ 
			yy_cp = yy_last_accepting_cpos;
			yy_current_state = yy_last_accepting_state;
			yy_act = yy_accept[yy_current_state];
			}

		YY_DO_BEFORE_ACTION;


do_action:	


		switch ( yy_act )
	{ 
			case 0: 
			
			*yy_cp = yy_hold_char;
			yy_cp = yy_last_accepting_cpos;
			yy_current_state = yy_last_accepting_state;
			goto yy_find_action;

case 1:
YY_RULE_SETUP
#line 30 "crlgen_lex_orig.l"
{
parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_ZDATE);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 2:
YY_RULE_SETUP
#line 36 "crlgen_lex_orig.l"
{
parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_DIGIT);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 3:
YY_RULE_SETUP
#line 42 "crlgen_lex_orig.l"
{
parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_DIGIT_RANGE);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 4:
YY_RULE_SETUP
#line 48 "crlgen_lex_orig.l"
{
parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_OID);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 5:
YY_RULE_SETUP
#line 54 "crlgen_lex_orig.l"
{
parserStatus = crlgen_createNewLangStruct(parserData, CRLGEN_ISSUER_CONTEXT);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 6:
YY_RULE_SETUP
#line 60 "crlgen_lex_orig.l"
{
parserStatus = crlgen_createNewLangStruct(parserData, CRLGEN_UPDATE_CONTEXT);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 7:
YY_RULE_SETUP
#line 65 "crlgen_lex_orig.l"
{
parserStatus = crlgen_createNewLangStruct(parserData, CRLGEN_NEXT_UPDATE_CONTEXT);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 8:
YY_RULE_SETUP
#line 71 "crlgen_lex_orig.l"
{
parserStatus = crlgen_createNewLangStruct(parserData, CRLGEN_CHANGE_RANGE_CONTEXT);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 9:
YY_RULE_SETUP
#line 77 "crlgen_lex_orig.l"
{
if (strcmp(yytext, "addcert") == 0) {
    parserStatus = crlgen_createNewLangStruct(parserData,
                                    CRLGEN_ADD_CERT_CONTEXT);
    if (parserStatus != SECSuccess)
        return parserStatus;
} else if (strcmp(yytext, "rmcert") == 0) {
    parserStatus = crlgen_createNewLangStruct(parserData,
                                    CRLGEN_RM_CERT_CONTEXT);
    if (parserStatus != SECSuccess)
        return parserStatus;
} else if (strcmp(yytext, "addext") == 0) {
    parserStatus = crlgen_createNewLangStruct(parserData,
                                    CRLGEN_ADD_EXTENSION_CONTEXT);
    if (parserStatus != SECSuccess)
        return parserStatus;
} else {
    parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_ID);
    if (parserStatus != SECSuccess)
        return parserStatus;
}
}
	YY_BREAK
case 10:
YY_RULE_SETUP
#line 100 "crlgen_lex_orig.l"

	YY_BREAK
case 11:
YY_RULE_SETUP
#line 102 "crlgen_lex_orig.l"
{
if (yytext[yyleng-1] == '\\') {
    yymore();
} else {
    register int c;
    c = input();
    if (c != '\"') {
        printf( "Error: Line ending \" is missing:  %c\n", c);
        unput(c);
    } else {
        parserStatus = crlgen_setNextData(parserData, yytext + 1,
                                          CRLGEN_TYPE_STRING);
        if (parserStatus != SECSuccess)
            return parserStatus;
    }
}
}
	YY_BREAK
case 12:
YY_RULE_SETUP
#line 120 "crlgen_lex_orig.l"
{
parserStatus = crlgen_setNextData(parserData, yytext, CRLGEN_TYPE_STRING);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 13:
YY_RULE_SETUP
#line 128 "crlgen_lex_orig.l"
 {}
	YY_BREAK
case 14:
YY_RULE_SETUP
#line 130 "crlgen_lex_orig.l"
{}
	YY_BREAK
case 15:
YY_RULE_SETUP
#line 132 "crlgen_lex_orig.l"
{
parserStatus = crlgen_updateCrl(parserData);
if (parserStatus != SECSuccess)
    return parserStatus;
}
	YY_BREAK
case 16:
YY_RULE_SETUP
#line 138 "crlgen_lex_orig.l"
{
    fprintf(stderr, "Syntax error at line %d: unknown token %s\n",
            parserData->parsedLineNum, yytext);
    return SECFailure;
}
	YY_BREAK
case 17:
YY_RULE_SETUP
#line 144 "crlgen_lex_orig.l"
ECHO;
	YY_BREAK
case YY_STATE_EOF(INITIAL):
	yyterminate();

	case YY_END_OF_BUFFER:
		{
		
		int yy_amount_of_matched_text = (int) (yy_cp - yytext_ptr) - 1;

		
		*yy_cp = yy_hold_char;
		YY_RESTORE_YY_MORE_OFFSET

		if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_NEW )
			{
			








			yy_n_chars = yy_current_buffer->yy_n_chars;
			yy_current_buffer->yy_input_file = yyin;
			yy_current_buffer->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		






		if ( yy_c_buf_p <= &yy_current_buffer->yy_ch_buf[yy_n_chars] )
			{ 
			yy_state_type yy_next_state;

			yy_c_buf_p = yytext_ptr + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state();

			








			yy_next_state = yy_try_NUL_trans( yy_current_state );

			yy_bp = yytext_ptr + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				
				yy_cp = ++yy_c_buf_p;
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = yy_c_buf_p;
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer() )
			{
			case EOB_ACT_END_OF_FILE:
				{
				yy_did_buffer_switch_on_eof = 0;

				if ( yywrap() )
					{
					








					yy_c_buf_p = yytext_ptr + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				yy_c_buf_p =
					yytext_ptr + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state();

				yy_cp = yy_c_buf_p;
				yy_bp = yytext_ptr + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				yy_c_buf_p =
				&yy_current_buffer->yy_ch_buf[yy_n_chars];

				yy_current_state = yy_get_previous_state();

				yy_cp = yy_c_buf_p;
				yy_bp = yytext_ptr + YY_MORE_ADJ;
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










static int yy_get_next_buffer()
	{
	register char *dest = yy_current_buffer->yy_ch_buf;
	register char *source = yytext_ptr;
	register int number_to_move, i;
	int ret_val;

	if ( yy_c_buf_p > &yy_current_buffer->yy_ch_buf[yy_n_chars + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( yy_current_buffer->yy_fill_buffer == 0 )
		{ 
		if ( yy_c_buf_p - yytext_ptr - YY_MORE_ADJ == 1 )
			{
			


			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			


			return EOB_ACT_LAST_MATCH;
			}
		}

	

	
	number_to_move = (int) (yy_c_buf_p - yytext_ptr) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( yy_current_buffer->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		


		yy_current_buffer->yy_n_chars = yy_n_chars = 0;

	else
		{
		int num_to_read =
			yy_current_buffer->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ 
#ifdef YY_USES_REJECT
			YY_FATAL_ERROR(
"input buffer overflow, can't enlarge buffer because scanner uses REJECT" );
#else

			
			YY_BUFFER_STATE b = yy_current_buffer;

			int yy_c_buf_p_offset =
				(int) (yy_c_buf_p - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					
					yy_flex_realloc( (void *) b->yy_ch_buf,
							 b->yy_buf_size + 2 );
				}
			else
				
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			yy_c_buf_p = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = yy_current_buffer->yy_buf_size -
						number_to_move - 1;
#endif
			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		
		YY_INPUT( (&yy_current_buffer->yy_ch_buf[number_to_move]),
			yy_n_chars, num_to_read );

		yy_current_buffer->yy_n_chars = yy_n_chars;
		}

	if ( yy_n_chars == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			yyrestart( yyin );
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			yy_current_buffer->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	yy_n_chars += number_to_move;
	yy_current_buffer->yy_ch_buf[yy_n_chars] = YY_END_OF_BUFFER_CHAR;
	yy_current_buffer->yy_ch_buf[yy_n_chars + 1] = YY_END_OF_BUFFER_CHAR;

	yytext_ptr = &yy_current_buffer->yy_ch_buf[0];

	return ret_val;
	}




static yy_state_type yy_get_previous_state()
	{
	register yy_state_type yy_current_state;
	register char *yy_cp;

	yy_current_state = yy_start;
	yy_current_state += YY_AT_BOL();

	for ( yy_cp = yytext_ptr + YY_MORE_ADJ; yy_cp < yy_c_buf_p; ++yy_cp )
		{
		register YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			yy_last_accepting_state = yy_current_state;
			yy_last_accepting_cpos = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 67 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
	}








#ifdef YY_USE_PROTOS
static yy_state_type yy_try_NUL_trans( yy_state_type yy_current_state )
#else
static yy_state_type yy_try_NUL_trans( yy_current_state )
yy_state_type yy_current_state;
#endif
	{
	register int yy_is_jam;
	register char *yy_cp = yy_c_buf_p;

	register YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		yy_last_accepting_state = yy_current_state;
		yy_last_accepting_cpos = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 67 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 66);

	return yy_is_jam ? 0 : yy_current_state;
	}


#ifndef YY_NO_UNPUT
#ifdef YY_USE_PROTOS
static void yyunput( int c, register char *yy_bp )
#else
static void yyunput( c, yy_bp )
int c;
register char *yy_bp;
#endif
	{
	register char *yy_cp = yy_c_buf_p;

	
	*yy_cp = yy_hold_char;

	if ( yy_cp < yy_current_buffer->yy_ch_buf + 2 )
		{ 
		
		register int number_to_move = yy_n_chars + 2;
		register char *dest = &yy_current_buffer->yy_ch_buf[
					yy_current_buffer->yy_buf_size + 2];
		register char *source =
				&yy_current_buffer->yy_ch_buf[number_to_move];

		while ( source > yy_current_buffer->yy_ch_buf )
			*--dest = *--source;

		yy_cp += (int) (dest - source);
		yy_bp += (int) (dest - source);
		yy_current_buffer->yy_n_chars =
			yy_n_chars = yy_current_buffer->yy_buf_size;

		if ( yy_cp < yy_current_buffer->yy_ch_buf + 2 )
			YY_FATAL_ERROR( "flex scanner push-back overflow" );
		}

	*--yy_cp = (char) c;


	yytext_ptr = yy_bp;
	yy_hold_char = *yy_cp;
	yy_c_buf_p = yy_cp;
	}
#endif	


#ifndef YY_NO_INPUT
#ifdef __cplusplus
static int yyinput()
#else
static int input()
#endif
	{
	int c;

	*yy_c_buf_p = yy_hold_char;

	if ( *yy_c_buf_p == YY_END_OF_BUFFER_CHAR )
		{
		



		if ( yy_c_buf_p < &yy_current_buffer->yy_ch_buf[yy_n_chars] )
			
			*yy_c_buf_p = '\0';

		else
			{ 
			int offset = yy_c_buf_p - yytext_ptr;
			++yy_c_buf_p;

			switch ( yy_get_next_buffer() )
				{
				case EOB_ACT_LAST_MATCH:
					









					
					yyrestart( yyin );

					

				case EOB_ACT_END_OF_FILE:
					{
					if ( yywrap() )
						return EOF;

					if ( ! yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput();
#else
					return input();
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					yy_c_buf_p = yytext_ptr + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) yy_c_buf_p;	
	*yy_c_buf_p = '\0';	
	yy_hold_char = *++yy_c_buf_p;

	yy_current_buffer->yy_at_bol = (c == '\n');

	return c;
	}
#endif 

#ifdef YY_USE_PROTOS
void yyrestart( FILE *input_file )
#else
void yyrestart( input_file )
FILE *input_file;
#endif
	{
	if ( ! yy_current_buffer )
		yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE );

	yy_init_buffer( yy_current_buffer, input_file );
	yy_load_buffer_state();
	}


#ifdef YY_USE_PROTOS
void yy_switch_to_buffer( YY_BUFFER_STATE new_buffer )
#else
void yy_switch_to_buffer( new_buffer )
YY_BUFFER_STATE new_buffer;
#endif
	{
	if ( yy_current_buffer == new_buffer )
		return;

	if ( yy_current_buffer )
		{
		
		*yy_c_buf_p = yy_hold_char;
		yy_current_buffer->yy_buf_pos = yy_c_buf_p;
		yy_current_buffer->yy_n_chars = yy_n_chars;
		}

	yy_current_buffer = new_buffer;
	yy_load_buffer_state();

	




	yy_did_buffer_switch_on_eof = 1;
	}


#ifdef YY_USE_PROTOS
void yy_load_buffer_state( void )
#else
void yy_load_buffer_state()
#endif
	{
	yy_n_chars = yy_current_buffer->yy_n_chars;
	yytext_ptr = yy_c_buf_p = yy_current_buffer->yy_buf_pos;
	yyin = yy_current_buffer->yy_input_file;
	yy_hold_char = *yy_c_buf_p;
	}


#ifdef YY_USE_PROTOS
YY_BUFFER_STATE yy_create_buffer( FILE *file, int size )
#else
YY_BUFFER_STATE yy_create_buffer( file, size )
FILE *file;
int size;
#endif
	{
	YY_BUFFER_STATE b;

	b = (YY_BUFFER_STATE) yy_flex_alloc( sizeof( struct yy_buffer_state ) );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_buf_size = size;

	


	b->yy_ch_buf = (char *) yy_flex_alloc( b->yy_buf_size + 2 );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_is_our_buffer = 1;

	yy_init_buffer( b, file );

	return b;
	}


#ifdef YY_USE_PROTOS
void yy_delete_buffer( YY_BUFFER_STATE b )
#else
void yy_delete_buffer( b )
YY_BUFFER_STATE b;
#endif
	{
	if ( ! b )
		return;

	if ( b == yy_current_buffer )
		yy_current_buffer = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		yy_flex_free( (void *) b->yy_ch_buf );

	yy_flex_free( (void *) b );
	}



#ifdef YY_USE_PROTOS
void yy_init_buffer( YY_BUFFER_STATE b, FILE *file )
#else
void yy_init_buffer( b, file )
YY_BUFFER_STATE b;
FILE *file;
#endif


	{
	yy_flush_buffer( b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

#if YY_ALWAYS_INTERACTIVE
	b->yy_is_interactive = 1;
#else
#if YY_NEVER_INTERACTIVE
	b->yy_is_interactive = 0;
#else
	b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
#endif
#endif
	}


#ifdef YY_USE_PROTOS
void yy_flush_buffer( YY_BUFFER_STATE b )
#else
void yy_flush_buffer( b )
YY_BUFFER_STATE b;
#endif

	{
	if ( ! b )
		return;

	b->yy_n_chars = 0;

	



	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == yy_current_buffer )
		yy_load_buffer_state();
	}


#ifndef YY_NO_SCAN_BUFFER
#ifdef YY_USE_PROTOS
YY_BUFFER_STATE yy_scan_buffer( char *base, yy_size_t size )
#else
YY_BUFFER_STATE yy_scan_buffer( base, size )
char *base;
yy_size_t size;
#endif
	{
	YY_BUFFER_STATE b;

	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		
		return 0;

	b = (YY_BUFFER_STATE) yy_flex_alloc( sizeof( struct yy_buffer_state ) );
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

	yy_switch_to_buffer( b );

	return b;
	}
#endif


#ifndef YY_NO_SCAN_STRING
#ifdef YY_USE_PROTOS
YY_BUFFER_STATE yy_scan_string( yyconst char *yy_str )
#else
YY_BUFFER_STATE yy_scan_string( yy_str )
yyconst char *yy_str;
#endif
	{
	int len;
	for ( len = 0; yy_str[len]; ++len )
		;

	return yy_scan_bytes( yy_str, len );
	}
#endif


#ifndef YY_NO_SCAN_BYTES
#ifdef YY_USE_PROTOS
YY_BUFFER_STATE yy_scan_bytes( yyconst char *bytes, int len )
#else
YY_BUFFER_STATE yy_scan_bytes( bytes, len )
yyconst char *bytes;
int len;
#endif
	{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;

	
	n = len + 2;
	buf = (char *) yy_flex_alloc( n );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_bytes()" );

	for ( i = 0; i < len; ++i )
		buf[i] = bytes[i];

	buf[len] = buf[len+1] = YY_END_OF_BUFFER_CHAR;

	b = yy_scan_buffer( buf, n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in yy_scan_bytes()" );

	


	b->yy_is_our_buffer = 1;

	return b;
	}
#endif


#ifndef YY_NO_PUSH_STATE
#ifdef YY_USE_PROTOS
static void yy_push_state( int new_state )
#else
static void yy_push_state( new_state )
int new_state;
#endif
	{
	if ( yy_start_stack_ptr >= yy_start_stack_depth )
		{
		yy_size_t new_size;

		yy_start_stack_depth += YY_START_STACK_INCR;
		new_size = yy_start_stack_depth * sizeof( int );

		if ( ! yy_start_stack )
			yy_start_stack = (int *) yy_flex_alloc( new_size );

		else
			yy_start_stack = (int *) yy_flex_realloc(
					(void *) yy_start_stack, new_size );

		if ( ! yy_start_stack )
			YY_FATAL_ERROR(
			"out of memory expanding start-condition stack" );
		}

	yy_start_stack[yy_start_stack_ptr++] = YY_START;

	BEGIN(new_state);
	}
#endif


#ifndef YY_NO_POP_STATE
static void yy_pop_state()
	{
	if ( --yy_start_stack_ptr < 0 )
		YY_FATAL_ERROR( "start-condition stack underflow" );

	BEGIN(yy_start_stack[yy_start_stack_ptr]);
	}
#endif


#ifndef YY_NO_TOP_STATE
static int yy_top_state()
	{
	return yy_start_stack[yy_start_stack_ptr - 1];
	}
#endif

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

#ifdef YY_USE_PROTOS
static void yy_fatal_error( yyconst char msg[] )
#else
static void yy_fatal_error( msg )
char msg[];
#endif
	{
	(void) fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
	}





#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
		yytext[yyleng] = yy_hold_char; \
		yy_c_buf_p = yytext + n; \
		yy_hold_char = *yy_c_buf_p; \
		*yy_c_buf_p = '\0'; \
		yyleng = n; \
		} \
	while ( 0 )




#ifndef yytext_ptr
#ifdef YY_USE_PROTOS
static void yy_flex_strncpy( char *s1, yyconst char *s2, int n )
#else
static void yy_flex_strncpy( s1, s2, n )
char *s1;
yyconst char *s2;
int n;
#endif
	{
	register int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
	}
#endif

#ifdef YY_NEED_STRLEN
#ifdef YY_USE_PROTOS
static int yy_flex_strlen( yyconst char *s )
#else
static int yy_flex_strlen( s )
yyconst char *s;
#endif
	{
	register int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
	}
#endif


#ifdef YY_USE_PROTOS
static void *yy_flex_alloc( yy_size_t size )
#else
static void *yy_flex_alloc( size )
yy_size_t size;
#endif
	{
	return (void *) malloc( size );
	}

#ifdef YY_USE_PROTOS
static void *yy_flex_realloc( void *ptr, yy_size_t size )
#else
static void *yy_flex_realloc( ptr, size )
void *ptr;
yy_size_t size;
#endif
	{
	






	return (void *) realloc( (char *) ptr, size );
	}

#ifdef YY_USE_PROTOS
static void yy_flex_free( void *ptr )
#else
static void yy_flex_free( ptr )
void *ptr;
#endif
	{
	free( ptr );
	}

#if YY_MAIN
int main()
	{
	yylex();
	return 0;
	}
#endif
#line 144 "crlgen_lex_orig.l"

#include "prlock.h"

static PRLock *parserInvocationLock;

void CRLGEN_InitCrlGenParserLock()
{
    parserInvocationLock = PR_NewLock();
}

void CRLGEN_DestroyCrlGenParserLock()
{
    PR_DestroyLock(parserInvocationLock);
}


SECStatus CRLGEN_StartCrlGen(CRLGENGeneratorData *parserCtlData)
{
    SECStatus rv;

    PR_Lock(parserInvocationLock);

    parserStatus = SECSuccess;
    parserData = parserCtlData;
    src = parserCtlData->src;

    rv = yylex();

    PR_Unlock(parserInvocationLock);

    return rv;
}

int yywrap() {return 1;}
