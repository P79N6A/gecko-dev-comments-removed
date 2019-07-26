


























#ifndef __INTERN_CUE_H__
# define __INTERN_CUE_H__
# include <webvtt/string.h>
# include <webvtt/cue.h>




WEBVTT_INTERN webvtt_status webvtt_create_node( webvtt_node **node, webvtt_node_kind kind, webvtt_node *parent );
WEBVTT_INTERN webvtt_status webvtt_create_internal_node( webvtt_node **node, webvtt_node *parent, webvtt_node_kind kind, webvtt_stringlist *css_classes, webvtt_string *annotation );




WEBVTT_INTERN webvtt_status webvtt_create_head_node( webvtt_node **node );
WEBVTT_INTERN webvtt_status webvtt_create_time_stamp_leaf_node( webvtt_node **node, webvtt_node *parent, webvtt_timestamp time_stamp );
WEBVTT_INTERN webvtt_status webvtt_create_text_leaf_node( webvtt_node **node, webvtt_node *parent, webvtt_string *text );




WEBVTT_INTERN webvtt_status webvtt_attach_internal_node( webvtt_node *parent, webvtt_node *to_attach );




enum {
  CUE_HAVE_VERTICAL = (1 << 0),
  CUE_HAVE_SIZE = (1 << 1),
  CUE_HAVE_POSITION = (1 << 2),
  CUE_HAVE_LINE = (1 << 3),
  CUE_HAVE_ALIGN = (1 << 4),

  CUE_HAVE_SETTINGS = (CUE_HAVE_VERTICAL | CUE_HAVE_SIZE
    | CUE_HAVE_POSITION | CUE_HAVE_LINE | CUE_HAVE_ALIGN),

  CUE_HAVE_CUEPARAMS = 0x40000000,
  CUE_HAVE_ID = 0x80000000,
};

#endif
