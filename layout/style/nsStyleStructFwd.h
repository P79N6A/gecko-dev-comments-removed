









































#ifndef nsStyleStructFwd_h_
#define nsStyleStructFwd_h_

enum nsStyleStructID {








#define STYLE_STRUCT(name, checkdata_cb, ctor_args) eStyleStruct_##name,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

nsStyleStructID_Length 

};

struct nsStyleStruct;

#endif 
