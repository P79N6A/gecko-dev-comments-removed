


















#ifndef __ATK_HYPERLINK_IMPL_H__
#define __ATK_HYPERLINK_IMPL_H__

#include <atk/atkobject.h>
#include <atk/atkhyperlink.h>

#ifdef __cplusplus
extern "C" {
#endif


















#define ATK_TYPE_HYPERLINK_IMPL          (atk_hyperlink_impl_get_type ())
#define ATK_IS_HYPERLINK_IMPL(obj)       G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_HYPERLINK_IMPL)
#define ATK_HYPERLINK_IMPL(obj)             G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_HYPERLINK_IMPL, AtkHyperlinkImpl)
#define ATK_HYPERLINK_IMPL_GET_IFACE(obj)   G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_HYPERLINK_IMPL, AtkHyperlinkImplIface)

#ifndef _TYPEDEF_ATK_HYPERLINK_IMPL_
#define _TYPEDEF_ATK_HYPERLINK_IMPL__
typedef struct _AtkHyperlinkImpl AtkHyperlinkImpl;
#endif
typedef struct _AtkHyperlinkImplIface AtkHyperlinkImplIface;

struct _AtkHyperlinkImplIface
{
  GTypeInterface parent;
    
  AtkHyperlink*  (* get_hyperlink) (AtkHyperlinkImpl *impl);

  AtkFunction pad1;
};

GType            atk_hyperlink_impl_get_type (void);

AtkHyperlink    *atk_hyperlink_impl_get_hyperlink (AtkHyperlinkImpl *obj);

#ifdef __cplusplus
}
#endif 


#endif
