





#ifndef __ZTRANS_H
#define __ZTRANS_H






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#ifndef UCNV_H





struct ZTrans;
typedef struct ZTrans ZTrans;

#endif









U_CAPI ZTrans* U_EXPORT2
ztrans_open(UDate time, const void* from, const void* to);




U_CAPI ZTrans* U_EXPORT2
ztrans_openEmpty();






U_CAPI void U_EXPORT2
ztrans_close(ZTrans *trans);






U_CAPI ZTrans* U_EXPORT2
ztrans_clone(ZTrans *trans);








U_CAPI UBool U_EXPORT2
ztrans_equals(const ZTrans* trans1, const ZTrans* trans2);






U_CAPI UDate U_EXPORT2
ztrans_getTime(ZTrans* trans);






U_CAPI void U_EXPORT2
ztrans_setTime(ZTrans* trans, UDate time);






U_CAPI void* U_EXPORT2
ztrans_getFrom(ZTrans* & trans);








U_CAPI void U_EXPORT2
ztrans_setFrom(ZTrans* trans, const void* from);







U_CAPI void U_EXPORT2
ztrans_adoptFrom(ZTrans* trans, void* from);






U_CAPI void* U_EXPORT2
ztrans_getTo(ZTrans* trans);







U_CAPI void U_EXPORT2
ztrans_setTo(ZTrans* trans, const void* to);







U_CAPI void U_EXPORT2
ztrans_adoptTo(ZTrans* trans, void* to);












U_CAPI UClassID U_EXPORT2
ztrans_getStaticClassID(ZTrans* trans);












U_CAPI UClassID U_EXPORT2
ztrans_getDynamicClassID(ZTrans* trans);

#endif

#endif
