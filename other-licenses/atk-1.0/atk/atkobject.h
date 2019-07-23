


















#ifndef __ATK_OBJECT_H__
#define __ATK_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib-object.h>
#include <atk/atkstate.h>
#include <atk/atkrelationtype.h>






















































































































typedef enum
{
  ATK_ROLE_INVALID = 0, 
  ATK_ROLE_ACCEL_LABEL,
  ATK_ROLE_ALERT,
  ATK_ROLE_ANIMATION,
  ATK_ROLE_ARROW,
  ATK_ROLE_CALENDAR,
  ATK_ROLE_CANVAS,
  ATK_ROLE_CHECK_BOX,
  ATK_ROLE_CHECK_MENU_ITEM,
  ATK_ROLE_COLOR_CHOOSER,
  ATK_ROLE_COLUMN_HEADER,
  ATK_ROLE_COMBO_BOX,
  ATK_ROLE_DATE_EDITOR,
  ATK_ROLE_DESKTOP_ICON,
  ATK_ROLE_DESKTOP_FRAME,
  ATK_ROLE_DIAL,
  ATK_ROLE_DIALOG,
  ATK_ROLE_DIRECTORY_PANE,
  ATK_ROLE_DRAWING_AREA,
  ATK_ROLE_FILE_CHOOSER,
  ATK_ROLE_FILLER,
  ATK_ROLE_FONT_CHOOSER,
  ATK_ROLE_FRAME,
  ATK_ROLE_GLASS_PANE,
  ATK_ROLE_HTML_CONTAINER,
  ATK_ROLE_ICON,
  ATK_ROLE_IMAGE,
  ATK_ROLE_INTERNAL_FRAME,
  ATK_ROLE_LABEL,
  ATK_ROLE_LAYERED_PANE,
  ATK_ROLE_LIST,
  ATK_ROLE_LIST_ITEM,
  ATK_ROLE_MENU,
  ATK_ROLE_MENU_BAR,
  ATK_ROLE_MENU_ITEM,
  ATK_ROLE_OPTION_PANE,
  ATK_ROLE_PAGE_TAB,
  ATK_ROLE_PAGE_TAB_LIST,
  ATK_ROLE_PANEL,
  ATK_ROLE_PASSWORD_TEXT,
  ATK_ROLE_POPUP_MENU,
  ATK_ROLE_PROGRESS_BAR,
  ATK_ROLE_PUSH_BUTTON,
  ATK_ROLE_RADIO_BUTTON,
  ATK_ROLE_RADIO_MENU_ITEM,
  ATK_ROLE_ROOT_PANE,
  ATK_ROLE_ROW_HEADER,
  ATK_ROLE_SCROLL_BAR,
  ATK_ROLE_SCROLL_PANE,
  ATK_ROLE_SEPARATOR,
  ATK_ROLE_SLIDER,
  ATK_ROLE_SPLIT_PANE,
  ATK_ROLE_SPIN_BUTTON,
  ATK_ROLE_STATUSBAR,
  ATK_ROLE_TABLE,
  ATK_ROLE_TABLE_CELL,
  ATK_ROLE_TABLE_COLUMN_HEADER,
  ATK_ROLE_TABLE_ROW_HEADER,
  ATK_ROLE_TEAR_OFF_MENU_ITEM,
  ATK_ROLE_TERMINAL,
  ATK_ROLE_TEXT,
  ATK_ROLE_TOGGLE_BUTTON,
  ATK_ROLE_TOOL_BAR,
  ATK_ROLE_TOOL_TIP,
  ATK_ROLE_TREE,
  ATK_ROLE_TREE_TABLE,
  ATK_ROLE_UNKNOWN,
  ATK_ROLE_VIEWPORT,
  ATK_ROLE_WINDOW,
  ATK_ROLE_HEADER,
  ATK_ROLE_FOOTER,
  ATK_ROLE_PARAGRAPH,
  ATK_ROLE_RULER,
  ATK_ROLE_APPLICATION,
  ATK_ROLE_AUTOCOMPLETE,
  ATK_ROLE_EDITBAR,
  ATK_ROLE_EMBEDDED,
  ATK_ROLE_ENTRY,
  ATK_ROLE_CHART,
  ATK_ROLE_CAPTION,
  ATK_ROLE_DOCUMENT_FRAME,
  ATK_ROLE_HEADING,
  ATK_ROLE_PAGE,
  ATK_ROLE_SECTION,
  ATK_ROLE_REDUNDANT_OBJECT,
  ATK_ROLE_FORM,
  ATK_ROLE_LINK,
  ATK_ROLE_INPUT_METHOD_WINDOW,
  ATK_ROLE_LAST_DEFINED
} AtkRole;

