


























#include "parser_internal.h"
#include "cuetext_internal.h"
#include "cue_internal.h"
#include <string.h>

#define _ERROR(X) do { if( skip_error == 0 ) { ERROR(X); } } while(0)

static const webvtt_byte separator[] = {
  UTF8_HYPHEN_MINUS, UTF8_HYPHEN_MINUS, UTF8_GREATER_THAN
};

#define MSECS_PER_HOUR (3600000)
#define MSECS_PER_MINUTE (60000)
#define MSECS_PER_SECOND (1000)
#define BUFFER (self->buffer + self->position)
#define MALFORMED_TIME ((webvtt_timestamp_t)-1.0)

static int find_bytes( const webvtt_byte *buffer, webvtt_uint len, const webvtt_byte *sbytes, webvtt_uint slen );
static webvtt_status webvtt_skipwhite( const webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint len );
static webvtt_int64 parse_int( const webvtt_byte **pb, int *pdigits );

WEBVTT_EXPORT webvtt_status
webvtt_create_parser( webvtt_cue_fn on_read,
                      webvtt_error_fn on_error, void *
                      userdata,
                      webvtt_parser *ppout )
{
  webvtt_parser p;
  if( !on_read || !on_error || !ppout ) {
    return WEBVTT_INVALID_PARAM;
  }

  if( !( p = ( webvtt_parser )webvtt_alloc0( sizeof * p ) ) ) {
    return WEBVTT_OUT_OF_MEMORY;
  }

  memset( p->astack, 0, sizeof( p->astack ) );
  p->stack = p->astack;
  p->top = p->stack;
  p->top->state = T_INITIAL;
  p->stack_alloc = sizeof( p->astack ) / sizeof( p->astack[0] );

  p->read = on_read;
  p->error = on_error;
  p->column = p->line = 1;
  p->userdata = userdata;
  p->finished = 0;
  *ppout = p;

  return WEBVTT_SUCCESS;
}










static void
finish_cue( webvtt_parser self, webvtt_cue **pcue )
{
  if( pcue ) {
    webvtt_cue *cue = *pcue;
    if( cue ) {
      if( webvtt_validate_cue( cue ) ) {
        self->read( self->userdata, cue );
      } else {
        webvtt_release_cue( &cue );
      }
      *pcue = 0;
    }
  }
}







WEBVTT_INTERN void
cleanup_stack( webvtt_parser self )
{
  webvtt_state *st = self->top;
  while( st >= self->stack ) {
    switch( st->type ) {
      case V_CUE:
        webvtt_release_cue( &st->v.cue );
        break;
      case V_TEXT:
        webvtt_release_string( &st->v.text );
        break;
        




    }
    st->type = V_NONE;
    st->line = st->column = st->token = 0;
    st->v.cue = NULL;
    if( st > self->stack ) {
      --self->top;
    }
    --st;
  }
  if( self->stack != self->astack ) {
    




    webvtt_state *pst = self->stack;
    memset( self->astack, 0, sizeof( self->astack ) );
    self->stack = self->astack;
    self->stack_alloc = sizeof( self->astack ) / sizeof( *( self->astack ) );
    webvtt_free( pst );
  }
}




WEBVTT_EXPORT webvtt_status 
webvtt_finish_parsing( webvtt_parser self ) 
{
  webvtt_status status = WEBVTT_SUCCESS;
  
  if( !self->finished ) {
    self->finished = 1;
    
    switch( self->mode ) {
      



      case M_WEBVTT:
        if( self->top->type != V_NONE ) {
          ERROR( WEBVTT_CUE_INCOMPLETE );
        }
        break;
      




      case M_CUETEXT:
        status = webvtt_parse_cuetext( self, self->top->v.cue, 
                                       &self->line_buffer, self->finished );
        webvtt_release_string( &self->line_buffer );
        finish_cue( self, &self->top->v.cue );
        break;
      case M_SKIP_CUE:
        
        break;
      case M_READ_LINE:
        
        break;
    }
    cleanup_stack( self );
  }
  
  return status;
}

WEBVTT_EXPORT void
webvtt_delete_parser( webvtt_parser self )
{
  if( self ) {
    cleanup_stack( self );

    webvtt_release_string( &self->line_buffer );
    webvtt_free( self );
  }
}

