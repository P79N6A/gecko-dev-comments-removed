








#ifndef nsStyleStructFwd_h_
#define nsStyleStructFwd_h_

enum nsStyleStructID {












nsStyleStructID_Inherited_Start = 0,

nsStyleStructID_DUMMY1 = nsStyleStructID_Inherited_Start - 1,

#define STYLE_STRUCT_INHERITED(name, checkdata_cb) \
  eStyleStruct_##name,
#define STYLE_STRUCT_RESET(name, checkdata_cb)
#include "nsStyleStructList.h"
#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

nsStyleStructID_Reset_Start,

nsStyleStructID_DUMMY2 = nsStyleStructID_Reset_Start - 1,

#define STYLE_STRUCT_RESET(name, checkdata_cb) \
  eStyleStruct_##name,
#define STYLE_STRUCT_INHERITED(name, checkdata_cb)
#include "nsStyleStructList.h"
#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET


nsStyleStructID_Length,

nsStyleStructID_Inherited_Count =
  nsStyleStructID_Reset_Start - nsStyleStructID_Inherited_Start,
nsStyleStructID_Reset_Count =
  nsStyleStructID_Length - nsStyleStructID_Reset_Start,





eStyleStruct_BackendOnly = nsStyleStructID_Length

};


#define NS_STYLE_INHERIT_BIT(sid_)        (1 << int32_t(eStyleStruct_##sid_))

#endif 