AtkRole                  atk_role_register        (const gchar *name);


















typedef enum
{
  ATK_LAYER_INVALID,
  ATK_LAYER_BACKGROUND,
  ATK_LAYER_CANVAS,
  ATK_LAYER_WIDGET,
  ATK_LAYER_MDI,
  ATK_LAYER_POPUP,
  ATK_LAYER_OVERLAY,
  ATK_LAYER_WINDOW
} AtkLayer;








typedef GSList AtkAttributeSet;











typedef struct _AtkAttribute AtkAttribute;

struct _AtkAttribute {
  gchar* name;
  gchar* value;
};

#define ATK_TYPE_OBJECT                           (atk_object_get_type ())
#define ATK_OBJECT(obj)                           (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_OBJECT, AtkObject))
#define ATK_OBJECT_CLASS(klass)                   (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_OBJECT, AtkObjectClass))
#define ATK_IS_OBJECT(obj)                        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_OBJECT))
#define ATK_IS_OBJECT_CLASS(klass)                (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_OBJECT))
#define ATK_OBJECT_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_OBJECT, AtkObjectClass))

#define ATK_TYPE_IMPLEMENTOR                      (atk_implementor_get_type ())
#define ATK_IS_IMPLEMENTOR(obj)                   G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_IMPLEMENTOR)
#define ATK_IMPLEMENTOR(obj)                      G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_IMPLEMENTOR, AtkImplementor)
#define ATK_IMPLEMENTOR_GET_IFACE(obj)            (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_IMPLEMENTOR, AtkImplementorIface))


typedef struct _AtkImplementor            AtkImplementor; 
typedef struct _AtkImplementorIface       AtkImplementorIface;


typedef struct _AtkObject                 AtkObject;
typedef struct _AtkObjectClass            AtkObjectClass;
typedef struct _AtkRelationSet            AtkRelationSet;
typedef struct _AtkStateSet               AtkStateSet;



















struct _AtkPropertyValues
{
  const gchar  *property_name;
  GValue old_value;
  GValue new_value;
};

typedef struct _AtkPropertyValues        AtkPropertyValues;

typedef gboolean (*AtkFunction)          (gpointer data); 












typedef void (*AtkPropertyChangeHandler) (AtkObject*, AtkPropertyValues*);


struct _AtkObject
{
  GObject parent;

  gchar *description;
  gchar *name;
  AtkObject *accessible_parent;
  AtkRole role;
  AtkRelationSet *relation_set;
  AtkLayer layer;
};

struct _AtkObjectClass
{
  GObjectClass parent;

  


  G_CONST_RETURN gchar*    (* get_name)            (AtkObject                *accessible);
  


  G_CONST_RETURN gchar*    (* get_description)     (AtkObject                *accessible);
  


  AtkObject*               (*get_parent)           (AtkObject                *accessible);

  


  gint                    (* get_n_children)       (AtkObject                *accessible);
  




  AtkObject*              (* ref_child)            (AtkObject                *accessible,
                                                    gint                      i);
  



  gint                    (* get_index_in_parent) (AtkObject                 *accessible);
  


  AtkRelationSet*         (* ref_relation_set)    (AtkObject                 *accessible);
  


  AtkRole                 (* get_role)            (AtkObject                 *accessible);
  AtkLayer                (* get_layer)           (AtkObject                 *accessible);
  gint                    (* get_mdi_zorder)      (AtkObject                 *accessible);
  


  AtkStateSet*            (* ref_state_set)       (AtkObject                 *accessible);
  


  void                    (* set_name)            (AtkObject                 *accessible,
                                                   const gchar               *name);
  


  void                    (* set_description)     (AtkObject                 *accessible,
                                                   const gchar               *description);
  


