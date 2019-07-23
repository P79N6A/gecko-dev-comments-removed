


















#ifndef __ATK_OBJECT_FACTORY_H__
#define __ATK_OBJECT_FACTORY_H__

#include <glib-object.h>
#include <atk/atkobject.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATK_TYPE_OBJECT_FACTORY                     (atk_object_factory_get_type ())
#define ATK_OBJECT_FACTORY(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_OBJECT_FACTORY, AtkObjectFactory))
#define ATK_OBJECT_FACTORY_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_OBJECT_FACTORY, AtkObjectFactoryClass))
#define ATK_IS_OBJECT_FACTORY(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_OBJECT_FACTORY))
#define ATK_IS_OBJECT_FACTORY_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_OBJECT_FACTORY))
#define ATK_OBJECT_FACTORY_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_OBJECT_FACTORY, AtkObjectFactoryClass))

typedef struct _AtkObjectFactory                AtkObjectFactory;
typedef struct _AtkObjectFactoryClass           AtkObjectFactoryClass;

struct _AtkObjectFactory
{
  GObject parent;
};

struct _AtkObjectFactoryClass
{
  GObjectClass parent_class;

  AtkObject* (* create_accessible) (GObject          *obj);
  void       (* invalidate)        (AtkObjectFactory *factory);
  GType      (* get_accessible_type)    (void);

  AtkFunction pad1;
  AtkFunction pad2;
};

GType atk_object_factory_get_type(void);

AtkObject* atk_object_factory_create_accessible (AtkObjectFactory *factory, GObject *obj);
void       atk_object_factory_invalidate (AtkObjectFactory *factory);
GType      atk_object_factory_get_accessible_type (AtkObjectFactory *factory);
#ifdef __cplusplus
}
#endif 


#endif

