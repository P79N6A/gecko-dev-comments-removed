







































#include <atk/atk.h>
#include "maiRedundantObjectFactory.h"

static void mai_redundant_object_factory_class_init (
                              maiRedundantObjectFactoryClass *klass);

static AtkObject* mai_redundant_object_factory_create_accessible (
                              GObject *obj);
static GType mai_redundant_object_factory_get_accessible_type (void);

static gpointer parent_class = NULL;

GType
mai_redundant_object_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
  {
    static const GTypeInfo tinfo =
    {
      sizeof (maiRedundantObjectFactoryClass),
      (GBaseInitFunc) NULL, 
      (GBaseFinalizeFunc) NULL, 
      (GClassInitFunc) mai_redundant_object_factory_class_init, 
      (GClassFinalizeFunc) NULL, 
      NULL, 
      sizeof (maiRedundantObjectFactory), 
      0, 
      (GInstanceInitFunc) NULL, 
      NULL 
    };
    type = g_type_register_static (
                           ATK_TYPE_OBJECT_FACTORY,
                           "MaiRedundantObjectFactory" , &tinfo, 0);
  }

  return type;
}

static void
mai_redundant_object_factory_class_init (maiRedundantObjectFactoryClass *klass)
{
  AtkObjectFactoryClass *class = ATK_OBJECT_FACTORY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  class->create_accessible = mai_redundant_object_factory_create_accessible;
  class->get_accessible_type = mai_redundant_object_factory_get_accessible_type;
}









AtkObjectFactory*
mai_redundant_object_factory_new ()
{
  GObject *factory;

  factory = g_object_new (mai_redundant_object_factory_get_type(), NULL);

  g_return_val_if_fail (factory != NULL, NULL);
  return ATK_OBJECT_FACTORY (factory);
}

static AtkObject*
mai_redundant_object_factory_create_accessible (GObject   *obj)
{
  AtkObject *accessible;

  g_return_val_if_fail (obj != NULL, NULL);

  accessible = g_object_new (ATK_TYPE_OBJECT, NULL);
  g_return_val_if_fail (accessible != NULL, NULL);

  accessible->role = ATK_ROLE_REDUNDANT_OBJECT;

  return accessible;
}

static GType
mai_redundant_object_factory_get_accessible_type ()
{
  return mai_redundant_object_factory_get_type();
}
