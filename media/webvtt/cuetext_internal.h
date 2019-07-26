


























#ifndef __INTERN_CUETEXT_H__
# define __INTERN_CUETEXT_H__
# include <webvtt/util.h>
# include <webvtt/string.h>
# include <webvtt/cue.h>

typedef enum webvtt_token_type_t webvtt_token_type;
typedef enum webvtt_token_state_t webvtt_token_state;

typedef struct webvtt_cuetext_token_t webvtt_cuetext_token;
typedef struct webvtt_start_token_data_t webvtt_start_token_data;




enum
webvtt_token_type_t {
  START_TOKEN, 
  END_TOKEN, 
  TIME_STAMP_TOKEN, 
  TEXT_TOKEN 
};




enum
webvtt_token_state_t {
  DATA, 
  ESCAPE, 
  TAG, 
  START_TAG, 
  START_TAG_CLASS, 


  START_TAG_ANNOTATION, 


  END_TAG, 

  TIME_STAMP_TAG 

};






struct
webvtt_start_token_data_t {
  webvtt_stringlist *css_classes;
  webvtt_string annotations;
};





struct
webvtt_cuetext_token_t {
  webvtt_token_type token_type;
  webvtt_string tag_name; 
  union {
    webvtt_string text;
    webvtt_timestamp time_stamp;
    webvtt_start_token_data start_token_data;
  };
};





WEBVTT_INTERN webvtt_status webvtt_create_token( webvtt_cuetext_token **token, webvtt_token_type token_type );
WEBVTT_INTERN webvtt_status webvtt_create_start_token( webvtt_cuetext_token **token, webvtt_string *tag_name,
    webvtt_stringlist *css_classes, webvtt_string *annotation );
WEBVTT_INTERN webvtt_status webvtt_create_end_token( webvtt_cuetext_token **token, webvtt_string *tag_name );
WEBVTT_INTERN webvtt_status webvtt_create_text_token( webvtt_cuetext_token **token, webvtt_string *text );
WEBVTT_INTERN webvtt_status webvtt_create_timestamp_token( webvtt_cuetext_token **token,
    webvtt_timestamp time_stamp );




WEBVTT_INTERN int tag_accepts_annotation( webvtt_string *tag_name );




WEBVTT_INTERN void webvtt_delete_token( webvtt_cuetext_token **token );






WEBVTT_INTERN webvtt_status webvtt_node_kind_from_tag_name( webvtt_string *tag_name, webvtt_node_kind *kind );





WEBVTT_INTERN webvtt_status webvtt_create_node_from_token( webvtt_cuetext_token *token, webvtt_node **node, webvtt_node *parent );






WEBVTT_INTERN webvtt_status webvtt_tokenizer( webvtt_byte **position, webvtt_cuetext_token **token );








WEBVTT_INTERN webvtt_status webvtt_data_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );




WEBVTT_INTERN webvtt_status webvtt_escape_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );




WEBVTT_INTERN webvtt_status webvtt_tag_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );




WEBVTT_INTERN webvtt_status webvtt_start_tag_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );




WEBVTT_INTERN webvtt_status webvtt_class_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_stringlist *css_classes );





WEBVTT_INTERN webvtt_status webvtt_annotation_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *annotation );




WEBVTT_INTERN webvtt_status webvtt_end_tag_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );




WEBVTT_INTERN webvtt_status webvtt_timestamp_state( webvtt_byte **position,
  webvtt_token_state *token_state, webvtt_string *result );

WEBVTT_INTERN webvtt_status webvtt_parse_cuetext( webvtt_parser self, webvtt_cue *cue, webvtt_string *payload, int finished );

#endif
