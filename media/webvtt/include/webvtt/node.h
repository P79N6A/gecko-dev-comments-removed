

























 
#ifndef __WEBVTT_NODE_H__
# define __WEBVTT_NODE_H__
# include <webvtt/string.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum
webvtt_node_kind_t {
  WEBVTT_NODE_LEAF = 0x80000000,
  WEBVTT_NODE_INTERNAL = 0x00000000,

  


  WEBVTT_NODE_INTERNAL_START = 0,
  WEBVTT_CLASS = 0 | WEBVTT_NODE_INTERNAL,
  WEBVTT_ITALIC = 1 | WEBVTT_NODE_INTERNAL,
  WEBVTT_BOLD = 2 | WEBVTT_NODE_INTERNAL,
  WEBVTT_UNDERLINE = 3 | WEBVTT_NODE_INTERNAL,
  WEBVTT_RUBY = 4 | WEBVTT_NODE_INTERNAL,
  WEBVTT_RUBY_TEXT = 5 | WEBVTT_NODE_INTERNAL,
  WEBVTT_VOICE = 6 | WEBVTT_NODE_INTERNAL,

  



  WEBVTT_HEAD_NODE = 7,

  WEBVTT_NODE_INTERNAL_END = 7,

  


  WEBVTT_NODE_LEAF_START = 256,
  WEBVTT_TEXT = 256 | WEBVTT_NODE_LEAF,
  WEBVTT_TIME_STAMP = 257 | WEBVTT_NODE_LEAF,

  WEBVTT_NODE_LEAF_END = 257,

  
  WEBVTT_EMPTY_NODE = 258
} webvtt_node_kind;

#define WEBVTT_IS_LEAF(Kind) ( ((Kind) & WEBVTT_NODE_LEAF) != 0 )
#define WEBVTT_NODE_INDEX(Kind) ( (Kind) & ~WEBVTT_NODE_LEAF )
#define WEBVTT_IS_VALID_LEAF_NODE(Kind) ( WEBVTT_IS_LEAF(Kind) && (WEBVTT_NODE_INDEX(Kind) >= WEBVTT_NODE_LEAF_START && WEBVTT_NODE_INDEX(Kind) <= WEBVTT_NODE_LEAF_END ) )
#define WEBVTT_IS_VALID_INTERNAL_NODE(Kind) ( (!WEBVTT_IS_LEAF(Kind)) && (WEBVTT_NODE_INDEX(Kind) >= WEBVTT_NODE_INTERNAL_START && WEBVTT_NODE_INDEX(Kind) <= WEBVTT_NODE_INTERNAL_END) )
#define WEBVTT_IS_VALID_NODE_KIND(Kind) ( WEBVTT_IS_VALID_INTERNAL_NODE(Kind) || WEBVTT_IS_VALID_LEAF_NODE(Kind) )

struct webvtt_internal_node_data_t;

typedef struct
webvtt_node_t {

  struct webvtt_refcount_t refs;
  




  struct webvtt_node_t *parent;
  webvtt_node_kind kind;

  union {
    webvtt_string text;
    webvtt_timestamp timestamp;
    struct webvtt_internal_node_data_t *internal_data;
  } data;
} webvtt_node;

typedef struct
webvtt_internal_node_data_t {
  webvtt_string annotation;
  webvtt_stringlist *css_classes;

  webvtt_uint alloc;
  webvtt_uint length;
  webvtt_node **children;
} webvtt_internal_node_data;

WEBVTT_EXPORT void webvtt_init_node( webvtt_node **node );
WEBVTT_EXPORT void webvtt_ref_node( webvtt_node *node );
WEBVTT_EXPORT void webvtt_release_node( webvtt_node **node );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
