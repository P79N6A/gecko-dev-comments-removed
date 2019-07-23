


















#ifndef __ATK_ACTION_H__
#define __ATK_ACTION_H__

#include <atk/atkobject.h>

#ifdef __cplusplus
extern "C" {
#endif










#define ATK_TYPE_ACTION                    (atk_action_get_type ())
#define ATK_IS_ACTION(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_ACTION)
#define ATK_ACTION(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_ACTION, AtkAction)
#define ATK_ACTION_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_ACTION, AtkActionIface))

#ifndef _TYPEDEF_ATK_ACTION_
#define _TYPEDEF_ATK_ACTION_
typedef struct _AtkAction AtkAction;
#endif
typedef struct _AtkActionIface AtkActionIface;

struct _AtkActionIface
{
  GTypeInterface parent;

  gboolean                (*do_action)         (AtkAction         *action,
                                                gint              i);
  gint                    (*get_n_actions)     (AtkAction         *action);
  G_CONST_RETURN gchar*   (*get_description)   (AtkAction         *action,
                                                gint              i);
  G_CONST_RETURN gchar*   (*get_name)          (AtkAction         *action,
                                                gint              i);
  G_CONST_RETURN gchar*   (*get_keybinding)    (AtkAction         *action,
                                                gint              i);
  gboolean                (*set_description)   (AtkAction         *action,
                                                gint              i,
                                                const gchar       *desc);
  G_CONST_RETURN gchar*   (*get_localized_name)(AtkAction         *action,
						gint              i);
  AtkFunction             pad2;
};

GType atk_action_get_type (void);












gboolean   atk_action_do_action                (AtkAction         *action,
                                            gint              i);
gint   atk_action_get_n_actions            (AtkAction *action);
G_CONST_RETURN gchar* atk_action_get_description  (AtkAction         *action,
                                                   gint              i);
G_CONST_RETURN gchar* atk_action_get_name         (AtkAction         *action,
                                                   gint              i);
G_CONST_RETURN gchar* atk_action_get_keybinding   (AtkAction         *action,
                                                   gint              i);
gboolean              atk_action_set_description  (AtkAction         *action,
                                                   gint              i,
                                                   const gchar       *desc);



G_CONST_RETURN gchar* atk_action_get_localized_name (AtkAction       *action,
						     gint            i);







#ifdef __cplusplus
}
#endif 


#endif
