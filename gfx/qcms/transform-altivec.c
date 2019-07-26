






















#include <altivec.h>

#include "qcmsint.h"

#define FLOATSCALE (float)(PRECACHE_OUTPUT_SIZE)
#define CLAMPMAXVAL (((float) (PRECACHE_OUTPUT_SIZE - 1)) / PRECACHE_OUTPUT_SIZE)
static const ALIGN float floatScaleX4 = FLOATSCALE;
static const ALIGN float clampMaxValueX4 = CLAMPMAXVAL;

inline vector float load_aligned_float(float *dataPtr)
{
	vector float data = vec_lde(0, dataPtr);
	vector unsigned char moveToStart = vec_lvsl(0, dataPtr);
	return vec_perm(data, data, moveToStart);
}

void qcms_transform_data_rgb_out_lut_altivec(qcms_transform *transform,
                                             unsigned char *src,
                                             unsigned char *dest,
                                             size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	char input_back[32];
	



	float const *input = (float*)(((uintptr_t)&input_back[16]) & ~0xf);
	

	uint32_t const *output = (uint32_t*)input;

	
	const float *igtbl_r = transform->input_gamma_table_r;
	const float *igtbl_g = transform->input_gamma_table_g;
	const float *igtbl_b = transform->input_gamma_table_b;

	
	const uint8_t *otdata_r = &transform->output_table_r->data[0];
	const uint8_t *otdata_g = &transform->output_table_g->data[0];
	const uint8_t *otdata_b = &transform->output_table_b->data[0];

	
	const vector float mat0 = vec_ldl(0, (vector float*)mat[0]);
	const vector float mat1 = vec_ldl(0, (vector float*)mat[1]);
	const vector float mat2 = vec_ldl(0, (vector float*)mat[2]);

	
	const vector float max = vec_splat(vec_lde(0, (float*)&clampMaxValueX4), 0);
	const vector float min = (vector float)vec_splat_u32(0);
	const vector float scale = vec_splat(vec_lde(0, (float*)&floatScaleX4), 0);

	
	vector float vec_r, vec_g, vec_b, result;

	
	if (!length)
		return;

	
	length--;

	
	vec_r = load_aligned_float((float*)&igtbl_r[src[0]]);
	vec_g = load_aligned_float((float*)&igtbl_r[src[1]]);
	vec_b = load_aligned_float((float*)&igtbl_r[src[2]]);
	src += 3;

	

	for (i=0; i<length; i++)
	{
		
		vec_r = vec_splat(vec_r, 0);
		vec_g = vec_splat(vec_g, 0);
		vec_b = vec_splat(vec_b, 0);

		
		vec_r = vec_madd(vec_r, mat0, min);
		vec_g = vec_madd(vec_g, mat1, min);
		vec_b = vec_madd(vec_b, mat2, min);

		
		vec_r = vec_add(vec_r, vec_add(vec_g, vec_b));
		vec_r = vec_max(min, vec_r);
		vec_r = vec_min(max, vec_r);
		result = vec_madd(vec_r, scale, min);

		
		vec_st(vec_ctu(vec_round(result), 0), 0, (vector unsigned int*)output);

		
		vec_r = load_aligned_float((float*)&igtbl_r[src[0]]);
		vec_g = load_aligned_float((float*)&igtbl_r[src[1]]);
		vec_b = load_aligned_float((float*)&igtbl_r[src[2]]);
		src += 3;

		
		dest[0] = otdata_r[output[0]];
		dest[1] = otdata_g[output[1]];
		dest[2] = otdata_b[output[2]];
		dest += 3;
	}

	

	vec_r = vec_splat(vec_r, 0);
	vec_g = vec_splat(vec_g, 0);
	vec_b = vec_splat(vec_b, 0);

	vec_r = vec_madd(vec_r, mat0, min);
	vec_g = vec_madd(vec_g, mat1, min);
	vec_b = vec_madd(vec_b, mat2, min);

	vec_r = vec_add(vec_r, vec_add(vec_g, vec_b));
	vec_r = vec_max(min, vec_r);
	vec_r = vec_min(max, vec_r);
	result = vec_madd(vec_r, scale, min);

	vec_st(vec_ctu(vec_round(result),0),0,(vector unsigned int*)output);

	dest[0] = otdata_r[output[0]];
	dest[1] = otdata_g[output[1]];
	dest[2] = otdata_b[output[2]];
}

