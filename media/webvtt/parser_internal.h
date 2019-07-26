


























#ifndef __INTERN_PARSER_H__
# define __INTERN_PARSER_H__
# include <webvtt/parser.h>
# include "string_internal.h"

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
  L_START = 0, L_BOM0, L_BOM1, L_WEBVTT0, L_WEBVTT1, L_WEBVTT2, L_WEBVTT3, L_WEBVTT4, L_WEBVTT5, L_DASH0, L_SEP1,
  L_DIGIT0, L_NEWLINE0, L_WHITESPACE, L_POSITION0, L_POSITION1, L_POSITION2, L_POSITION3, L_POSITION4, L_POSITION5,
  L_POSITION6, L_ALIGN0, L_ALIGN1, L_ALIGN2, L_ALIGN3, L_L0, L_LINE1, L_LINE2, L_LINE3,
  L_VERTICAL0, L_VERTICAL1, L_VERTICAL2, L_VERTICAL3, L_VERTICAL4, L_VERTICAL5, L_VERTICAL6, L_RL0,
  L_S0, L_SIZE1, L_SIZE2, L_START1, L_START2, L_START3, L_MIDDLE0, L_MIDDLE1, L_MIDDLE2, L_MIDDLE3,
  L_MIDDLE4, L_END0, L_END1, L_TIMESTAMP1, L_TIMESTAMP2, L_TIMESTAMP3, L_RIGHT1, L_RIGHT2,
  L_RIGHT3, L_NOTE1, L_NOTE2, L_NOTE3, L_LEFT1, L_LEFT2,
} webvtt_lexer_state;

typedef struct
webvtt_state {
  webvtt_parse_state state;
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
WEBVTT_INTERN int parse_timestamp( const webvtt_byte *b, webvtt_timestamp *result );

#define BAD_TIMESTAMP(ts) ( ( ts ) == 0xFFFFFFFFFFFFFFFF )

#define ERROR(Code) \
do \
{ \
  if( !self->error || self->error(self->userdata,self->line,self->column,Code) < 0 ) \
    return WEBVTT_PARSE_ERROR; \
} while(0)

#define ERROR_AT_COLUMN(Code,Column) \
do \
{ \
  if( !self->error || self->error(self->userdata,self->line,(Column),Code) < 0 ) \
    return WEBVTT_PARSE_ERROR; \
} while(0)

#endif
