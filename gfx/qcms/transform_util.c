#define _ISOC99_SOURCE

#include <math.h>
#include <assert.h>
#include <string.h> 
#include "qcmsint.h"
#include "transform_util.h"
#include "matrix.h"

#if !defined(INFINITY)
#define INFINITY HUGE_VAL
#endif

#define PARAMETRIC_CURVE_TYPE 0x70617261 //'para'




float lut_interp_linear(double input_value, uint16_t *table, int length)
{
	int upper, lower;
	float value;
	input_value = input_value * (length - 1); 
	upper = ceil(input_value);
	lower = floor(input_value);
	
	value = table[upper]*(1. - (upper - input_value)) + table[lower]*(upper - input_value);
	
	return value * (1.f/65535.f);
}


uint16_t lut_interp_linear16(uint16_t input_value, uint16_t *table, int length)
{
	

	uint32_t value = (input_value * (length - 1));
	uint32_t upper = (value + 65534) / 65535; 
	uint32_t lower = value / 65535;           
	
	uint32_t interp = value % 65535;

	value = (table[upper]*(interp) + table[lower]*(65535 - interp))/65535; 

	return value;
}



static
uint8_t lut_interp_linear_precache_output(uint32_t input_value, uint16_t *table, int length)
{
	

	uint32_t value = (input_value * (length - 1));

	
	uint32_t upper = (value + PRECACHE_OUTPUT_MAX-1) / PRECACHE_OUTPUT_MAX;
	
	uint32_t lower = value / PRECACHE_OUTPUT_MAX;
	
	uint32_t interp = value % PRECACHE_OUTPUT_MAX;

	
	value = (table[upper]*(interp) + table[lower]*(PRECACHE_OUTPUT_MAX - interp)); 

	
	value += (PRECACHE_OUTPUT_MAX*65535/255)/2;
        value /= (PRECACHE_OUTPUT_MAX*65535/255); 
	return value;
}



float lut_interp_linear_float(float value, float *table, int length)
{
        int upper, lower;
        value = value * (length - 1);
        upper = ceilf(value);
        lower = floorf(value);
        
        value = table[upper]*(1. - (upper - value)) + table[lower]*(upper - value);
        
        return value;
}

#if 0



uint16_t lut_interp_linear16(uint16_t input_value, uint16_t *table, int length)
{
	uint32_t value = (input_value * (length - 1));
	uint32_t upper = (value + 4095) / 4096; 
	uint32_t lower = value / 4096;           
	uint32_t interp = value % 4096;

	value = (table[upper]*(interp) + table[lower]*(4096 - interp))/4096; 

	return value;
}
#endif

void compute_curve_gamma_table_type1(float gamma_table[256], uint16_t gamma)
{
	unsigned int i;
	float gamma_float = u8Fixed8Number_to_float(gamma);
	for (i = 0; i < 256; i++) {
                
		gamma_table[i] = pow(i/255., gamma_float);
	}
}

void compute_curve_gamma_table_type2(float gamma_table[256], uint16_t *table, int length)
{
	unsigned int i;
	for (i = 0; i < 256; i++) {
		gamma_table[i] = lut_interp_linear(i/255., table, length);
	}
}

void compute_curve_gamma_table_type_parametric(float gamma_table[256], float parameter[7], int count)
{
        size_t X;
        float interval;
        float a, b, c, e, f;
        float y = parameter[0];
        if (count == 0) {
                a = 1;
                b = 0;
                c = 0;
                e = 0;
                f = 0;
                interval = -INFINITY;
        } else if(count == 1) {
                a = parameter[1];
                b = parameter[2];
                c = 0;
                e = 0;
                f = 0;
                interval = -1 * parameter[2] / parameter[1];
        } else if(count == 2) {
                a = parameter[1];
                b = parameter[2];
                c = 0;
                e = parameter[3];
                f = parameter[3];
                interval = -1 * parameter[2] / parameter[1];
        } else if(count == 3) {
                a = parameter[1];
                b = parameter[2];
                c = parameter[3];
                e = -c;
                f = 0;
                interval = parameter[4];
        } else if(count == 4) {
                a = parameter[1];
                b = parameter[2];
                c = parameter[3];
                e = parameter[5] - c;
                f = parameter[6];
                interval = parameter[4];
        } else {
                assert(0 && "invalid parametric function type.");
                a = 1;
                b = 0;
                c = 0;
                e = 0;
                f = 0;
                interval = -INFINITY;
        }       
        for (X = 0; X < 256; X++) {
                if (X >= interval) {
                        
                        
                        
                        gamma_table[X] = clamp_float(pow(a * X / 255. + b, y) + c + e);
                } else {
                        gamma_table[X] = clamp_float(c * X / 255. + f);
                }
        }
}