  void                    (* set_parent)          (AtkObject                 *accessible,
                                                   AtkObject                 *parent);
  


  void                    (* set_role)            (AtkObject                 *accessible,
                                                   AtkRole                   role);
  


guint                     (* connect_property_change_handler)    (AtkObject
                 *accessible,
                                                                  AtkPropertyChangeHandler       *handler);
  



void                      (* remove_property_change_handler)     (AtkObject
                *accessible,
                                                                  guint
                handler_id);
void                      (* initialize)                         (AtkObject                     *accessible,
                                                                  gpointer                      data);
  



  void                    (* children_changed)    (AtkObject                  *accessible,
                                                   guint                      change_index,
                                                   gpointer                   changed_child);
  



  void                    (* focus_event)         (AtkObject                  *accessible,
                                                   gboolean                   focus_in);
  



  void                    (* property_change)     (AtkObject                  *accessible,
                                                   AtkPropertyValues          *values);
  



  void                    (* state_change)        (AtkObject                  *accessible,
                                                   const gchar                *name,
                                                   gboolean                   state_set);
  



  void                    (*visible_data_changed) (AtkObject                  *accessible);

  





  void                    (*active_descendant_changed) (AtkObject                  *accessible,
                                                        gpointer                   *child);

  



  AtkAttributeSet* 	  (*get_attributes)            (AtkObject                  *accessible);
  AtkFunction             pad1;
  AtkFunction             pad2;
};

GType            atk_object_get_type   (void);

struct _AtkImplementorIface
{
  GTypeInterface parent;

  AtkObject*   (*ref_accessible) (AtkImplementor *implementor);
};
GType atk_implementor_get_type (void);













AtkObject*              atk_implementor_ref_accessible            (AtkImplementor *implementor);





G_CONST_RETURN gchar*   atk_object_get_name                       (AtkObject *accessible);
G_CONST_RETURN gchar*   atk_object_get_description                (AtkObject *accessible);
AtkObject*              atk_object_get_parent                     (AtkObject *accessible);
gint                    atk_object_get_n_accessible_children      (AtkObject *accessible);
AtkObject*              atk_object_ref_accessible_child           (AtkObject *accessible,
                                                                   gint        i);
AtkRelationSet*         atk_object_ref_relation_set               (AtkObject *accessible);
AtkRole                 atk_object_get_role                       (AtkObject *accessible);
AtkLayer                atk_object_get_layer                      (AtkObject *accessible);
gint                    atk_object_get_mdi_zorder                 (AtkObject *accessible);
AtkAttributeSet*        atk_object_get_attributes                 (AtkObject *accessible);
AtkStateSet*            atk_object_ref_state_set                  (AtkObject *accessible);
gint                    atk_object_get_index_in_parent            (AtkObject *accessible);
void                    atk_object_set_name                       (AtkObject *accessible,
                                                                   const gchar *name);
void                    atk_object_set_description                (AtkObject *accessible,
                                                                   const gchar *description);
void                    atk_object_set_parent                     (AtkObject *accessible,
                                                                   AtkObject *parent);
void                    atk_object_set_role                       (AtkObject *accessible,
                                                                   AtkRole   role);


guint                atk_object_connect_property_change_handler  (AtkObject                      *accessible,
                                                                  AtkPropertyChangeHandler       *handler);
void                 atk_object_remove_property_change_handler   (AtkObject                      *accessible,
                                                                  guint                          handler_id);

void                 atk_object_notify_state_change              (AtkObject                      *accessible,
                                                                  AtkState                       state,
                                                                  gboolean                       value);
void                 atk_object_initialize                       (AtkObject                     *accessible,
                                                                  gpointer                      data);
                                    
G_CONST_RETURN gchar* atk_role_get_name      (AtkRole         role);
AtkRole               atk_role_for_name      (const gchar     *name);



gboolean              atk_object_add_relationship              (AtkObject      *object,
								AtkRelationType relationship,
								AtkObject      *target);
gboolean              atk_object_remove_relationship           (AtkObject      *object,
								AtkRelationType relationship,
								AtkObject      *target);
G_CONST_RETURN gchar* atk_role_get_localized_name              (AtkRole     role);





























































#ifdef __cplusplus
}
#endif 


#endif
