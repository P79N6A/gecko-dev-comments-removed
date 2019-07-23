


















#ifndef __ATK_RELATION_H__
#define __ATK_RELATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib-object.h>
#include <atk/atkrelationtype.h>







#define ATK_TYPE_RELATION                         (atk_relation_get_type ())
#define ATK_RELATION(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_RELATION, AtkRelation))
#define ATK_RELATION_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_RELATION, AtkRelationClass))
#define ATK_IS_RELATION(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_RELATION))
#define ATK_IS_RELATION_CLASS(klass)              (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_RELATION))
#define ATK_RELATION_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_RELATION, AtkRelationClass))

typedef struct _AtkRelation            AtkRelation;
typedef struct _AtkRelationClass       AtkRelationClass;

struct _AtkRelation
{
  GObject parent;

  GPtrArray       *target;
  AtkRelationType relationship;
};

struct _AtkRelationClass
{
  GObjectClass parent;
};

GType atk_relation_get_type (void);

AtkRelationType       atk_relation_type_register      (const gchar     *name);
G_CONST_RETURN gchar* atk_relation_type_get_name      (AtkRelationType type);
AtkRelationType       atk_relation_type_for_name      (const gchar     *name);





AtkRelation*          atk_relation_new                (AtkObject       **targets,
                                                       gint            n_targets,
                                                       AtkRelationType relationship);



AtkRelationType       atk_relation_get_relation_type  (AtkRelation     *relation);



GPtrArray*            atk_relation_get_target         (AtkRelation     *relation);
void                  atk_relation_add_target         (AtkRelation     *relation,
                                                       AtkObject       *target);
                

#ifdef __cplusplus
}
#endif 

#endif
