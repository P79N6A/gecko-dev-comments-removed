



#ifndef __ecl_priv_h_
#define __ecl_priv_h_

#include "ecl.h"
#include "mpi.h"
#include "mplogic.h"



#if defined(MP_USE_LONG_LONG_DIGIT) || defined(MP_USE_LONG_DIGIT)
#define ECL_SIXTY_FOUR_BIT
#else
#define ECL_THIRTY_TWO_BIT
#endif

#define ECL_CURVE_DIGITS(curve_size_in_bits) \
	(((curve_size_in_bits)+(sizeof(mp_digit)*8-1))/(sizeof(mp_digit)*8))
#define ECL_BITS (sizeof(mp_digit)*8)
#define ECL_MAX_FIELD_SIZE_DIGITS (80/sizeof(mp_digit))




#define MP_GET_BIT(a, i) \
	((i) >= mpl_significant_bits((a))) ? 0 : mpl_get_bit((a), (i))

#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
#define MP_ADD_CARRY(a1, a2, s, cin, cout)   \
    { mp_word w; \
    w = ((mp_word)(cin)) + (a1) + (a2); \
    s = ACCUM(w); \
    cout = CARRYOUT(w); }

#define MP_SUB_BORROW(a1, a2, s, bin, bout)   \
    { mp_word w; \
    w = ((mp_word)(a1)) - (a2) - (bin); \
    s = ACCUM(w); \
    bout = (w >> MP_DIGIT_BIT) & 1; }

#else






#define MP_ADD_CARRY(a1, a2, s, cin, cout)   \
    { mp_digit tmp,sum; \
    tmp = (a1); \
    sum = tmp + (a2); \
    tmp = (sum < tmp);                     /* detect overflow */ \
    s = sum += (cin); \
    cout = tmp + (sum < (cin)); }

#define MP_SUB_BORROW(a1, a2, s, bin, bout)   \
    { mp_digit tmp; \
    tmp = (a1); \
    s = tmp - (a2); \
    tmp = (s > tmp);                    /* detect borrow */ \
    if ((bin) && !s--) tmp++;	\
    bout = tmp; }
#endif


struct GFMethodStr;
typedef struct GFMethodStr GFMethod;
struct GFMethodStr {
	

	int constructed;
	


	mp_int irr;
	




	unsigned int irr_arr[5];
	



	mp_err (*field_add) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_neg) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_sub) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_mod) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_mul) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_sqr) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_div) (const mp_int *a, const mp_int *b, mp_int *r,
						 const GFMethod *meth);
	mp_err (*field_enc) (const mp_int *a, mp_int *r, const GFMethod *meth);
	mp_err (*field_dec) (const mp_int *a, mp_int *r, const GFMethod *meth);
	

	void *extra1;
	void *extra2;
	void (*extra_free) (GFMethod *meth);
};


GFMethod *GFMethod_consGFp(const mp_int *irr);
GFMethod *GFMethod_consGFp_mont(const mp_int *irr);
GFMethod *GFMethod_consGF2m(const mp_int *irr,
							const unsigned int irr_arr[5]);

void GFMethod_free(GFMethod *meth);

struct ECGroupStr {
	

	int constructed;
	
	GFMethod *meth;
	
	char *text;
	
	mp_int curvea, curveb;
	
	mp_int genx, geny;
	
	mp_int order;
	int cofactor;
	



	mp_err (*point_add) (const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_sub) (const mp_int *px, const mp_int *py,
						 const mp_int *qx, const mp_int *qy, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_dbl) (const mp_int *px, const mp_int *py, mp_int *rx,
						 mp_int *ry, const ECGroup *group);
	mp_err (*point_mul) (const mp_int *n, const mp_int *px,
						 const mp_int *py, mp_int *rx, mp_int *ry,
						 const ECGroup *group);
	mp_err (*base_point_mul) (const mp_int *n, mp_int *rx, mp_int *ry,
							  const ECGroup *group);
	mp_err (*points_mul) (const mp_int *k1, const mp_int *k2,
						  const mp_int *px, const mp_int *py, mp_int *rx,
						  mp_int *ry, const ECGroup *group);
	mp_err (*validate_point) (const mp_int *px, const mp_int *py, const ECGroup *group);
	

	void *extra1;
	void *extra2;
	void (*extra_free) (ECGroup *group);
};


mp_err ec_GFp_add(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_neg(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_sub(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);


mp_err ec_GFp_add_3(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_add_4(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_add_5(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_add_6(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sub_3(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sub_4(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sub_5(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sub_6(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);

mp_err ec_GFp_mod(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_mul(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);
mp_err ec_GFp_sqr(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_div(const mp_int *a, const mp_int *b, mp_int *r,
				  const GFMethod *meth);

mp_err ec_GF2m_add(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);
mp_err ec_GF2m_neg(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_mod(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_mul(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);
mp_err ec_GF2m_sqr(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GF2m_div(const mp_int *a, const mp_int *b, mp_int *r,
				   const GFMethod *meth);


mp_err ec_GFp_mul_mont(const mp_int *a, const mp_int *b, mp_int *r,
					   const GFMethod *meth);
mp_err ec_GFp_sqr_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_div_mont(const mp_int *a, const mp_int *b, mp_int *r,
					   const GFMethod *meth);
mp_err ec_GFp_enc_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
mp_err ec_GFp_dec_mont(const mp_int *a, mp_int *r, const GFMethod *meth);
void ec_GFp_extra_free_mont(GFMethod *meth);


mp_err ec_pts_mul_basic(const mp_int *k1, const mp_int *k2,
						const mp_int *px, const mp_int *py, mp_int *rx,
						mp_int *ry, const ECGroup *group);
mp_err ec_pts_mul_simul_w2(const mp_int *k1, const mp_int *k2,
						   const mp_int *px, const mp_int *py, mp_int *rx,
						   mp_int *ry, const ECGroup *group);







mp_err ec_compute_wNAF(signed char *out, int bitsize, const mp_int *in,
					   int w);


mp_err ec_group_set_gfp192(ECGroup *group, ECCurveName);
mp_err ec_group_set_gfp224(ECGroup *group, ECCurveName);
mp_err ec_group_set_gfp256(ECGroup *group, ECCurveName);
mp_err ec_group_set_gfp384(ECGroup *group, ECCurveName);
mp_err ec_group_set_gfp521(ECGroup *group, ECCurveName);
mp_err ec_group_set_gf2m163(ECGroup *group, ECCurveName name);
mp_err ec_group_set_gf2m193(ECGroup *group, ECCurveName name);
mp_err ec_group_set_gf2m233(ECGroup *group, ECCurveName name);


#ifdef ECL_USE_FP
mp_err ec_group_set_secp160r1_fp(ECGroup *group);
mp_err ec_group_set_nistp192_fp(ECGroup *group);
mp_err ec_group_set_nistp224_fp(ECGroup *group);
#endif

#endif							