void compute_curve_gamma_table_type0(float gamma_table[256])
{
	unsigned int i;
	for (i = 0; i < 256; i++) {
		gamma_table[i] = i/255.;
	}
}

float *build_input_gamma_table(struct curveType *TRC)
{
	float *gamma_table;

	if (!TRC) return NULL;
	gamma_table = malloc(sizeof(float)*256);
	if (gamma_table) {
		if (TRC->type == PARAMETRIC_CURVE_TYPE) {
			compute_curve_gamma_table_type_parametric(gamma_table, TRC->parameter, TRC->count);
		} else {
			if (TRC->count == 0) {
				compute_curve_gamma_table_type0(gamma_table);
			} else if (TRC->count == 1) {
				compute_curve_gamma_table_type1(gamma_table, TRC->data[0]);
			} else {
				compute_curve_gamma_table_type2(gamma_table, TRC->data, TRC->count);
			}
		}
	}
        return gamma_table;
}

struct matrix build_colorant_matrix(qcms_profile *p)
{
	struct matrix result;
	result.m[0][0] = s15Fixed16Number_to_float(p->redColorant.X);
	result.m[0][1] = s15Fixed16Number_to_float(p->greenColorant.X);
	result.m[0][2] = s15Fixed16Number_to_float(p->blueColorant.X);
	result.m[1][0] = s15Fixed16Number_to_float(p->redColorant.Y);
	result.m[1][1] = s15Fixed16Number_to_float(p->greenColorant.Y);
	result.m[1][2] = s15Fixed16Number_to_float(p->blueColorant.Y);
	result.m[2][0] = s15Fixed16Number_to_float(p->redColorant.Z);
	result.m[2][1] = s15Fixed16Number_to_float(p->greenColorant.Z);
	result.m[2][2] = s15Fixed16Number_to_float(p->blueColorant.Z);
	result.invalid = false;
	return result;
}





uint16_fract_t lut_inverse_interp16(uint16_t Value, uint16_t LutTable[], int length)
{
        int l = 1;
        int r = 0x10000;
        int x = 0, res;       
        int NumZeroes, NumPoles;
        int cell0, cell1;
        double val2;
        double y0, y1, x0, x1;
        double a, b, f;

        
        
        
        

        NumZeroes = 0;
        while (LutTable[NumZeroes] == 0 && NumZeroes < length-1)
                        NumZeroes++;

        
        
	
        if (NumZeroes == 0 && Value == 0)
            return 0;

        NumPoles = 0;
        while (LutTable[length-1- NumPoles] == 0xFFFF && NumPoles < length-1)
                        NumPoles++;

        
        if (NumZeroes > 1 || NumPoles > 1)
        {
                int a, b;

                
                if (Value == 0) return 0;
                

                

                if (NumZeroes > 1) {
                        a = ((NumZeroes-1) * 0xFFFF) / (length-1);
                        l = a - 1;
                }
                if (NumPoles > 1) {
                        b = ((length-1 - NumPoles) * 0xFFFF) / (length-1);
                        r = b + 1;
                }
        }

        if (r <= l) {
                
                return 0;
        }


        
        while (r > l) {

                x = (l + r) / 2;

		res = (int) lut_interp_linear16((uint16_fract_t) (x-1), LutTable, length);

                if (res == Value) {

                    

                    return (uint16_fract_t) (x - 1);
                }

                if (res > Value) r = x - 1;
                else l = x + 1;
        }

        

        

        assert(x >= 1);

        val2 = (length-1) * ((double) (x - 1) / 65535.0);

        cell0 = (int) floor(val2);
        cell1 = (int) ceil(val2);
           
        if (cell0 == cell1) return (uint16_fract_t) x;

        y0 = LutTable[cell0] ;
        x0 = (65535.0 * cell0) / (length-1); 

        y1 = LutTable[cell1] ;
        x1 = (65535.0 * cell1) / (length-1);

        a = (y1 - y0) / (x1 - x0);
        b = y0 - a * x0;

        if (fabs(a) < 0.01) return (uint16_fract_t) x;

        f = ((Value - b) / a);

        if (f < 0.0) return (uint16_fract_t) 0;
        if (f >= 65535.0) return (uint16_fract_t) 0xFFFF;

        return (uint16_fract_t) floor(f + 0.5);                        

}














static uint16_t *invert_lut(uint16_t *table, int length, int out_length)
{
        int i;
        

        uint16_t *output = malloc(sizeof(uint16_t)*out_length);
        if (!output)
                return NULL;

        for (i = 0; i < out_length; i++) {
                double x = ((double) i * 65535.) / (double) (out_length - 1);
                uint16_fract_t input = floor(x + .5);
                output[i] = lut_inverse_interp16(input, table, length);
        }
        return output;
}

