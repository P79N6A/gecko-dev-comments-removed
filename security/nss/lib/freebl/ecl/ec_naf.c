



#include "ecl-priv.h"



int
ec_twoTo(int e)
{
	int a = 1;
	int i;

	for (i = 0; i < e; i++) {
		a *= 2;
	}
	return a;
}







mp_err
ec_compute_wNAF(signed char *out, int bitsize, const mp_int *in, int w)
{
	mp_int k;
	mp_err res = MP_OKAY;
	int i, twowm1, mask;

	twowm1 = ec_twoTo(w - 1);
	mask = 2 * twowm1 - 1;

	MP_DIGITS(&k) = 0;
	MP_CHECKOK(mp_init_copy(&k, in));

	i = 0;
	
	while (mp_cmp_z(&k) > 0) {
		if (mp_isodd(&k)) {
			out[i] = MP_DIGIT(&k, 0) & mask;
			if (out[i] >= twowm1)
				out[i] -= 2 * twowm1;

			

			if (out[i] >= 0) {
				mp_sub_d(&k, out[i], &k);
			} else {
				mp_add_d(&k, -(out[i]), &k);
			}
		} else {
			out[i] = 0;
		}
		mp_div_2(&k, &k);
		i++;
	}
	
	for (; i < bitsize + 1; i++) {
		out[i] = 0;
	}
  CLEANUP:
	mp_clear(&k);
	return res;

}