#define BEGIN_STATE(State) case State: {
#define END_STATE } break;
#define IF_TOKEN(Token,Actions) case Token: { Actions } break;
#define BEGIN_DFA switch(top->state) {
#define END_DFA }
#define BEGIN_TOKEN switch(token) {
#define END_TOKEN }
#define IF_TRANSITION(Token,State) if( token == Token ) { self->state = State;
#define ELIF_TRANSITION(Token,State) } else IF_TRANSITION(Token,State)
#define ENDIF }
#define ELSE } else {

static int
find_newline( const webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint len )
{
  while( *pos < len ) {
    if( buffer[ *pos ] == UTF8_CARRIAGE_RETURN || buffer[ *pos ] == UTF8_LINE_FEED ) {
      return 1;
    } else {
      ( *pos )++;
    }
  }
  return -1;
}

static webvtt_status
webvtt_skipwhite( const webvtt_byte *buffer, webvtt_uint *pos, webvtt_uint len )
{
  if( !buffer || !pos ) {
    return WEBVTT_INVALID_PARAM;
  }

  for( ; *pos < len && webvtt_iswhite( buffer[ *pos ] ); (*pos)++ );

  return WEBVTT_SUCCESS;
}

static void
find_next_whitespace( const webvtt_byte *buffer, webvtt_uint *ppos, webvtt_uint len )
{
  webvtt_uint pos = *ppos;
  while( pos < len ) {
    webvtt_byte c = buffer[pos];
    if( c == UTF8_CARRIAGE_RETURN || c == UTF8_LINE_FEED || c == UTF8_SPACE || c == UTF8_TAB ) {
      break;
    }

    ++pos;
  }
  *ppos = pos;
}




static int
find_bytes( const webvtt_byte *buffer, webvtt_uint len,
    const webvtt_byte *sbytes, webvtt_uint slen )
{
  webvtt_uint slen2;
  
  if( !buffer || len < 1 || !sbytes || slen < 1 ) {
    return 0;
  }

  slen2 = slen - 1;
  while( len-- >= slen && *buffer ){
    if( *buffer == *sbytes && memcmp( buffer + 1, sbytes + 1, slen2 ) == 0 ) {
      return 1;
    }
    buffer++;
  }

  return 0;
}




#define SP (self->top)
#define AT_BOTTOM (self->top == self->stack)
#define ON_HEAP (self->stack_alloc == sizeof(p->astack) / sizeof(p->astack[0]))
#define STACK_SIZE ((webvtt_uint)(self->top - self->stack))
#define FRAME(i) (self->top - (i))
#define FRAMEUP(i) (self->top + (i))
#define RECHECK goto _recheck;
#define BACK (SP->back)



static webvtt_status
do_push( webvtt_parser self, webvtt_uint token, webvtt_uint back, webvtt_uint state, void *data, webvtt_state_value_type type, webvtt_uint line, webvtt_uint column )
{
  if( STACK_SIZE + 1 >= self->stack_alloc ) {
    webvtt_state *stack = ( webvtt_state * )webvtt_alloc0( sizeof( webvtt_state ) * ( self->stack_alloc << 1 ) ), *tmp;
    if( !stack ) {
      ERROR( WEBVTT_ALLOCATION_FAILED );
      return WEBVTT_OUT_OF_MEMORY;
    }
    memcpy( stack, self->stack, sizeof( webvtt_state ) * self->stack_alloc );
    tmp = self->stack;
    self->stack = stack;
    self->top = stack + ( self->top - tmp );
    if( tmp != self->astack ) {
      webvtt_free( tmp );
    }
  }
  ++self->top;
  self->top->state = state;
  self->top->type = type;
  self->top->token = ( webvtt_token )token;
  self->top->line = line;
  self->top->back = back;
  self->top->column = column;
  self->top->v.cue = ( webvtt_cue * )data;
  return WEBVTT_SUCCESS;
}
static int
do_pop( webvtt_parser self )
{
  int count = self->top->back;
  self->top -= count;
  self->top->back = 0;
  self->popped = 1;
  return count;
}

#define PUSH0(S,V,T) \
do { \
    self->popped = 0; \
    if( do_push(self,token,BACK+1,(S),(void*)(V),T,last_line, last_column) \
      == WEBVTT_OUT_OF_MEMORY ) \
      return WEBVTT_OUT_OF_MEMORY; \
  } while(0)

#define PUSH(S,B,V,T) \
do { \
  self->popped = 0; \
  if( do_push(self,token,(B),(S),(void*)(V),T,last_line, last_column) \
    == WEBVTT_OUT_OF_MEMORY ) \
    return WEBVTT_OUT_OF_MEMORY; \
  } while(0)