static void compute_precache_pow(uint8_t *output, float gamma)
{
	uint32_t v = 0;
	for (v = 0; v < PRECACHE_OUTPUT_SIZE; v++) {
		
		output[v] = 255. * pow(v/(double)PRECACHE_OUTPUT_MAX, gamma);
	}
}

void compute_precache_lut(uint8_t *output, uint16_t *table, int length)
{
	uint32_t v = 0;
	for (v = 0; v < PRECACHE_OUTPUT_SIZE; v++) {
		output[v] = lut_interp_linear_precache_output(v, table, length);
	}
}

void compute_precache_linear(uint8_t *output)
{
	uint32_t v = 0;
	for (v = 0; v < PRECACHE_OUTPUT_SIZE; v++) {
		
		output[v] = v / (PRECACHE_OUTPUT_SIZE/256);
	}
}

qcms_bool compute_precache(struct curveType *trc, uint8_t *output)
{
        
        if (trc->type == PARAMETRIC_CURVE_TYPE) {
                        float gamma_table[256];
                        uint16_t gamma_table_uint[256];
                        uint16_t i;
                        uint16_t *inverted;
                        int inverted_size = 256;

                        compute_curve_gamma_table_type_parametric(gamma_table, trc->parameter, trc->count);
                        for(i = 0; i < 256; i++) {
                                gamma_table_uint[i] = (uint16_t)(gamma_table[i] * 65535);
                        }

                        
                        
                        
                        
                        if (inverted_size < 256)
                                inverted_size = 256;

                        inverted = invert_lut(gamma_table_uint, 256, inverted_size);
                        if (!inverted)
                                return false;
                        compute_precache_lut(output, inverted, inverted_size);
                        free(inverted);
        } else {
                if (trc->count == 0) {
                        compute_precache_linear(output);
                } else if (trc->count == 1) {
                        compute_precache_pow(output, 1./u8Fixed8Number_to_float(trc->data[0]));
                } else {
                        uint16_t *inverted;
                        int inverted_size = trc->count;
                        
                        
                        
                        
                        if (inverted_size < 256)
                                inverted_size = 256;

                        inverted = invert_lut(trc->data, trc->count, inverted_size);
                        if (!inverted)
                                return false;
                        compute_precache_lut(output, inverted, inverted_size);
                        free(inverted);
                }
        }
        return true;
}


static uint16_t *build_linear_table(int length)
{
        int i;
        uint16_t *output = malloc(sizeof(uint16_t)*length);
        if (!output)
                return NULL;

        for (i = 0; i < length; i++) {
                double x = ((double) i * 65535.) / (double) (length - 1);
                uint16_fract_t input = floor(x + .5);
                output[i] = input;
        }
        return output;
}

static uint16_t *build_pow_table(float gamma, int length)
{
        int i;
        uint16_t *output = malloc(sizeof(uint16_t)*length);
        if (!output)
                return NULL;

        for (i = 0; i < length; i++) {
                uint16_fract_t result;
                double x = ((double) i) / (double) (length - 1);
                x = pow(x, gamma);                
                result = floor(x*65535. + .5);
                output[i] = result;
        }
        return output;
}

void build_output_lut(struct curveType *trc,
                uint16_t **output_gamma_lut, size_t *output_gamma_lut_length)
{
        if (trc->type == PARAMETRIC_CURVE_TYPE) {
                float gamma_table[256];
                uint16_t i;
                uint16_t *output = malloc(sizeof(uint16_t)*256);

                if (!output) {
                        *output_gamma_lut = NULL;
                        return;
                }

                compute_curve_gamma_table_type_parametric(gamma_table, trc->parameter, trc->count);
                *output_gamma_lut_length = 256;
                for(i = 0; i < 256; i++) {
                        output[i] = (uint16_t)(gamma_table[i] * 65535);
                }
                *output_gamma_lut = output;
        } else {
                if (trc->count == 0) {
                        *output_gamma_lut = build_linear_table(4096);
                        *output_gamma_lut_length = 4096;
                } else if (trc->count == 1) {
                        float gamma = 1./u8Fixed8Number_to_float(trc->data[0]);
                        *output_gamma_lut = build_pow_table(gamma, 4096);
                        *output_gamma_lut_length = 4096;
                } else {
                        
                        
                        *output_gamma_lut_length = trc->count;
                        if (*output_gamma_lut_length < 256)
                                *output_gamma_lut_length = 256;

                        *output_gamma_lut = invert_lut(trc->data, trc->count, *output_gamma_lut_length);
                }
        }

}

