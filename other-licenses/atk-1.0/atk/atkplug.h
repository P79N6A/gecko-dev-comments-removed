


















#if defined(ATK_DISABLE_SINGLE_INCLUDES) && !defined (__ATK_H_INSIDE__) && !defined (ATK_COMPILATION)
#error "Only <atk/atk.h> can be included directly."
#endif

#ifndef __ATK_PLUG_H__
#define __ATK_PLUG_H__

G_BEGIN_DECLS

#define ATK_TYPE_PLUG               (atk_plug_get_type ())
#define ATK_PLUG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_PLUG, AtkPlug))
#define ATK_IS_PLUG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_PLUG))
#define ATK_PLUG_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), ATK_TYPE_PLUG, AtkPlugClass))
#define ATK_IS_PLUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), ATK_TYPE_PLUG))
#define ATK_PLUG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), ATK_TYPE_PLUG, AtkPlugClass))

typedef struct _AtkPlug         AtkPlug;
typedef struct _AtkPlugClass    AtkPlugClass;

struct _AtkPlug
{
  AtkObject parent;
};

GType atk_plug_get_type (void);

struct _AtkPlugClass
{
  AtkObjectClass parent_class;
  
  

  
  gchar* (* get_object_id) (AtkPlug* obj);
};

AtkObject*    atk_plug_new     (void);
gchar*        atk_plug_get_id  (AtkPlug* plug);

G_END_DECLS

#endif 
