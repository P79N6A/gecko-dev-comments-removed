


















#ifndef __ATK_COMPONENT_H__
#define __ATK_COMPONENT_H__

#include <atk/atkobject.h>
#include <atk/atkutil.h>

#ifdef __cplusplus
extern "C" {
#endif








#define ATK_TYPE_COMPONENT                    (atk_component_get_type ())
#define ATK_IS_COMPONENT(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_COMPONENT)
#define ATK_COMPONENT(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_COMPONENT, AtkComponent)
#define ATK_COMPONENT_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_COMPONENT, AtkComponentIface))

#ifndef _TYPEDEF_ATK_COMPONENT_
#define _TYPEDEF_ATK_COMPONENT_
typedef struct _AtkComponent AtkComponent;
#endif
typedef struct _AtkComponentIface  AtkComponentIface;

typedef void (*AtkFocusHandler) (AtkObject*, gboolean);

typedef struct _AtkRectangle       AtkRectangle;

struct _AtkRectangle
{
  gint x;
  gint y;
  gint width;
  gint height;
};

GType atk_rectangle_get_type (void);

#define ATK_TYPE_RECTANGLE (atk_rectangle_get_type ())
struct _AtkComponentIface
{
  GTypeInterface parent;

  guint          (* add_focus_handler)  (AtkComponent          *component,
                                         AtkFocusHandler        handler);

  gboolean       (* contains)           (AtkComponent          *component,
                                         gint                   x,
                                         gint                   y,
                                         AtkCoordType           coord_type);

  AtkObject*    (* ref_accessible_at_point)  (AtkComponent     *component,
                                         gint                   x,
                                         gint                   y,
                                         AtkCoordType           coord_type);
  void          (* get_extents)         (AtkComponent          *component,
                                         gint                  *x,
                                         gint                  *y,
                                         gint                  *width,
                                         gint                  *height,
                                         AtkCoordType          coord_type);
  void                     (* get_position)     (AtkComponent   *component,
                                                 gint           *x,
                                                 gint           *y,
                                                 AtkCoordType   coord_type);
  void                     (* get_size)                 (AtkComponent   *component,
                                                         gint           *width,
                                                         gint           *height);
  gboolean                 (* grab_focus)               (AtkComponent   *component);
  void                     (* remove_focus_handler)      (AtkComponent  *component,
                                                          guint         handler_id);
  gboolean                 (* set_extents)      (AtkComponent   *component,
                                                 gint           x,
                                                 gint           y,
                                                 gint           width,
                                                 gint           height,
                                                 AtkCoordType   coord_type);
  gboolean                 (* set_position)     (AtkComponent   *component,
                                                 gint           x,
                                                 gint           y,
                                                 AtkCoordType   coord_type);
  gboolean                 (* set_size)         (AtkComponent   *component,
                                                 gint           width,
                                                 gint           height);
  	
  AtkLayer                 (* get_layer)        (AtkComponent   *component);
  gint                     (* get_mdi_zorder)   (AtkComponent   *component);

  


  void                     (* bounds_changed)   (AtkComponent   *component,
                                                 AtkRectangle   *bounds);
  gdouble                  (* get_alpha)        (AtkComponent   *component);
};

GType atk_component_get_type (void);



guint                atk_component_add_focus_handler      (AtkComponent    *component,
                                                           AtkFocusHandler handler);
gboolean              atk_component_contains               (AtkComponent    *component,
                                                            gint            x,
                                                            gint            y,
                                                            AtkCoordType    coord_type);
AtkObject*            atk_component_ref_accessible_at_point(AtkComponent    *component,
                                                            gint            x,
                                                            gint            y,
                                                            AtkCoordType    coord_type);
void                  atk_component_get_extents            (AtkComponent    *component,
                                                            gint            *x,
                                                            gint            *y,
                                                            gint            *width,
                                                            gint            *height,
                                                            AtkCoordType    coord_type);
void                  atk_component_get_position           (AtkComponent    *component,
                                                            gint            *x,
                                                            gint            *y,
                                                            AtkCoordType    coord_type);
void                  atk_component_get_size               (AtkComponent    *component,
                                                            gint            *width,
                                                            gint            *height);
AtkLayer              atk_component_get_layer              (AtkComponent    *component);
gint                  atk_component_get_mdi_zorder         (AtkComponent    *component);
gboolean              atk_component_grab_focus             (AtkComponent    *component);
void                  atk_component_remove_focus_handler   (AtkComponent    *component,
                                                            guint           handler_id);
gboolean              atk_component_set_extents            (AtkComponent    *component,
                                                            gint            x,
                                                            gint            y,
                                                            gint            width,
                                                            gint            height,
                                                            AtkCoordType    coord_type);
gboolean              atk_component_set_position           (AtkComponent    *component,
                                                            gint            x,
                                                            gint            y,
                                                            AtkCoordType    coord_type);
gboolean              atk_component_set_size               (AtkComponent    *component,
                                                            gint            width,
                                                            gint            height);
gdouble               atk_component_get_alpha              (AtkComponent    *component);
#ifdef __cplusplus
}
#endif 


#endif
