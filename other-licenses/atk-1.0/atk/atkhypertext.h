


















#ifndef __ATK_HYPERTEXT_H__
#define __ATK_HYPERTEXT_H__

#include <atk/atkobject.h>
#include <atk/atkhyperlink.h>

#ifdef __cplusplus
extern "C" {
#endif






#define ATK_TYPE_HYPERTEXT                    (atk_hypertext_get_type ())
#define ATK_IS_HYPERTEXT(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_HYPERTEXT)
#define ATK_HYPERTEXT(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_HYPERTEXT, AtkHypertext)
#define ATK_HYPERTEXT_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_HYPERTEXT, AtkHypertextIface))

#ifndef _TYPEDEF_ATK_HYPERTEXT_
#define _TYPEDEF_ATK_HYPERTEXT_
typedef struct _AtkHypertext AtkHypertext;
#endif
typedef struct _AtkHypertextIface AtkHypertextIface;

struct _AtkHypertextIface
{
  GTypeInterface parent;

  AtkHyperlink*(* get_link)                 (AtkHypertext       *hypertext,
                                             gint               link_index);
  gint         (* get_n_links)              (AtkHypertext       *hypertext);
  gint         (* get_link_index)           (AtkHypertext       *hypertext,
                                             gint               char_index);

  


  void         (* link_selected)            (AtkHypertext       *hypertext,
                                             gint               link_index);

  AtkFunction pad1;
  AtkFunction pad2;
  AtkFunction pad3;
};
GType atk_hypertext_get_type (void);

AtkHyperlink* atk_hypertext_get_link       (AtkHypertext *hypertext,
                                            gint          link_index);
gint          atk_hypertext_get_n_links    (AtkHypertext *hypertext);
gint          atk_hypertext_get_link_index (AtkHypertext *hypertext,
                                            gint          char_index);


#ifdef __cplusplus
}
#endif 


#endif
