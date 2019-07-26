


























#ifndef __INTERN_PARSER_H__
# define __INTERN_PARSER_H__
# include <webvtt/parser.h>
# include "string_internal.h"
# ifndef NDEBUG
#   define NDEBUG
# endif

# if defined(FATAL_ASSERTION)
#   undef NDEBUG
#   include <assert.h>
# else
#   if defined(BREAK_ON_ASSERTION) && !WEBVTT_OS_WIN32
static void break_on_assert();
#   endif
# endif

typedef enum
webvtt_token_t {
  BADTOKEN = -2,
  UNFINISHED = -1, 
  BOM,
  WEBVTT, 
  NOTE, 
  INTEGER, 
  NEWLINE, 
  WHITESPACE, 
  FULL_STOP, 
  POSITION, 
  ALIGN, 
  SIZE, 
  LINE, 
  VERTICAL, 
  RL, 
  LR, 
  START, 
  MIDDLE, 
  END, 
  LEFT, 
  RIGHT, 
  SEPARATOR, 
  TIMESTAMP,
  PERCENTAGE, 
  COLON, 
} webvtt_token;

typedef enum
webvtt_state_value_type_t {
  V_NONE,
  V_POINTER,
  V_INTEGER,
  V_CUE,
  V_TEXT,
  V_LNODE,
  V_INODE,
  V_TOKEN,
} webvtt_state_value_type;

typedef enum
webvtt_parse_mode_t {
  M_WEBVTT = 0,
  M_CUETEXT,
  M_SKIP_CUE,
  M_READ_LINE,
} webvtt_parse_mode;


typedef enum
webvtt_parse_state_t {
  


  T_INITIAL = 0,
  T_TAG,
  T_TAGCOMMENT,
  T_EOL,
  T_BODY,

  T_CUEREAD, 
  T_CUE, 
  T_CUEID, 
  T_CUEPARAMS, 
  T_CUETEXT, 

  T_TIMESTAMP, 

  


  T_COMMENT,

  


  T_FROM,
  T_SEP_LEFT,
  T_SEP,
  T_SEP_RIGHT,
  T_UNTIL,

  


  T_PRECUESETTING,
  T_CUESETTING,
  T_CUESETTING_DELIMITER,
  T_CUESETTING_VALUE,
  T_SKIP_SETTING 

  


} webvtt_parse_state;




typedef enum
webvtt_lexer_state_t {
  L_START = 0, L_BOM0, L_BOM1, L_WEBVTT0, L_WEBVTT1, L_WEBVTT2, L_WEBVTT3,
  L_WEBVTT4, L_DASH0, L_SEP1, L_DIGIT0, L_NEWLINE0, L_WHITESPACE, L_POSITION0,
  L_POSITION1, L_POSITION2, L_POSITION3, L_POSITION4, L_POSITION5, L_POSITION6,
  L_ALIGN0, L_ALIGN1, L_ALIGN2, L_ALIGN3, L_L0, L_LINE1, L_LINE2, L_VERTICAL0,
  L_VERTICAL1, L_VERTICAL2, L_VERTICAL3, L_VERTICAL4, L_VERTICAL5, L_VERTICAL6,
  L_RL0, L_S0, L_SIZE1, L_SIZE2, L_START1, L_START2, L_START3, L_MIDDLE0,
  L_MIDDLE1, L_MIDDLE2, L_MIDDLE3, L_MIDDLE4, L_END0, L_END1, L_TIMESTAMP1,
  L_TIMESTAMP2, L_TIMESTAMP3, L_RIGHT1, L_RIGHT2, L_RIGHT3, L_NOTE1, L_NOTE2,
  L_NOTE3, L_LEFT1, L_LEFT2,
} webvtt_lexer_state;

typedef struct
webvtt_state {
  webvtt_parse_state state;
  webvtt_uint flags; 
  webvtt_token token;
  webvtt_state_value_type type;
  webvtt_uint back;
  webvtt_uint line;
  webvtt_uint column;
  union {
    


    webvtt_cue *cue;

    


    webvtt_string text;

    






    webvtt_node *node;

    


    webvtt_uint value;
  } v;
} webvtt_state;

struct
webvtt_parser_t {
  webvtt_uint state;
  webvtt_uint bytes; 
  webvtt_uint line;
  webvtt_uint column;
  webvtt_cue_fn read;
  webvtt_error_fn error;
  void *userdata;
  webvtt_bool finished;
  
  webvtt_uint cuetext_line; 

  


  webvtt_parse_mode mode;

  webvtt_state *top; 
  webvtt_state astack[0x100];
  webvtt_state *stack; 
  webvtt_uint stack_alloc; 
  webvtt_bool popped;

  


  int truncate;
  webvtt_uint line_pos;
  webvtt_string line_buffer;

  


  webvtt_lexer_state tstate;
  webvtt_uint token_pos;
  webvtt_byte token[0x100];
};

