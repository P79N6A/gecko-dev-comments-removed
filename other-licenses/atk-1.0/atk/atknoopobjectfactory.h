


















#ifndef __ATK_NO_OP_OBJECT_FACTORY_H__
#define __ATK_NO_OP_OBJECT_FACTORY_H__

#include <atk/atkobjectfactory.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATK_TYPE_NO_OP_OBJECT_FACTORY                (atk_no_op_object_factory_get_type ())
#define ATK_NO_OP_OBJECT_FACTORY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_NO_OP_OBJECT_FACTORY, AtkNoOpObjectFactory))
#define ATK_NO_OP_OBJECT_FACTORY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_NO_OP_OBJECT_FACTORY, AtkNoOpObjectFactoryClass))
#define ATK_IS_NO_OP_OBJECT_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_NO_OP_OBJECT_FACTORY))
#define ATK_IS_NO_OP_OBJECT_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_NO_OP_OBJECT_FACTORY))
#define ATK_NO_OP_OBJECT_FACTORY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ( (obj), ATK_TYPE_NO_OP_OBJECT_FACTORY, AtkNoOpObjectFactoryClass))

typedef struct _AtkNoOpObjectFactory                 AtkNoOpObjectFactory;
typedef struct _AtkNoOpObjectFactoryClass            AtkNoOpObjectFactoryClass;

struct _AtkNoOpObjectFactory
{
  AtkObjectFactory parent;
};

struct _AtkNoOpObjectFactoryClass
{
  AtkObjectFactoryClass parent_class;
};

GType atk_no_op_object_factory_get_type(void);

AtkObjectFactory *atk_no_op_object_factory_new(void);

#ifdef __cplusplus
}
#endif 


#endif
