


















#ifndef __ATK_NO_OP_OBJECT_H__
#define __ATK_NO_OP_OBJECT_H__

G_BEGIN_DECLS

#define ATK_TYPE_NO_OP_OBJECT                (atk_no_op_object_get_type ())
#define ATK_NO_OP_OBJECT(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_NO_OP_OBJECT, AtkNoOpObject))
#define ATK_NO_OP_OBJECT_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_NO_OP_OBJECT, AtkNoOpObjectClass))
#define ATK_IS_NO_OP_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_NO_OP_OBJECT))
#define ATK_IS_NO_OP_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_NO_OP_OBJECT))
#define ATK_NO_OP_OBJECT_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_NO_OP_OBJECT, AtkNoOpObjectClass))

typedef struct _AtkNoOpObject                   AtkNoOpObject;
typedef struct _AtkNoOpObjectClass              AtkNoOpObjectClass;

struct _AtkNoOpObject
{
  AtkObject     parent;
};

GType atk_no_op_object_get_type (void);

struct _AtkNoOpObjectClass
{
  AtkObjectClass parent_class;
};

AtkObject *atk_no_op_object_new (GObject  *obj);

G_END_DECLS

#endif 
