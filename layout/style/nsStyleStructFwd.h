









































#ifndef nsStyleStructFwd_h_
#define nsStyleStructFwd_h_

enum nsStyleStructID {








#define STYLE_STRUCT(name, checkdata_cb, ctor_args) eStyleStruct_##name,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT


nsStyleStructID_Length,





eStyleStruct_BackendOnly = nsStyleStructID_Length

};


#define NS_STYLE_INHERIT_BIT(sid_)        (1 << PRInt32(eStyleStruct_##sid_))

#endif 
