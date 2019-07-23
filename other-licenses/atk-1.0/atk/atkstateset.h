


















#ifndef __ATK_STATE_SET_H__
#define __ATK_STATE_SET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib-object.h>
#include <atk/atkobject.h>
#include <atk/atkstate.h>

#define ATK_TYPE_STATE_SET                        (atk_state_set_get_type ())
#define ATK_STATE_SET(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_STATE_SET, AtkStateSet))
#define ATK_STATE_SET_CLASS(klass)                (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_STATE_SET, AtkStateSetClass))
#define ATK_IS_STATE_SET(obj)                     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_STATE_SET))
#define ATK_IS_STATE_SET_CLASS(klass)             (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_STATE_SET))
#define ATK_STATE_SET_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_STATE_SET, AtkStateSetClass))

typedef struct _AtkStateSetClass       AtkStateSetClass;


struct _AtkStateSet
{
  GObject parent;

};

struct _AtkStateSetClass
{
  GObjectClass parent;
};

GType atk_state_set_get_type (void);

AtkStateSet*    atk_state_set_new               (void);
gboolean        atk_state_set_is_empty          (AtkStateSet  *set);
gboolean        atk_state_set_add_state         (AtkStateSet  *set,
                                                 AtkStateType type);
void            atk_state_set_add_states        (AtkStateSet  *set,
                                                 AtkStateType *types,
                                                 gint         n_types);
void            atk_state_set_clear_states      (AtkStateSet  *set);
gboolean        atk_state_set_contains_state    (AtkStateSet  *set,
                                                 AtkStateType type);
gboolean        atk_state_set_contains_states   (AtkStateSet  *set,
                                                 AtkStateType *types,
                                                 gint         n_types);
gboolean        atk_state_set_remove_state      (AtkStateSet  *set,
                                                 AtkStateType type);
AtkStateSet*    atk_state_set_and_sets          (AtkStateSet  *set,
                                                 AtkStateSet  *compare_set);
AtkStateSet*    atk_state_set_or_sets           (AtkStateSet  *set,
                                                 AtkStateSet  *compare_set);
AtkStateSet*    atk_state_set_xor_sets          (AtkStateSet  *set,
                                                 AtkStateSet  *compare_set);

#ifdef __cplusplus
}
#endif 


#endif
