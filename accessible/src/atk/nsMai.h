





#ifndef __NS_MAI_H__
#define __NS_MAI_H__

#include <atk/atk.h>
#include <glib.h>
#include <glib-object.h>

#include "AccessibleWrap.h"

#define MAI_TYPE_ATK_OBJECT             (mai_atk_object_get_type ())
#define MAI_ATK_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         MAI_TYPE_ATK_OBJECT, MaiAtkObject))
#define MAI_ATK_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                         MAI_TYPE_ATK_OBJECT, \
                                         MaiAtkObjectClass))
#define IS_MAI_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         MAI_TYPE_ATK_OBJECT))
#define IS_MAI_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                         MAI_TYPE_ATK_OBJECT))
#define MAI_ATK_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                         MAI_TYPE_ATK_OBJECT, \
                                         MaiAtkObjectClass))
GType mai_atk_object_get_type(void);
GType mai_util_get_type();
mozilla::a11y::AccessibleWrap* GetAccessibleWrap(AtkObject* aAtkObj);

#endif 
