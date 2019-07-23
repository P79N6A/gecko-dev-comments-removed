


















#ifndef __ATK_REGISTRY_H__
#define __ATK_REGISTRY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib-object.h>
#include "atkobjectfactory.h"

#define ATK_TYPE_REGISTRY                (atk_registry_get_type ())
#define ATK_REGISTRY(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_REGISTRY, AtkRegistry))
#define ATK_REGISTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_REGISTRY, AtkRegistryClass))
#define ATK_IS_REGISTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_REGISTRY))
#define ATK_IS_REGISTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_REGISTRY))
#define ATK_REGISTRY_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_REGISTRY, AtkRegistryClass))

struct _AtkRegistry
{
  GObject    parent;
  GHashTable *factory_type_registry;
  GHashTable *factory_singleton_cache;
};

struct _AtkRegistryClass
{
  GObjectClass    parent_class;
};

typedef struct _AtkRegistry             AtkRegistry;
typedef struct _AtkRegistryClass        AtkRegistryClass;


GType             atk_registry_get_type         (void);
void              atk_registry_set_factory_type (AtkRegistry *registry,
                                                 GType type,
                                                 GType factory_type);
GType             atk_registry_get_factory_type (AtkRegistry *registry,
						 GType type);
AtkObjectFactory* atk_registry_get_factory      (AtkRegistry *registry,
                                                 GType type);

AtkRegistry*      atk_get_default_registry      (void);

#ifdef __cplusplus
}
#endif 

#endif

