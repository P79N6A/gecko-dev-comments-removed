


















#ifndef __ATK_SELECTION_H__
#define __ATK_SELECTION_H__

#include <atk/atkobject.h>

#ifdef __cplusplus
extern "C" {
#endif








#define ATK_TYPE_SELECTION                        (atk_selection_get_type ())
#define ATK_IS_SELECTION(obj)                     G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_SELECTION)
#define ATK_SELECTION(obj)                        G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_SELECTION, AtkSelection)
#define ATK_SELECTION_GET_IFACE(obj)              (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_SELECTION, AtkSelectionIface))

#ifndef _TYPEDEF_ATK_SELECTION_
#define _TYPEDEF_ATK_SELECTION_
typedef struct _AtkSelection AtkSelection;
#endif
typedef struct _AtkSelectionIface AtkSelectionIface;

struct _AtkSelectionIface
{
  GTypeInterface parent;

  gboolean     (* add_selection)        (AtkSelection   *selection,
                                         gint           i);
  gboolean     (* clear_selection)      (AtkSelection   *selection);
  AtkObject*   (* ref_selection)        (AtkSelection   *selection,
                                         gint           i);
  gint         (* get_selection_count)  (AtkSelection   *selection);
  gboolean     (* is_child_selected)    (AtkSelection   *selection,
                                         gint           i);
  gboolean     (* remove_selection)     (AtkSelection   *selection,
                                         gint           i);
  gboolean     (* select_all_selection) (AtkSelection   *selection);

  
  
  void         (*selection_changed)     (AtkSelection   *selection);

  AtkFunction  pad1;
  AtkFunction  pad2;
};

GType atk_selection_get_type (void);

gboolean     atk_selection_add_selection        (AtkSelection   *selection,
                                                 gint           i);

gboolean     atk_selection_clear_selection      (AtkSelection   *selection);

AtkObject*   atk_selection_ref_selection        (AtkSelection   *selection,
                                                 gint           i);

gint         atk_selection_get_selection_count  (AtkSelection   *selection);

gboolean     atk_selection_is_child_selected    (AtkSelection   *selection,
                                                 gint           i);

gboolean     atk_selection_remove_selection     (AtkSelection   *selection,
                                                 gint           i);

gboolean     atk_selection_select_all_selection (AtkSelection   *selection);

#ifdef __cplusplus
}
#endif 


#endif