#define POP() \
do \
{ \
  --(self->top); \
  self->popped = 1; \
} while(0)
#define POPBACK() do_pop(self)

WEBVTT_INTERN int
parse_cueparams( webvtt_parser self, const webvtt_byte *buffer,
                 webvtt_uint len, webvtt_cue *cue )
{
  int digits;
  int have_ws = 0;
  int unexpected_whitespace = 0;
  webvtt_uint baddelim = 0;
  webvtt_uint pos = 0;
  webvtt_token last_token = 0;
  enum cp_state {
    CP_T1, CP_T2, CP_T3, CP_T4, CP_T5, 

    CP_CS0, 

    CP_SD, 

    CP_V1, 
    CP_P1, 
    CP_A1, 
    CP_S1, 
    CP_L1, 

    CP_SV, 

    CP_V2,
    CP_P2,
    CP_A2,
    CP_S2,
    CP_L2,
  };

  enum cp_state last_state = CP_T1;
  enum cp_state state = CP_T1;

#define SETST(X) do { baddelim = 0; last_state = state; state = (X); } while( 0 )

  self->token_pos = 0;
  while( pos < len ) {
    webvtt_uint last_column = self->column;
    webvtt_token token = webvtt_lex( self, buffer, &pos, len, 1 );
_recheck:
    switch( state ) {
        
      case CP_T1:
        if( token == WHITESPACE && !unexpected_whitespace ) {
          ERROR_AT_COLUMN( WEBVTT_UNEXPECTED_WHITESPACE, self->column );
          unexpected_whitespace = 1;
        } else if( token == TIMESTAMP )
          if( !parse_timestamp( self->token, &cue->from ) ) {
            ERROR_AT_COLUMN(
              ( BAD_TIMESTAMP( cue->from )
                ? WEBVTT_EXPECTED_TIMESTAMP
                : WEBVTT_MALFORMED_TIMESTAMP ), last_column  );
            if( !webvtt_isdigit( self->token[self->token_pos - 1] ) ) {
              while( pos < len && buffer[pos] != 0x09 && buffer[pos] != 0x20 ) { ++pos; }
            }
            if( BAD_TIMESTAMP( cue->from ) )
            { return -1; }
            SETST( CP_T2 );
          } else {
            SETST( CP_T2 );
          }
        else {
          ERROR_AT_COLUMN( WEBVTT_EXPECTED_TIMESTAMP, last_column );
          return -1;
        }
        break;
        
      case CP_T5:
        if( token == WHITESPACE ) {
          
        } else if( token == TIMESTAMP )
          if( !parse_timestamp( self->token, &cue->until ) ) {
            ERROR_AT_COLUMN(
              ( BAD_TIMESTAMP( cue->until )
                ? WEBVTT_EXPECTED_TIMESTAMP
                : WEBVTT_MALFORMED_TIMESTAMP ), last_column  );
            if( !webvtt_isdigit( self->token[self->token_pos - 1] ) ) {
              while( pos < len && buffer[pos] != 0x09 && buffer[pos] != 0x20 ) { ++pos; }
            }
            if( BAD_TIMESTAMP( cue->until ) )
            { return -1; }
            SETST( CP_CS0 );
          } else {
            SETST( CP_CS0 );
          }
        else {
          ERROR_AT_COLUMN( WEBVTT_EXPECTED_TIMESTAMP, last_column );
          return -1;
        }
        break;

        
      case CP_T2:
        switch( token ) {
          case SEPARATOR:
            ERROR_AT_COLUMN( WEBVTT_EXPECTED_WHITESPACE, last_column );
            SETST( CP_T4 );
            break;
          case WHITESPACE:
            SETST( CP_T3 );
            break;
        }
        break;
      case CP_T3:
        switch( token ) {
          case WHITESPACE: 
            break;

          case SEPARATOR:
            SETST( CP_T4 );
            break;

          case TIMESTAMP:
            ERROR( WEBVTT_MISSING_CUETIME_SEPARATOR );
            SETST( CP_T5 );
            goto _recheck;

          default: 
            ERROR_AT_COLUMN( WEBVTT_EXPECTED_CUETIME_SEPARATOR, last_column );
            return -1;
        }
        break;
      case CP_T4:
        switch( token ) {
          case WHITESPACE:
            SETST( CP_T5 );
            break;
          case TIMESTAMP:
            ERROR_AT_COLUMN( WEBVTT_EXPECTED_WHITESPACE, last_column );
            goto _recheck;
          default:
            ERROR_AT_COLUMN( WEBVTT_EXPECTED_WHITESPACE, last_column );
            goto _recheck;
        }
        break;
#define CHKDELIM \
if( baddelim ) \
  ERROR_AT_COLUMN(WEBVTT_INVALID_CUESETTING_DELIMITER,baddelim); \
else if( !have_ws ) \
  ERROR_AT_COLUMN(WEBVTT_EXPECTED_WHITESPACE,last_column);

        









      case CP_CS0:
        switch( token ) {
          case WHITESPACE:
            have_ws = last_column;
            break;
          case COLON:
            ERROR_AT_COLUMN( WEBVTT_MISSING_CUESETTING_KEYWORD, last_column );
            break;
          case VERTICAL:
            CHKDELIM have_ws = 0;
            SETST( CP_V1 );
            break;
          case POSITION:
            CHKDELIM have_ws = 0;
            SETST( CP_P1 );
            break;
          case ALIGN:
            CHKDELIM have_ws = 0;
            SETST( CP_A1 );
            break;
          case SIZE:
            CHKDELIM have_ws = 0;
            SETST( CP_S1 );
            break;
          case LINE:
            CHKDELIM have_ws = 0;
            SETST( CP_L1 );
            break;
          default:
            if( have_ws ) {
              ERROR_AT_COLUMN( WEBVTT_INVALID_CUESETTING, last_column );
              while( pos < len && buffer[pos] != 0x09 && buffer[pos] != 0x20 ) { ++pos; }
            } else if( token == BADTOKEN ) {
              
              if( !baddelim ) {
                baddelim = last_column;
              }
              ++pos;
            }
        }
        break;
#define CS1(S) \
  if( token == COLON ) \
  { if(have_ws) { ERROR_AT_COLUMN(WEBVTT_UNEXPECTED_WHITESPACE,have_ws); } SETST((S)); have_ws = 0; } \
  else if( token == WHITESPACE && !have_ws ) \
  { \
    have_ws = last_column; \
  } \
  else \
  { \
    switch(token) \
    { \
    case LR: case RL: case INTEGER: case PERCENTAGE: case START: case MIDDLE: case END: case LEFT: case RIGHT: \
       ERROR_AT_COLUMN(WEBVTT_MISSING_CUESETTING_DELIMITER,have_ws ? have_ws : last_column); break; \
    default: \
      ERROR_AT_COLUMN(WEBVTT_INVALID_CUESETTING_DELIMITER,last_column); \
      while( pos < len && buffer[pos] != 0x20 && buffer[pos] != 0x09 ) ++pos; \
      break; \
    } \
    have_ws = 0; \
  }

        








      case CP_V1:
        CS1( CP_V2 );
        break;
      case CP_P1:
        CS1( CP_P2 );
        break;
      case CP_A1:
        CS1( CP_A2 );
        break;
      case CP_S1:
        CS1( CP_S2 );
        break;
      case CP_L1:
        CS1( CP_L2 );
        break;
#undef CS1


#define BV(T) \
ERROR_AT_COLUMN(WEBVTT_##T##_BAD_VALUE,last_column); \
while( pos < len && buffer[pos] != 0x20 && buffer[pos] != 0x09 ) ++pos; \
SETST(CP_CS0);


#define HV(T) \
if( cue->flags & CUE_HAVE_##T ) \
{ \
  ERROR_AT_COLUMN(WEBVTT_##T##_ALREADY_SET,last_column); \
}

#define WS \
case WHITESPACE: \
  if( !have_ws ) \
  { \
    ERROR_AT_COLUMN(WEBVTT_UNEXPECTED_WHITESPACE,last_column); \
    have_ws = last_column; \
  } \
break


#define SV(T) cue->flags |= CUE_HAVE_##T
      case CP_V2:
        HV( VERTICAL );
        switch( token ) {
            WS;
          case LR:
            cue->settings.vertical = WEBVTT_VERTICAL_LR;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( VERTICAL );
            break;
          case RL:
            cue->settings.vertical = WEBVTT_VERTICAL_RL;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( VERTICAL );
            break;
          default:
            BV( VERTICAL );
        }
        break;

      case CP_P2:
        HV( POSITION );
        switch( token ) {
            WS;
          case PERCENTAGE: {
            int digits;
            const webvtt_byte *t = self->token;
            webvtt_int64 v = parse_int( &t, &digits );
            if( v < 0 ) {
              BV( POSITION );
            }
            cue->settings.position = ( webvtt_uint )v;
            SETST( CP_CS0 );
            SV( POSITION );
          }
          break;
          default:
            BV( POSITION );
            break;
        }
        break;

      case CP_A2:
        HV( ALIGN );
        switch( token ) {
            WS;
          case START:
            cue->settings.align = WEBVTT_ALIGN_START;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( ALIGN );
            break;
          case MIDDLE:
            cue->settings.align = WEBVTT_ALIGN_MIDDLE;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( ALIGN );
            break;
          case END:
            cue->settings.align = WEBVTT_ALIGN_END;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( ALIGN );
            break;
          case LEFT:
            cue->settings.align = WEBVTT_ALIGN_LEFT;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( ALIGN );
            break;
          case RIGHT:
            cue->settings.align = WEBVTT_ALIGN_RIGHT;
            have_ws = 0;
            SETST( CP_CS0 );
            SV( ALIGN );
            break;
          default:
            BV( ALIGN );
            break;
        }
        break;

      case CP_S2:
        HV( SIZE );
        switch( token ) {
            WS;
          case PERCENTAGE: {
            int digits;
            const webvtt_byte *t = self->token;
            webvtt_int64 v = parse_int( &t, &digits );
            if( v < 0 ) {
              BV( SIZE );
            }
            cue->settings.size = ( webvtt_uint )v;
            SETST( CP_CS0 );
            SV( SIZE );
          }
          break;
          default:
            BV( SIZE );
            break;
        }
        break;

      case CP_L2:
        HV( LINE );
        switch( token ) {
            WS;
          case INTEGER: {
            const webvtt_byte *t = self->token;
            webvtt_int64 v = parse_int( &t, &digits );
            cue->snap_to_lines = 1;
            cue->settings.line = ( int )v;
            SETST( CP_CS0 );
            SV( LINE );
          }
          break;
          case PERCENTAGE: {
            const webvtt_byte *t = self->token;
            webvtt_int64 v = parse_int( &t, &digits );
            if( v < 0 ) {
              BV( POSITION );
            }
            cue->snap_to_lines = 0;
            cue->settings.line = ( int )v;
            SETST( CP_CS0 );
            SV( LINE );
          }
          break;
          default:
            BV( LINE );
            break;
        }
#undef BV
#undef HV
#undef SV
#undef WS
    }
    self->token_pos = 0;
    last_token = token;
  }
  


  if( state != CP_CS0 ) {
    
    if( state < CP_CS0 ) {
      ERROR( WEBVTT_UNFINISHED_CUETIMES );
      return -1;
    } else {
      
      webvtt_error e = WEBVTT_INVALID_CUESETTING;
      switch( state ) {
        case CP_V2:
          e = WEBVTT_VERTICAL_BAD_VALUE;
          break;
        case CP_P2:
          e = WEBVTT_POSITION_BAD_VALUE;
          break;
        case CP_A2:
          e = WEBVTT_ALIGN_BAD_VALUE;
          break;
        case CP_S2:
          e = WEBVTT_SIZE_BAD_VALUE;
          break;
        case CP_L2:
          e = WEBVTT_LINE_BAD_VALUE;
          break;
      }
      ERROR( e );
    }
  } else {
    if( baddelim ) {
      ERROR_AT_COLUMN( WEBVTT_INVALID_CUESETTING_DELIMITER, baddelim );
    }
  }
#undef SETST
  return 0;
}

static webvtt_status
parse_webvtt( webvtt_parser self, const webvtt_byte *buffer, webvtt_uint *ppos,
              webvtt_uint len, webvtt_parse_mode *mode, int finish )
{
  webvtt_status status = WEBVTT_SUCCESS;
  webvtt_token token;
  webvtt_uint pos = *ppos;
  int settings_delimiter = 0;
  int skip_error = 0;
  int settings_whitespace = 0;

  while( pos < len ) {
    webvtt_uint last_column, last_line, last_pos;
    skip_error = 0;
_next:
    last_column = self->column;
    last_line = self->line;
    last_pos = pos;

    



    if( SP->state == T_CUEREAD ) {
      int v;
      webvtt_uint old_pos = pos;
      if( v = webvtt_string_getline( &SP->v.text, buffer, &pos,
                                        len, 0, finish, 0 ) ) {
        if( v < 0 ) {
          webvtt_release_string( &SP->v.text );
          SP->type = V_NONE;
          POP();
          ERROR( WEBVTT_ALLOCATION_FAILED );
          status = WEBVTT_OUT_OF_MEMORY;
          goto _finish;
        }
        
        POP();
      }
    }

    








    if( SP->state != T_CUE || !( self->popped && FRAMEUP( 1 )->state == T_CUEREAD ) ) {
      


      token = webvtt_lex( self, buffer, &pos, len, finish );
      if( token == UNFINISHED ) {
        if( finish ) {
          token = BADTOKEN;
        } else if( pos == len ) {
          goto _finish;
        }
      }
    }
_recheck:
    switch( SP->state ) {
      case T_INITIAL:
        











        if( token == WEBVTT ) {
          PUSH0( T_TAG, 0, V_NONE );
          break;
        } else {
          if( pos != len ) {
            if( !skip_error ) {
              ERROR_AT_COLUMN( WEBVTT_MALFORMED_TAG, last_column );
              skip_error = 1;
            }
            status = WEBVTT_PARSE_ERROR;
            goto _finish;
          }
        }
        break;

      case T_TAG:
        








        if( token == WHITESPACE ) {
          
          PUSH0( T_TAGCOMMENT, 0, V_NONE );
        } else if( token == NEWLINE ) {
          
          POPBACK();
          self->popped = 0;
          SP->state = T_BODY;
          PUSH0( T_EOL, 1, V_INTEGER );
          break;
        } else {
          




          if( !skip_error ) {
            ERROR_AT_COLUMN( WEBVTT_MALFORMED_TAG, FRAME( 1 )->column );
            skip_error = 1;
            status = WEBVTT_PARSE_ERROR;
            goto _finish;
          }
        }
        break;

        


      case T_TAGCOMMENT:
        switch( token ) {
          case NEWLINE:
            




            POPBACK();
            PUSH0( T_EOL, 1, V_INTEGER );
            break;
          default:
            find_newline( buffer, &pos, len );
            continue;
        }
        break;

      case T_CUEID:
        switch( token ) {
            



          case NEWLINE:
            SP->state = T_FROM;
            break;
        }

        


      case T_EOL:
        switch( token ) {
          case NEWLINE:
            SP->v.value++;
            break;
          default:
            POPBACK();
            RECHECK
        }
        break;

      case T_BODY:
        if( self->popped && FRAMEUP( 1 )->state == T_EOL ) {
          if( FRAMEUP( 1 )->v.value < 2 ) {
            ERROR_AT_COLUMN( WEBVTT_EXPECTED_EOL, 1 );
          }
          FRAMEUP( 1 )->state = 0;
          FRAMEUP( 1 )->v.cue = NULL;
        }
        if( token == NOTE ) {
          PUSH0( T_COMMENT, 0, V_NONE );
        } else if( token != NEWLINE ) {
          webvtt_cue *cue = 0;
          webvtt_string tk = { 0 };
          if( WEBVTT_FAILED( status = webvtt_create_cue( &cue ) ) ) {
            if( status == WEBVTT_OUT_OF_MEMORY ) {
              ERROR( WEBVTT_ALLOCATION_FAILED );
            }
            goto _finish;
          }
          if( WEBVTT_FAILED( status = webvtt_create_string_with_text( &tk,
            self->token, self->token_pos ) ) ) {
            if( status == WEBVTT_OUT_OF_MEMORY ) {
              ERROR( WEBVTT_ALLOCATION_FAILED );
            }
            webvtt_release_cue( &cue );
            goto _finish;
          }
          PUSH0( T_CUE, cue, V_CUE );
          PUSH0( T_CUEREAD, 0, V_TEXT );
          SP->v.text.d = tk.d;
        }
        break;


      case T_CUE:
        if( self->popped && FRAMEUP( 1 )->state == T_CUEREAD ) {
          



          webvtt_cue *cue = SP->v.cue;
          webvtt_state *st = FRAMEUP( 1 );
          webvtt_string text = st->v.text;

          st->type = V_NONE;
          st->v.cue = NULL;

          




          if( find_bytes( webvtt_string_text( &text ), webvtt_string_length( &text ), separator,
                          sizeof( separator ) ) ) {
            


            int v;
            
            self->column = 1;
            if( ( v = parse_cueparams( self, webvtt_string_text( &text ),
                                       webvtt_string_length( &text ), cue ) ) < 0 ) {
              if( v == WEBVTT_PARSE_ERROR ) {
                status = WEBVTT_PARSE_ERROR;
                goto _finish;
              }
              webvtt_release_string( &text );
              *mode = M_SKIP_CUE;
              goto _finish;
            } else {
              webvtt_release_string( &text );
              cue->flags |= CUE_HAVE_CUEPARAMS;
              *mode = M_CUETEXT;
              goto _finish;
            }
          } else {
            
            if( cue->flags & CUE_HAVE_ID ) {
              




              webvtt_release_string( &text );
              ERROR( WEBVTT_CUE_INCOMPLETE );
              *mode = M_SKIP_CUE;
              goto _finish;
            } else {
              self->column += webvtt_string_length( &text );
              if( WEBVTT_FAILED( status = webvtt_string_append(
                                            &cue->id, webvtt_string_text( &text ), webvtt_string_length( &text ) ) ) ) {
                webvtt_release_string( &text );
                ERROR( WEBVTT_ALLOCATION_FAILED );
              }

              cue->flags |= CUE_HAVE_ID;
            }
          }
          webvtt_release_string( &text );
          self->popped = 0;
        } else {
          webvtt_cue *cue = SP->v.cue;
          
          if( token == NEWLINE ) {
            if( cue->flags & CUE_HAVE_CUEPARAMS ) {
              *mode = M_CUETEXT;
            } else if( cue->flags & CUE_HAVE_ID ) {
              PUSH0( T_CUEREAD, 0, V_NONE );
            } else {
              
              POPBACK();
            }
          }
        }
        break;

    }

    


    self->token_pos = 0;
  }


_finish:
  if( status == WEBVTT_OUT_OF_MEMORY ) {
    cleanup_stack( self );
  }
  *ppos = pos;
  return status;
}

static webvtt_status
read_cuetext( webvtt_parser self, const webvtt_byte *b, webvtt_uint
*ppos, webvtt_uint len, webvtt_parse_mode *mode, webvtt_bool finish )
{
  webvtt_status status = WEBVTT_SUCCESS;
  webvtt_uint pos = *ppos;
  int finished = 0;
  do {
    int v;
    if( ( v = webvtt_string_getline( &self->line_buffer, b, &pos, len, &self->truncate, finish, 1 ) ) ) {
      if( v < 0 ) {
        status = WEBVTT_OUT_OF_MEMORY;
        goto _finish;
      }

      if( self->line_buffer.d->length > 1 && self->line_buffer.d->text[ self->line_buffer.d->length - 1 ] == UTF8_LINE_FEED ) {
        


        finished = 1;
      }
      webvtt_string_putc( &self->line_buffer, UTF8_LINE_FEED );

      if( pos < len ) {
        if( b[pos] == UTF8_CARRIAGE_RETURN ) {
          if( len - pos >= 2 && b[pos + 1] == UTF8_LINE_FEED ) {
            ++pos;
          }
          ++pos;
        } else {
          ++pos;
        }
      }
    }
  } while( pos < len && !finished );
_finish:
  *ppos = pos;
  



  if( pos >= len && !WEBVTT_FAILED( status ) && !finished ) {
    status = WEBVTT_UNFINISHED;
  }
  return status;
}

WEBVTT_EXPORT webvtt_status
webvtt_parse_chunk( webvtt_parser self, const void *buffer, webvtt_uint len )
{
  webvtt_status status;
  webvtt_uint pos = 0;
  const webvtt_byte *b = ( const webvtt_byte * )buffer;

  while( pos < len ) {
    switch( self->mode ) {
      case M_WEBVTT:
        if( WEBVTT_FAILED( status = parse_webvtt( self, b, &pos, len, &self->mode, self->finished ) ) ) {
          return status;
        }
        break;

      case M_CUETEXT:
        


        if( WEBVTT_FAILED( status = read_cuetext( self, b, &pos, len, &self->mode, self->finished ) ) ) {
          if( status == WEBVTT_UNFINISHED ) {
            
            return WEBVTT_SUCCESS;
          }
          return status;
        }
        



        status = webvtt_parse_cuetext( self, SP->v.cue, &self->line_buffer, self->finished );

        


        finish_cue( self, &SP->v.cue );

        


        SP->type = V_NONE;
        webvtt_release_string( &self->line_buffer );
        self->mode = M_WEBVTT;

        
        if( WEBVTT_FAILED( status ) ) {
          return status;
        }
        break;

      case M_SKIP_CUE:
        if( WEBVTT_FAILED( status = read_cuetext( self, b, &pos, len, &self->mode, self->finished ) ) ) {
          return status;
        }
        webvtt_release_string( &self->line_buffer );
        self->mode = M_WEBVTT;
        break;

      case M_READ_LINE: {
        



        int ret;
        if( ( ret = webvtt_string_getline( &self->line_buffer, b, &pos, len, &self->truncate, self->finished, 0 ) ) ) {
          if( ret < 0 ) {
            ERROR( WEBVTT_ALLOCATION_FAILED );
            return WEBVTT_OUT_OF_MEMORY;
          }
          self->mode = M_WEBVTT;
        }
        break;
      }
    }
    if( WEBVTT_FAILED( status = webvtt_skipwhite( b, &pos, len ) ) ) {
      return status;
    }
  }

  return WEBVTT_SUCCESS;
}

#undef SP
#undef AT_BOTTOM
#undef ON_HEAP
#undef STACK_SIZE
#undef FRAME
#undef PUSH
#undef POP




static webvtt_int64
parse_int( const webvtt_byte **pb, int *pdigits )
{
  int digits = 0;
  webvtt_int64 result = 0;
  webvtt_int64 mul = 1;
  const webvtt_byte *b = *pb;
  while( *b ) {
    webvtt_byte ch = *b;
    if( webvtt_isdigit( ch ) ) {
      


      result = result * 10 + ( ch - UTF8_DIGIT_ZERO );
      ++digits;
    } else if( mul == 1 && digits == 0 && ch == UTF8_HYPHEN_MINUS ) {
      mul = -1;
    } else {
      break;
    }
    ++b;
  }
  *pb = b;
  if( pdigits ) {
    *pdigits = digits;
  }
  return result * mul;
}





WEBVTT_INTERN int
parse_timestamp( const webvtt_byte *b, webvtt_timestamp *result )
{
  webvtt_int64 tmp;
  int have_hours = 0;
  int digits;
  int malformed = 0;
  webvtt_int64 v[4];
  if ( !webvtt_isdigit( *b ) ) {
    goto _malformed;
  }

  
  v[0] = parse_int( &b, &digits );

  



  if ( digits != 2 || v[0] > 59 ) {
    have_hours = 1;
  }

  
  if ( !*b || *b++ != UTF8_COLON ) {
    malformed = 1;
  }

  
  if ( !*b || !webvtt_isdigit( *b ) ) {
    malformed = 1;
  }

  
  v[1] = parse_int( &b, &digits );
  if( digits != 2 ) {
    malformed = 1;
  }

  

  if ( have_hours || ( *b == UTF8_COLON ) ) {
    if( *b++ != UTF8_COLON ) {
      goto _malformed;
    }
    if( !*b || !webvtt_isdigit( *b ) ) {
      malformed = 1;
    }
    v[2] = parse_int( &b, &digits );
    if( digits != 2 ) {
      malformed = 1;
    }
  } else {
    
    v[2] = v[1];
    v[1] = v[0];
    v[0] = 0;
  }

  

  if( *b++ != UTF8_FULL_STOP || !webvtt_isdigit( *b ) ) {
    goto _malformed;
  }
  v[3] = parse_int( &b, &digits );
  if( digits != 3 ) {
    malformed = 1;
  }

  
  if( v[3] > 999 ) {
#define MILLIS_PER_SEC (1000)
    tmp = v[3];
    v[2] += tmp / MILLIS_PER_SEC;
    v[3] = tmp % MILLIS_PER_SEC;
    malformed = 1;
  }
  if( v[2] > 59 ) {
#define SEC_PER_MIN (60)
    tmp = v[2];
    v[1] += tmp / SEC_PER_MIN;
    v[2] = tmp % SEC_PER_MIN;
    malformed = 1;
  }
  if( v[1] > 59 ) {
#define MIN_PER_HOUR (60)
    tmp = v[1];
    v[0] += tmp / MIN_PER_HOUR;
    v[1] = tmp % MIN_PER_HOUR;
    malformed = 1;
  }

  *result = ( webvtt_timestamp )( v[0] * MSECS_PER_HOUR )
            + ( v[1] * MSECS_PER_MINUTE )
            + ( v[2] * MSECS_PER_SECOND )
            + ( v[3] );

  if( malformed ) {
    return 0;
  }
  return 1;
_malformed:
  *result = 0xFFFFFFFFFFFFFFFF;
  return 0;
}
