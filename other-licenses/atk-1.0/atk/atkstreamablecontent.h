


















#ifndef __ATK_STREAMABLE_CONTENT_H__
#define __ATK_STREAMABLE_CONTENT_H__

#include <atk/atkobject.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATK_TYPE_STREAMABLE_CONTENT           (atk_streamable_content_get_type ())
#define ATK_IS_STREAMABLE_CONTENT(obj)        G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_STREAMABLE_CONTENT)
#define ATK_STREAMABLE_CONTENT(obj)           G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_STREAMABLE_CONTENT, AtkStreamableContent)
#define ATK_STREAMABLE_CONTENT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_STREAMABLE_CONTENT, AtkStreamableContentIface))

#ifndef _TYPEDEF_ATK_STREAMABLE_CONTENT
#define _TYPEDEF_ATK_STREAMABLE_CONTENT
typedef struct _AtkStreamableContent AtkStreamableContent;
#endif
typedef struct _AtkStreamableContentIface AtkStreamableContentIface;

struct _AtkStreamableContentIface
{
  GTypeInterface parent;

  


  gint                      (* get_n_mime_types)  (AtkStreamableContent     *streamable);
  










  G_CONST_RETURN gchar*     (* get_mime_type)     (AtkStreamableContent     *streamable,
                                                   gint                     i);
  





  GIOChannel*               (* get_stream)        (AtkStreamableContent     *streamable,
                                                   const gchar              *mime_type);












    G_CONST_RETURN  gchar*  (* get_uri)           (AtkStreamableContent     *streamable,
                                                   const gchar              *mime_type);


  AtkFunction               pad1;
  AtkFunction               pad2;
  AtkFunction               pad3;
};
GType                  atk_streamable_content_get_type (void);

gint                   atk_streamable_content_get_n_mime_types (AtkStreamableContent     *streamable);
                                                       
G_CONST_RETURN gchar*  atk_streamable_content_get_mime_type    (AtkStreamableContent     *streamable,
                                                                gint                     i);
GIOChannel*             atk_streamable_content_get_stream       (AtkStreamableContent     *streamable,
                                                                 const gchar              *mime_type);

gchar*                  atk_streamable_content_get_uri          (AtkStreamableContent     *streamable,
                                                                 const gchar              *mime_type);


#ifdef __cplusplus
}
#endif 


#endif
