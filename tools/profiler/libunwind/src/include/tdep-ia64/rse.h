








#ifndef RSE_H
#define RSE_H

#include <libunwind.h>

static inline uint64_t
rse_slot_num (uint64_t addr)
{
	return (addr >> 3) & 0x3f;
}




static inline uint64_t
rse_is_rnat_slot (uint64_t addr)
{
	return rse_slot_num (addr) == 0x3f;
}





static inline uint64_t
rse_rnat_addr (uint64_t slot_addr)
{
	return slot_addr | (0x3f << 3);
}






static inline uint64_t
rse_num_regs (uint64_t bspstore, uint64_t bsp)
{
	uint64_t slots = (bsp - bspstore) >> 3;

	return slots - (rse_slot_num(bspstore) + slots)/0x40;
}





static inline uint64_t
rse_skip_regs (uint64_t addr, long num_regs)
{
	long delta = rse_slot_num(addr) + num_regs;

	if (num_regs < 0)
		delta -= 0x3e;
	return addr + ((num_regs + delta/0x3f) << 3);
}

#endif 