void qcms_transform_data_rgba_out_lut_altivec(qcms_transform *transform,
                                              unsigned char *src,
                                              unsigned char *dest,
                                              size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	char input_back[32];
	



	float const *input = (float*)(((uintptr_t)&input_back[16]) & ~0xf);
	

	uint32_t const *output = (uint32_t*)input;

	
	const float *igtbl_r = transform->input_gamma_table_r;
	const float *igtbl_g = transform->input_gamma_table_g;
	const float *igtbl_b = transform->input_gamma_table_b;

	
	const uint8_t *otdata_r = &transform->output_table_r->data[0];
	const uint8_t *otdata_g = &transform->output_table_g->data[0];
	const uint8_t *otdata_b = &transform->output_table_b->data[0];

	
	const vector float mat0 = vec_ldl(0, (vector float*)mat[0]);
	const vector float mat1 = vec_ldl(0, (vector float*)mat[1]);
	const vector float mat2 = vec_ldl(0, (vector float*)mat[2]);

	
	const vector float max = vec_splat(vec_lde(0, (float*)&clampMaxValueX4), 0);
	const vector float min = (vector float)vec_splat_u32(0);
	const vector float scale = vec_splat(vec_lde(0, (float*)&floatScaleX4), 0);

	
	vector float vec_r, vec_g, vec_b, result;
	unsigned char alpha;

	
	if (!length)
		return;

	
	length--;

	
	vec_r = load_aligned_float((float*)&igtbl_r[src[0]]);
	vec_g = load_aligned_float((float*)&igtbl_r[src[1]]);
	vec_b = load_aligned_float((float*)&igtbl_r[src[2]]);
	alpha = src[3];
	src += 4;

	

	for (i=0; i<length; i++)
	{
		
		vec_r = vec_splat(vec_r, 0);
		vec_g = vec_splat(vec_g, 0);
		vec_b = vec_splat(vec_b, 0);

		
		vec_r = vec_madd(vec_r, mat0, min);
		vec_g = vec_madd(vec_g, mat1, min);
		vec_b = vec_madd(vec_b, mat2, min);

		
		dest[3] = alpha;
		alpha = src[3];

		
		vec_r = vec_add(vec_r, vec_add(vec_g, vec_b));
		vec_r = vec_max(min, vec_r);
		vec_r = vec_min(max, vec_r);
		result = vec_madd(vec_r, scale, min);

		
		vec_st(vec_ctu(vec_round(result), 0), 0, (vector unsigned int*)output);

		
		vec_r = load_aligned_float((float*)&igtbl_r[src[0]]);
		vec_g = load_aligned_float((float*)&igtbl_r[src[1]]);
		vec_b = load_aligned_float((float*)&igtbl_r[src[2]]);
		src += 4;

		
		dest[0] = otdata_r[output[0]];
		dest[1] = otdata_g[output[1]];
		dest[2] = otdata_b[output[2]];
		dest += 4;
	}

	

	vec_r = vec_splat(vec_r, 0);
	vec_g = vec_splat(vec_g, 0);
	vec_b = vec_splat(vec_b, 0);

	vec_r = vec_madd(vec_r, mat0, min);
	vec_g = vec_madd(vec_g, mat1, min);
	vec_b = vec_madd(vec_b, mat2, min);

	dest[3] = alpha;

	vec_r = vec_add(vec_r, vec_add(vec_g, vec_b));
	vec_r = vec_max(min, vec_r);
	vec_r = vec_min(max, vec_r);
	result = vec_madd(vec_r, scale, min);

	vec_st(vec_ctu(vec_round(result), 0), 0, (vector unsigned int*)output);

	dest[0] = otdata_r[output[0]];
	dest[1] = otdata_g[output[1]];
	dest[2] = otdata_b[output[2]];
}

