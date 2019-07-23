


















#ifndef __ATK_VALUE_H__
#define __ATK_VALUE_H__

#include <atk/atkobject.h>

#ifdef __cplusplus
extern "C" {
#endif









#define ATK_TYPE_VALUE                    (atk_value_get_type ())
#define ATK_IS_VALUE(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_VALUE)
#define ATK_VALUE(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_VALUE, AtkValue)
#define ATK_VALUE_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_VALUE, AtkValueIface))

#ifndef _TYPEDEF_ATK_VALUE_
#define _TYPEDEF_ATK_VALUE__
typedef struct _AtkValue AtkValue;
#endif
typedef struct _AtkValueIface AtkValueIface;

struct _AtkValueIface
{
  GTypeInterface parent;

  void     (* get_current_value) (AtkValue     *obj,
                                  GValue       *value);
  void     (* get_maximum_value) (AtkValue     *obj,
                                  GValue       *value);
  void     (* get_minimum_value) (AtkValue     *obj,
                                  GValue       *value);
  gboolean (* set_current_value) (AtkValue     *obj,
                                  const GValue *value);
  void     (* get_minimum_increment) (AtkValue   *obj,
				      GValue     *value);
  AtkFunction pad1;
};

GType            atk_value_get_type (void);

void      atk_value_get_current_value (AtkValue     *obj,
                                       GValue       *value);


void     atk_value_get_maximum_value  (AtkValue     *obj,
                                       GValue       *value);

void     atk_value_get_minimum_value  (AtkValue     *obj,
                                       GValue       *value);

gboolean atk_value_set_current_value  (AtkValue     *obj,
                                       const GValue *value);

void     atk_value_get_minimum_increment  (AtkValue     *obj,
					   GValue       *value);








#ifdef __cplusplus
}
#endif 


#endif