WEBVTT_INTERN webvtt_token webvtt_lex( webvtt_parser self, const webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint length, webvtt_bool finish );
WEBVTT_INTERN webvtt_status webvtt_lex_word( webvtt_parser self, webvtt_string *pba, const webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint length, webvtt_bool finish );



WEBVTT_INTERN webvtt_token webvtt_lex_newline( webvtt_parser self, const
  webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint length, webvtt_bool finish );

WEBVTT_INTERN webvtt_status webvtt_proc_cueline( webvtt_parser self,
  webvtt_cue *cue, webvtt_string *line );

WEBVTT_INTERN webvtt_status webvtt_parse_align( webvtt_parser self,
  webvtt_cue *cue, const webvtt_byte *text, webvtt_uint *pos, webvtt_uint len );

WEBVTT_INTERN webvtt_status webvtt_parse_line( webvtt_parser self,
  webvtt_cue *cue, const webvtt_byte *text, webvtt_uint *pos, webvtt_uint len );

WEBVTT_INTERN webvtt_status webvtt_parse_position( webvtt_parser self,
  webvtt_cue *cue, const webvtt_byte *text, webvtt_uint *pos, webvtt_uint len );

WEBVTT_INTERN webvtt_status webvtt_parse_size( webvtt_parser self,
  webvtt_cue *cue, const webvtt_byte *text, webvtt_uint *pos, webvtt_uint len );

WEBVTT_INTERN webvtt_status webvtt_parse_vertical( webvtt_parser self,
  webvtt_cue *cue, const webvtt_byte *text, webvtt_uint *pos, webvtt_uint len );

WEBVTT_INTERN int parse_timestamp( const webvtt_byte *b, webvtt_timestamp *result );

WEBVTT_INTERN webvtt_status do_push( webvtt_parser self, webvtt_uint token,
  webvtt_uint back, webvtt_uint state, void *data, webvtt_state_value_type type,
  webvtt_uint line, webvtt_uint column );

WEBVTT_INTERN webvtt_status webvtt_read_cuetext( webvtt_parser self,
  const webvtt_byte *b, webvtt_uint *ppos, webvtt_uint len,
  webvtt_bool finish );

WEBVTT_INTERN webvtt_status webvtt_proc_cuetext( webvtt_parser self,
  const webvtt_byte *b, webvtt_uint *ppos, webvtt_uint len,
  webvtt_bool finish );

WEBVTT_INTERN int parse_cueparams( webvtt_parser self, const webvtt_byte *text,
  webvtt_uint len, webvtt_cue *cue );





typedef
enum webvtt_token_flags_t
{
  
  TF_POSITIVE = 0x80000000,

  
  TF_NEGATIVE = 0x40000000,
  


  TF_SIGN_MASK = ( TF_POSITIVE | TF_NEGATIVE ),

  

  TF_FLAGS_MASK = TF_SIGN_MASK,

  
  TF_TOKEN_MASK = ( 0xFFFFFFFF & ~TF_FLAGS_MASK ),
} webvtt_token_flags;








WEBVTT_INTERN webvtt_bool token_in_list( webvtt_token search_for,
  const webvtt_token token_list[] );











WEBVTT_INTERN int find_token( webvtt_token search_for,
  const webvtt_token token_list[] );

#define BAD_TIMESTAMP(ts) ( ( ts ) == 0xFFFFFFFFFFFFFFFF )

#ifdef FATAL_ASSERTION
#  define SAFE_ASSERT(condition) assert(condition)
#  define DIE_IF(condition) assert( !(condition) )
#else
#  ifdef BREAK_ON_ASSERTION
static void
break_on_assert(void) {
#if WEBVTT_OS_WIN32
  
  __declspec(dllimport) void __stdcall DebugBreak( void );
  DebugBreak();
#else
  volatile int *ptr = (volatile int *)0;
  *ptr = 1;
#endif
}
#    define SAFE_ASSERT(condition) \
if( !(condition) ) { \
  break_on_assert(); \
  return WEBVTT_FAILED_ASSERTION; \
}
#    define DIE_IF(condition) \
if( (condition) ) { \
  break_on_assert(); \
}
#  else
#    define SAFE_ASSERT(condition) \
if( !(condition) ) { \
  return WEBVTT_FAILED_ASSERTION; \
}
#    define DIE_IF(condition)
#  endif
#endif

#define ERROR_AT(errno, line, column) \
do \
{ \
  if( !self->error \
    || self->error( (self->userdata), (line), (column), (errno) ) < 0 ) { \
    return WEBVTT_PARSE_ERROR; \
  } \
} while(0)

#define ERROR(error) \
  ERROR_AT( (error), (self->line), (self->column) )

#define ERROR_AT_COLUMN(error, column) \
  ERROR_AT( (error), (self->line), (column) )
#endif
