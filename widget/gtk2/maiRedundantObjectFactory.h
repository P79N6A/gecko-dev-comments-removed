







































#ifndef __MAI_REDUNDANT_OBJECT_FACTORY_H__
#define __MAI_REDUNDANT_OBJECT_FACTORY_H__

G_BEGIN_DECLS

typedef struct _maiRedundantObjectFactory       maiRedundantObjectFactory;
typedef struct _maiRedundantObjectFactoryClass  maiRedundantObjectFactoryClass;

struct _maiRedundantObjectFactory
{
  AtkObjectFactory parent;
};

struct _maiRedundantObjectFactoryClass
{
  AtkObjectFactoryClass parent_class;
};

GType mai_redundant_object_factory_get_type();

AtkObjectFactory *mai_redundant_object_factory_new();

G_END_DECLS

#endif 
