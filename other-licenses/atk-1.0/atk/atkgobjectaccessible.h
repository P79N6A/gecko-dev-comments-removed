


















#ifndef __ATK_GOBJECT_ACCESSIBLE_H__
#define __ATK_GOBJECT_ACCESSIBLE_H__

#include <atk/atk.h>


#ifdef __cplusplus
extern "C" {
#endif





#define ATK_TYPE_GOBJECT_ACCESSIBLE            (atk_gobject_accessible_get_type ())
#define ATK_GOBJECT_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_GOBJECT_ACCESSIBLE, AtkGObjectAccessible))
#define ATK_GOBJECT_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_GOBJECT_ACCESSIBLE, AtkGObjectAccessibleClass))
#define ATK_IS_GOBJECT_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_GOBJECT_ACCESSIBLE))
#define ATK_IS_GOBJECT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_GOBJECT_ACCESSIBLE))
#define ATK_GOBJECT_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_GOBJECT_ACCESSIBLE, AtkGObjectAccessibleClass))

typedef struct _AtkGObjectAccessible                AtkGObjectAccessible;
typedef struct _AtkGObjectAccessibleClass           AtkGObjectAccessibleClass;

struct _AtkGObjectAccessible
{
  AtkObject parent;
};

GType atk_gobject_accessible_get_type (void);

struct _AtkGObjectAccessibleClass
{
  AtkObjectClass parent_class;

  AtkFunction pad1;
  AtkFunction pad2;
};

AtkObject *atk_gobject_accessible_for_object      (GObject           *obj);
GObject   *atk_gobject_accessible_get_object      (AtkGObjectAccessible *obj);

#ifdef __cplusplus
}
#endif 


#endif
