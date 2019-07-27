





#ifndef xpcquickstubs_h___
#define xpcquickstubs_h___

#include "XPCForwards.h"



nsresult
xpc_qsUnwrapArgImpl(JSContext *cx, JS::HandleObject src, const nsIID &iid, void **ppArg,
                    nsISupports **ppArgRef, JS::MutableHandleValue vp);

#endif 
