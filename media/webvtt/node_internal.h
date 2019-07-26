

























 
#ifndef __WEBVTT_NODE_INTERNAL_H__
# define __WEBVTT_NODE_INTERNAL_H__
# include <webvtt/node.h>




WEBVTT_INTERN webvtt_status webvtt_create_node( webvtt_node **node, webvtt_node_kind kind, webvtt_node *parent );
WEBVTT_INTERN webvtt_status webvtt_create_internal_node( webvtt_node **node, webvtt_node *parent, webvtt_node_kind kind, webvtt_stringlist *css_classes, webvtt_string *annotation );




WEBVTT_INTERN webvtt_status webvtt_create_head_node( webvtt_node **node );
WEBVTT_INTERN webvtt_status webvtt_create_timestamp_node( webvtt_node **node, webvtt_node *parent, webvtt_timestamp time_stamp );
WEBVTT_INTERN webvtt_status webvtt_create_text_node( webvtt_node **node, webvtt_node *parent, webvtt_string *text );




WEBVTT_INTERN webvtt_status webvtt_attach_node( webvtt_node *parent, webvtt_node *to_attach );

#endif
