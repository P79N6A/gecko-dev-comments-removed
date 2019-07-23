


















#ifndef __ATK_RELATION_SET_H__
#define __ATK_RELATION_SET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib-object.h>
#include <atk/atkobject.h>
#include <atk/atkrelation.h>

#define ATK_TYPE_RELATION_SET                     (atk_relation_set_get_type ())
#define ATK_RELATION_SET(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_RELATION_SET, AtkRelationSet))
#define ATK_RELATION_SET_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_RELATION_SET, AtkRelationSetClass))
#define ATK_IS_RELATION_SET(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_RELATION_SET))
#define ATK_IS_RELATION_SET_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_RELATION_SET))
#define ATK_RELATION_SET_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_RELATION_SET, AtkRelationSetClass))

typedef struct _AtkRelationSetClass       AtkRelationSetClass;


struct _AtkRelationSet
{
  GObject parent;

  GPtrArray *relations;
};

struct _AtkRelationSetClass
{
  GObjectClass parent;

  AtkFunction pad1;
  AtkFunction pad2;
};

GType atk_relation_set_get_type (void);

AtkRelationSet* atk_relation_set_new                  (void);
gboolean        atk_relation_set_contains             (AtkRelationSet  *set,
                                                       AtkRelationType relationship);
void            atk_relation_set_remove               (AtkRelationSet  *set,
                                                       AtkRelation     *relation);
void            atk_relation_set_add                  (AtkRelationSet  *set,
                                                       AtkRelation     *relation);
gint            atk_relation_set_get_n_relations      (AtkRelationSet  *set);
AtkRelation*    atk_relation_set_get_relation         (AtkRelationSet  *set,
                                                       gint            i);
AtkRelation*    atk_relation_set_get_relation_by_type (AtkRelationSet  *set,
                                                       AtkRelationType relationship);
void            atk_relation_set_add_relation_by_type (AtkRelationSet  *set,
                                                       AtkRelationType relationship,
                                                       AtkObject       *target);

#ifdef __cplusplus
}
#endif 


#endif
