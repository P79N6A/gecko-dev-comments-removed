#ifndef IA32_IMPLICIT_H
#define IA32_IMPLICIT_H

#include "libdis.h"




#define IDX_IMPLICIT_REP 41	/* change this if the table changes! */

unsigned int ia32_insn_implicit_ops( x86_insn_t *insn, unsigned int impl_idx );

#endif
