










#ifndef ENCODEMB_ARM_H
#define ENCODEMB_ARM_H

#if HAVE_ARMV6
extern prototype_subb(vp8_subtract_b_armv6);
extern prototype_submby(vp8_subtract_mby_armv6);
extern prototype_submbuv(vp8_subtract_mbuv_armv6);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_encodemb_subb
#define vp8_encodemb_subb vp8_subtract_b_armv6

#undef  vp8_encodemb_submby
#define vp8_encodemb_submby vp8_subtract_mby_armv6

#undef  vp8_encodemb_submbuv
#define vp8_encodemb_submbuv vp8_subtract_mbuv_armv6
#endif

#endif 

#if HAVE_ARMV7




extern prototype_subb(vp8_subtract_b_neon);
extern prototype_submby(vp8_subtract_mby_neon);
extern prototype_submbuv(vp8_subtract_mbuv_neon);










#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_encodemb_subb
#define vp8_encodemb_subb vp8_subtract_b_neon

#undef  vp8_encodemb_submby
#define vp8_encodemb_submby vp8_subtract_mby_neon

#undef  vp8_encodemb_submbuv
#define vp8_encodemb_submbuv vp8_subtract_mbuv_neon
#endif

#endif

#endif
