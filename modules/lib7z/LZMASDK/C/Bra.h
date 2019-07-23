


#ifndef __BRA_H
#define __BRA_H

#include "Types.h"

#ifdef __cplusplus
extern "C" {
#endif












































#define x86_Convert_Init(state) { state = 0; }
SizeT x86_Convert(Byte *data, SizeT size, UInt32 ip, UInt32 *state, int encoding);
SizeT ARM_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT ARMT_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT PPC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT SPARC_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);
SizeT IA64_Convert(Byte *data, SizeT size, UInt32 ip, int encoding);

#ifdef __cplusplus
}
#endif

#endif
