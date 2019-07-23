





















#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "qcmsint.h"


#if defined(_M_IX86) || defined(__i386__) || defined(_M_AMD64) || defined(__x86_64__)
#define X86
#endif 


typedef uint16_t uint16_fract_t;



float lut_interp_linear(double value, uint16_t *table, int length)
{
	int upper, lower;
	value = value * (length - 1);
	upper = ceil(value);
	lower = floor(value);
	
	value = table[upper]*(1. - (upper - value)) + table[lower]*(upper - value);
	
	return value * (1./65535.);
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

void compute_curve_gamma_table_type1(float gamma_table[256], double gamma)
{
	unsigned int i;
	for (i = 0; i < 256; i++) {
		gamma_table[i] = powf(i/255.f, gamma);
	}
}

void compute_curve_gamma_table_type2(float gamma_table[256], uint16_t *table, int length)
{
	unsigned int i;
	for (i = 0; i < 256; i++) {
		gamma_table[i] = lut_interp_linear(i/255., table, length);
	}
}

void compute_curve_gamma_table_type0(float gamma_table[256])
{
	unsigned int i;
	for (i = 0; i < 256; i++) {
		gamma_table[i] = i/255.;
	}
}

unsigned char clamp_u8(float v)
{
	if (v > 255.)
		return 255;
	else if (v < 0)
		return 0;
	else
		return floor(v+.5);
}

struct vector {
	float v[3];
};

struct matrix {
	float m[3][3];
	bool invalid;
};

struct vector matrix_eval(struct matrix mat, struct vector v)
{
	struct vector result;
	result.v[0] = mat.m[0][0]*v.v[0] + mat.m[0][1]*v.v[1] + mat.m[0][2]*v.v[2];
	result.v[1] = mat.m[1][0]*v.v[0] + mat.m[1][1]*v.v[1] + mat.m[1][2]*v.v[2];
	result.v[2] = mat.m[2][0]*v.v[0] + mat.m[2][1]*v.v[1] + mat.m[2][2]*v.v[2];
	return result;
}



float matrix_det(struct matrix mat)
{
	float det;
	det = mat.m[0][0]*mat.m[1][1]*mat.m[2][2] +
		mat.m[0][1]*mat.m[1][2]*mat.m[2][0] +
		mat.m[0][2]*mat.m[1][0]*mat.m[2][1] -
		mat.m[0][0]*mat.m[1][2]*mat.m[2][1] -
		mat.m[0][1]*mat.m[1][0]*mat.m[2][2] -
		mat.m[0][2]*mat.m[1][1]*mat.m[2][0];
	return det;
}





struct matrix matrix_invert(struct matrix mat)
{
	struct matrix dest_mat;
	int i,j;
	static int a[3] = { 2, 2, 1 };
	static int b[3] = { 1, 0, 0 };

	
	float det = matrix_det(mat);

	if (det == 0) {
		dest_mat.invalid = true;
	} else {
		dest_mat.invalid = false;
	}

	det = 1/det;

	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			double p;
			int ai = a[i];
			int aj = a[j];
			int bi = b[i];
			int bj = b[j];

			p = mat.m[ai][aj] * mat.m[bi][bj] -
				mat.m[ai][bj] * mat.m[bi][aj];
			if (((i + j) & 1) != 0)
				p = -p;

			dest_mat.m[j][i] = det * p;
		}
	}
	return dest_mat;
}

struct matrix matrix_identity(void)
{
	struct matrix i;
	i.m[0][0] = 1;
	i.m[0][1] = 0;
	i.m[0][2] = 0;
	i.m[1][0] = 0;
	i.m[1][1] = 1;
	i.m[1][2] = 0;
	i.m[2][0] = 0;
	i.m[2][1] = 0;
	i.m[2][2] = 1;
	i.invalid = false;
	return i;
}

static struct matrix matrix_invalid(void)
{
	struct matrix inv = matrix_identity();
	inv.invalid = true;
	return inv;
}




struct matrix matrix_multiply(struct matrix a, struct matrix b)
{
	struct matrix result;
	int dx, dy;
	int o;
	for (dy = 0; dy < 3; dy++) {
		for (dx = 0; dx < 3; dx++) {
			double v = 0;
			for (o = 0; o < 3; o++) {
				v += a.m[dy][o] * b.m[o][dx];
			}
			result.m[dy][dx] = v;
		}
	}
	result.invalid = a.invalid || b.invalid;
	return result;
}

float u8Fixed8Number_to_float(uint16_t x)
{
	
	
	
	return x/256.;
}

float *build_input_gamma_table(struct curveType *TRC)
{
	float *gamma_table = malloc(sizeof(float)*256);
	if (gamma_table) {
		if (TRC->count == 0) {
			compute_curve_gamma_table_type0(gamma_table);
		} else if (TRC->count == 1) {
			compute_curve_gamma_table_type1(gamma_table, u8Fixed8Number_to_float(TRC->data[0]));
		} else {
			compute_curve_gamma_table_type2(gamma_table, TRC->data, TRC->count);
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
               

                

                a = ((NumZeroes-1) * 0xFFFF) / (length-1);               
                b = ((length-1 - NumPoles) * 0xFFFF) / (length-1);
                                                                
                l = a - 1;
                r = b + 1;
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













static struct matrix build_RGB_to_XYZ_transfer_matrix(qcms_CIE_xyY white, qcms_CIE_xyYTRIPLE primrs)
{
	struct matrix primaries;
	struct matrix primaries_invert;
	struct matrix result;
	struct vector white_point;
	struct vector coefs;

	double xn, yn;
	double xr, yr;
	double xg, yg;
	double xb, yb;

	xn = white.x;
	yn = white.y;

	if (yn == 0.0)
		return matrix_invalid();

	xr = primrs.red.x;
	yr = primrs.red.y;
	xg = primrs.green.x;
	yg = primrs.green.y;
	xb = primrs.blue.x;
	yb = primrs.blue.y;

	primaries.m[0][0] = xr;
	primaries.m[0][1] = xg;
	primaries.m[0][2] = xb;

	primaries.m[1][0] = yr;
	primaries.m[1][1] = yg;
	primaries.m[1][2] = yb;

	primaries.m[2][0] = 1 - xr - yr;
	primaries.m[2][1] = 1 - xg - yg;
	primaries.m[2][2] = 1 - xb - yb;
	primaries.invalid = false;

	white_point.v[0] = xn/yn;
	white_point.v[1] = 1.;
	white_point.v[2] = (1.0-xn-yn)/yn;

	primaries_invert = matrix_invert(primaries);

	coefs = matrix_eval(primaries_invert, white_point);

	result.m[0][0] = coefs.v[0]*xr;
	result.m[0][1] = coefs.v[1]*xg;
	result.m[0][2] = coefs.v[2]*xb;

	result.m[1][0] = coefs.v[0]*yr;
	result.m[1][1] = coefs.v[1]*yg;
	result.m[1][2] = coefs.v[2]*yb;

	result.m[2][0] = coefs.v[0]*(1.-xr-yr);
	result.m[2][1] = coefs.v[1]*(1.-xg-yg);
	result.m[2][2] = coefs.v[2]*(1.-xb-yb);
	result.invalid = primaries_invert.invalid;

	return result;
}

struct CIE_XYZ {
	double X;
	double Y;
	double Z;
};


static const struct CIE_XYZ D50_XYZ = {
	0.9642,
	1.0000,
	0.8249
};



static struct CIE_XYZ xyY2XYZ(qcms_CIE_xyY source)
{
	struct CIE_XYZ dest;
	dest.X = (source.x / source.y) * source.Y;
	dest.Y = source.Y;
	dest.Z = ((1 - source.x - source.y) / source.y) * source.Y;
	return dest;
}



static struct matrix
compute_chromatic_adaption(struct CIE_XYZ source_white_point,
                           struct CIE_XYZ dest_white_point,
                           struct matrix chad)
{
	struct matrix chad_inv;
	struct vector cone_source_XYZ, cone_source_rgb;
	struct vector cone_dest_XYZ, cone_dest_rgb;
	struct matrix cone, tmp;

	tmp = chad;
	chad_inv = matrix_invert(tmp);

	cone_source_XYZ.v[0] = source_white_point.X;
	cone_source_XYZ.v[1] = source_white_point.Y;
	cone_source_XYZ.v[2] = source_white_point.Z;

	cone_dest_XYZ.v[0] = dest_white_point.X;
	cone_dest_XYZ.v[1] = dest_white_point.Y;
	cone_dest_XYZ.v[2] = dest_white_point.Z;

	cone_source_rgb = matrix_eval(chad, cone_source_XYZ);
	cone_dest_rgb   = matrix_eval(chad, cone_dest_XYZ);

	cone.m[0][0] = cone_dest_rgb.v[0]/cone_source_rgb.v[0];
	cone.m[0][1] = 0;
	cone.m[0][2] = 0;
	cone.m[1][0] = 0;
	cone.m[1][1] = cone_dest_rgb.v[1]/cone_source_rgb.v[1];
	cone.m[1][2] = 0;
	cone.m[2][0] = 0;
	cone.m[2][1] = 0;
	cone.m[2][2] = cone_dest_rgb.v[2]/cone_source_rgb.v[2];
	cone.invalid = false;

	
	return matrix_multiply(chad_inv, matrix_multiply(cone, chad));
}




static struct matrix
adaption_matrix(struct CIE_XYZ source_illumination, struct CIE_XYZ target_illumination)
{
	struct matrix lam_rigg = {{ 
	                         {  0.8951,  0.2664, -0.1614 },
	                         { -0.7502,  1.7135,  0.0367 },
	                         {  0.0389, -0.0685,  1.0296 }
	                         }};
	return compute_chromatic_adaption(source_illumination, target_illumination, lam_rigg);
}


static struct matrix adapt_matrix_to_D50(struct matrix r, qcms_CIE_xyY source_white_pt)
{
	struct CIE_XYZ Dn;
	struct matrix Bradford;

	if (source_white_pt.y == 0.0)
		return matrix_invalid();

	Dn = xyY2XYZ(source_white_pt);

	Bradford = adaption_matrix(Dn, D50_XYZ);
	return matrix_multiply(Bradford, r);
}

qcms_bool set_rgb_colorants(qcms_profile *profile, qcms_CIE_xyY white_point, qcms_CIE_xyYTRIPLE primaries)
{
	struct matrix colorants;
	colorants = build_RGB_to_XYZ_transfer_matrix(white_point, primaries);
	colorants = adapt_matrix_to_D50(colorants, white_point);

	if (colorants.invalid)
		return false;

	
	profile->redColorant.X = double_to_s15Fixed16Number(colorants.m[0][0]);
	profile->redColorant.Y = double_to_s15Fixed16Number(colorants.m[1][0]);
	profile->redColorant.Z = double_to_s15Fixed16Number(colorants.m[2][0]);

	profile->greenColorant.X = double_to_s15Fixed16Number(colorants.m[0][1]);
	profile->greenColorant.Y = double_to_s15Fixed16Number(colorants.m[1][1]);
	profile->greenColorant.Z = double_to_s15Fixed16Number(colorants.m[2][1]);

	profile->blueColorant.X = double_to_s15Fixed16Number(colorants.m[0][2]);
	profile->blueColorant.Y = double_to_s15Fixed16Number(colorants.m[1][2]);
	profile->blueColorant.Z = double_to_s15Fixed16Number(colorants.m[2][2]);

	return true;
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

static float clamp_float(float a)
{
	if (a > 1.)
		return 1.;
	else if (a < 0)
		return 0;
	else
		return a;
}

#if 0
static void qcms_transform_data_rgb_out_pow(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	int i;
	float (*mat)[4] = transform->matrix;
	for (i=0; i<length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		float out_device_r = pow(out_linear_r, transform->out_gamma_r);
		float out_device_g = pow(out_linear_g, transform->out_gamma_g);
		float out_device_b = pow(out_linear_b, transform->out_gamma_b);

		*dest++ = clamp_u8(255*out_device_r);
		*dest++ = clamp_u8(255*out_device_g);
		*dest++ = clamp_u8(255*out_device_b);
	}
}
#endif

static void qcms_transform_data_gray_out_lut(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	for (i = 0; i < length; i++) {
		float out_device_r, out_device_g, out_device_b;
		unsigned char device = *src++;

		float linear = transform->input_gamma_table_gray[device];

                out_device_r = lut_interp_linear(linear, transform->output_gamma_lut_r, transform->output_gamma_lut_r_length);
		out_device_g = lut_interp_linear(linear, transform->output_gamma_lut_g, transform->output_gamma_lut_g_length);
		out_device_b = lut_interp_linear(linear, transform->output_gamma_lut_b, transform->output_gamma_lut_b_length);

		*dest++ = clamp_u8(out_device_r*255);
		*dest++ = clamp_u8(out_device_g*255);
		*dest++ = clamp_u8(out_device_b*255);
	}
}







static void qcms_transform_data_graya_out_lut(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	for (i = 0; i < length; i++) {
		float out_device_r, out_device_g, out_device_b;
		unsigned char device = *src++;
		unsigned char alpha = *src++;

		float linear = transform->input_gamma_table_gray[device];

                out_device_r = lut_interp_linear(linear, transform->output_gamma_lut_r, transform->output_gamma_lut_r_length);
		out_device_g = lut_interp_linear(linear, transform->output_gamma_lut_g, transform->output_gamma_lut_g_length);
		out_device_b = lut_interp_linear(linear, transform->output_gamma_lut_b, transform->output_gamma_lut_b_length);

		*dest++ = clamp_u8(out_device_r*255);
		*dest++ = clamp_u8(out_device_g*255);
		*dest++ = clamp_u8(out_device_b*255);
		*dest++ = alpha;
	}
}


static void qcms_transform_data_gray_out_precache(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	for (i = 0; i < length; i++) {
		unsigned char device = *src++;
		uint16_t gray;

		float linear = transform->input_gamma_table_gray[device];

		
		gray = linear * 65535.;

		*dest++ = transform->output_table_r->data[gray];
		*dest++ = transform->output_table_g->data[gray];
		*dest++ = transform->output_table_b->data[gray];
	}
}

static void qcms_transform_data_graya_out_precache(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	for (i = 0; i < length; i++) {
		unsigned char device = *src++;
		unsigned char alpha = *src++;
		uint16_t gray;

		float linear = transform->input_gamma_table_gray[device];

		
		gray = linear * 65535.;

		*dest++ = transform->output_table_r->data[gray];
		*dest++ = transform->output_table_g->data[gray];
		*dest++ = transform->output_table_b->data[gray];
		*dest++ = alpha;
	}
}

static void qcms_transform_data_rgb_out_lut_precache(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	for (i = 0; i < length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;
		uint16_t r, g, b;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		out_linear_r = clamp_float(out_linear_r);
		out_linear_g = clamp_float(out_linear_g);
		out_linear_b = clamp_float(out_linear_b);

		
		r = out_linear_r * 65535.;
		g = out_linear_g * 65535.;
		b = out_linear_b * 65535.;

		*dest++ = transform->output_table_r->data[r];
		*dest++ = transform->output_table_g->data[g];
		*dest++ = transform->output_table_b->data[b];
	}
}

static void qcms_transform_data_rgba_out_lut_precache(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	for (i = 0; i < length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;
		unsigned char alpha = *src++;
		uint16_t r, g, b;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		out_linear_r = clamp_float(out_linear_r);
		out_linear_g = clamp_float(out_linear_g);
		out_linear_b = clamp_float(out_linear_b);

		
		r = out_linear_r * 65535.;
		g = out_linear_g * 65535.;
		b = out_linear_b * 65535.;

		*dest++ = transform->output_table_r->data[r];
		*dest++ = transform->output_table_g->data[g];
		*dest++ = transform->output_table_b->data[b];
		*dest++ = alpha;
	}
}

static void qcms_transform_data_rgb_out_lut(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	for (i = 0; i < length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;
		float out_device_r, out_device_g, out_device_b;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		out_linear_r = clamp_float(out_linear_r);
		out_linear_g = clamp_float(out_linear_g);
		out_linear_b = clamp_float(out_linear_b);

		out_device_r = lut_interp_linear(out_linear_r, transform->output_gamma_lut_r, transform->output_gamma_lut_r_length);
		out_device_g = lut_interp_linear(out_linear_g, transform->output_gamma_lut_g, transform->output_gamma_lut_g_length);
		out_device_b = lut_interp_linear(out_linear_b, transform->output_gamma_lut_b, transform->output_gamma_lut_b_length);

		*dest++ = clamp_u8(out_device_r*255);
		*dest++ = clamp_u8(out_device_g*255);
		*dest++ = clamp_u8(out_device_b*255);
	}
}

static void qcms_transform_data_rgba_out_lut(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	unsigned int i;
	float (*mat)[4] = transform->matrix;
	for (i = 0; i < length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;
		unsigned char alpha = *src++;
		float out_device_r, out_device_g, out_device_b;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		out_linear_r = clamp_float(out_linear_r);
		out_linear_g = clamp_float(out_linear_g);
		out_linear_b = clamp_float(out_linear_b);

		out_device_r = lut_interp_linear(out_linear_r, transform->output_gamma_lut_r, transform->output_gamma_lut_r_length);
		out_device_g = lut_interp_linear(out_linear_g, transform->output_gamma_lut_g, transform->output_gamma_lut_g_length);
		out_device_b = lut_interp_linear(out_linear_b, transform->output_gamma_lut_b, transform->output_gamma_lut_b_length);

		*dest++ = clamp_u8(out_device_r*255);
		*dest++ = clamp_u8(out_device_g*255);
		*dest++ = clamp_u8(out_device_b*255);
		*dest++ = alpha;
	}
}

#if 0
static void qcms_transform_data_rgb_out_linear(qcms_transform *transform, unsigned char *src, unsigned char *dest, size_t length)
{
	int i;
	float (*mat)[4] = transform->matrix;
	for (i = 0; i < length; i++) {
		unsigned char device_r = *src++;
		unsigned char device_g = *src++;
		unsigned char device_b = *src++;

		float linear_r = transform->input_gamma_table_r[device_r];
		float linear_g = transform->input_gamma_table_g[device_g];
		float linear_b = transform->input_gamma_table_b[device_b];

		float out_linear_r = mat[0][0]*linear_r + mat[1][0]*linear_g + mat[2][0]*linear_b;
		float out_linear_g = mat[0][1]*linear_r + mat[1][1]*linear_g + mat[2][1]*linear_b;
		float out_linear_b = mat[0][2]*linear_r + mat[1][2]*linear_g + mat[2][2]*linear_b;

		*dest++ = clamp_u8(out_linear_r*255);
		*dest++ = clamp_u8(out_linear_g*255);
		*dest++ = clamp_u8(out_linear_b*255);
	}
}
#endif

static struct precache_output *precache_reference(struct precache_output *p)
{
	p->ref_count++;
	return p;
}

static struct precache_output *precache_create()
{
	struct precache_output *p = malloc(sizeof(struct precache_output));
	if (p)
		p->ref_count = 1;
	return p;
}

void precache_release(struct precache_output *p)
{
	if (--p->ref_count == 0) {
		free(p);
	}
}

#ifdef HAS_POSIX_MEMALIGN
static qcms_transform *transform_alloc(void)
{
	qcms_transform *t;
	if (!posix_memalign(&t, 16, sizeof(*t))) {
		return t;
	} else {
		return NULL;
	}
}
static void transform_free(qcms_transform *t)
{
	free(t);
}
#else
static qcms_transform *transform_alloc(void)
{
	
	char *original_block = calloc(sizeof(qcms_transform) + sizeof(void*) + 16, 1);
	
	void *transform_start = original_block + sizeof(void*);
	
	qcms_transform *transform_aligned = (qcms_transform*)(((uintptr_t)transform_start + 15) & ~0xf);

	
	void **(original_block_ptr) = (void**)transform_aligned;
	if (!original_block)
		return NULL;
	original_block_ptr--;
	*original_block_ptr = original_block;

	return transform_aligned;
}
static void transform_free(qcms_transform *t)
{
	
	void **p = (void**)t;
	p--;
	free(*p);
}
#endif

void qcms_transform_release(qcms_transform *t)
{
	


	if (t->output_table_r)
		precache_release(t->output_table_r);
	if (t->output_table_g)
		precache_release(t->output_table_g);
	if (t->output_table_b)
		precache_release(t->output_table_b);

	free(t->input_gamma_table_r);
	if (t->input_gamma_table_g != t->input_gamma_table_r)
		free(t->input_gamma_table_g);
	if (t->input_gamma_table_g != t->input_gamma_table_r &&
	    t->input_gamma_table_g != t->input_gamma_table_b)
		free(t->input_gamma_table_b);

	free(t->input_gamma_table_gray);

	free(t->output_gamma_lut_r);
	free(t->output_gamma_lut_g);
	free(t->output_gamma_lut_b);

	transform_free(t);
}

static void compute_precache_pow(uint8_t *output, float gamma)
{
	uint32_t v = 0;
	for (v = 0; v <= 0xffff; v++) {
		
		output[v] = 255.f * powf(v/65535.f, gamma);
	}
}

void compute_precache_lut(uint8_t *output, uint16_t *table, int length)
{
	uint32_t v = 0;
	for (v = 0; v <= 0xffff; v++) {
		
		output[v] = lut_interp_linear16(v, table, length) >> 8;
	}
}

void compute_precache_linear(uint8_t *output)
{
	uint32_t v = 0;
	for (v = 0; v <= 0xffff; v++) {
		
		output[v] = v >> 8;
	}
}

qcms_bool compute_precache(struct curveType *trc, uint8_t *output)
{
	if (trc->count == 0) {
		compute_precache_linear(output);
	} else if (trc->count == 1) {
		compute_precache_pow(output, 1.f/u8Fixed8Number_to_float(trc->data[0]));
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
	return true;
}

#ifdef X86


 
#if defined(_M_IX86) && defined(_MSC_VER)
#define HAS_CPUID




static void cpuid(uint32_t fxn, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
       uint32_t a_, b_, c_, d_;
       __asm {
              xchg   ebx, esi
              mov    eax, fxn
              cpuid
              mov    a_, eax
              mov    b_, ebx
              mov    c_, ecx
              mov    d_, edx
              xchg   ebx, esi
       }
       *a = a_;
       *b = b_;
       *c = c_;
       *d = d_;
}
#elif defined(__GNUC__) && defined(__i386__)
#define HAS_CPUID


static void cpuid(uint32_t fxn, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {

	uint32_t a_, b_, c_, d_;
       __asm__ __volatile__ ("xchgl %%ebx, %%esi; cpuid; xchgl %%ebx, %%esi;" 
                             : "=a" (a_), "=S" (b_), "=c" (c_), "=d" (d_) : "a" (fxn));
	   *a = a_;
	   *b = b_;
	   *c = c_;
	   *d = d_;
}
#endif





#define SSE1_EDX_MASK (1UL << 25)
#define SSE2_EDX_MASK (1UL << 26)
#define SSE3_ECX_MASK (1UL <<  0)

static int sse_version_available(void)
{
#if defined(__x86_64__) || defined(_M_AMD64)
	


	return 2;
#elif defined(HAS_CPUID)
	static int sse_version = -1;
	uint32_t a, b, c, d;
	uint32_t function = 0x00000001;

	if (sse_version == -1) {
		sse_version = 0;
		cpuid(function, &a, &b, &c, &d);
		if (c & SSE3_ECX_MASK)
			sse_version = 3;
		else if (d & SSE2_EDX_MASK)
			sse_version = 2;
		else if (d & SSE1_EDX_MASK)
			sse_version = 1;
	}

	return sse_version;
#else
	return 0;
#endif
}
#endif

void build_output_lut(struct curveType *trc,
		uint16_t **output_gamma_lut, size_t *output_gamma_lut_length)
{
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

void qcms_profile_precache_output_transform(qcms_profile *profile)
{
	
	if (profile->color_space != RGB_SIGNATURE)
		return;

	if (!profile->output_table_r) {
		profile->output_table_r = precache_create();
		if (profile->output_table_r &&
				!compute_precache(profile->redTRC, profile->output_table_r->data)) {
			precache_release(profile->output_table_r);
			profile->output_table_r = NULL;
		}
	}
	if (!profile->output_table_g) {
		profile->output_table_g = precache_create();
		if (profile->output_table_g &&
				!compute_precache(profile->greenTRC, profile->output_table_g->data)) {
			precache_release(profile->output_table_g);
			profile->output_table_g = NULL;
		}
	}
	if (!profile->output_table_b) {
		profile->output_table_b = precache_create();
		if (profile->output_table_b &&
				!compute_precache(profile->blueTRC, profile->output_table_b->data)) {
			precache_release(profile->output_table_g);
			profile->output_table_g = NULL;
		}
	}
}

#define NO_MEM_TRANSFORM NULL

qcms_transform* qcms_transform_create(
		qcms_profile *in, qcms_data_type in_type,
		qcms_profile* out, qcms_data_type out_type,
		qcms_intent intent)
{
	bool precache = false;

        qcms_transform *transform = transform_alloc();
        if (!transform) {
		return NULL;
	}
	if (out_type != QCMS_DATA_RGB_8 &&
                out_type != QCMS_DATA_RGBA_8) {
            assert(0 && "output type");
	    free(transform);
            return NULL;
        }

	if (out->output_table_r &&
			out->output_table_g &&
			out->output_table_b) {
		precache = true;
	}

	if (precache) {
		transform->output_table_r = precache_reference(out->output_table_r);
		transform->output_table_g = precache_reference(out->output_table_g);
		transform->output_table_b = precache_reference(out->output_table_b);
	} else {
		build_output_lut(out->redTRC, &transform->output_gamma_lut_r, &transform->output_gamma_lut_r_length);
		build_output_lut(out->greenTRC, &transform->output_gamma_lut_g, &transform->output_gamma_lut_g_length);
		build_output_lut(out->blueTRC, &transform->output_gamma_lut_b, &transform->output_gamma_lut_b_length);
		if (!transform->output_gamma_lut_r || !transform->output_gamma_lut_g || !transform->output_gamma_lut_b) {
			qcms_transform_release(transform);
			return NO_MEM_TRANSFORM;
		}
	}

        if (in->color_space == RGB_SIGNATURE) {
            struct matrix in_matrix, out_matrix, result;

            if (in_type != QCMS_DATA_RGB_8 &&
                    in_type != QCMS_DATA_RGBA_8){
                assert(0 && "input type");
		free(transform);
                return NULL;
            }
	    if (precache) {
#ifdef X86
		    if (sse_version_available() >= 2) {
			    if (in_type == QCMS_DATA_RGB_8)
				    transform->transform_fn = qcms_transform_data_rgb_out_lut_sse2;
			    else
				    transform->transform_fn = qcms_transform_data_rgba_out_lut_sse2;

		    } else
		    if (sse_version_available() >= 1) {
			    if (in_type == QCMS_DATA_RGB_8)
				    transform->transform_fn = qcms_transform_data_rgb_out_lut_sse1;
			    else
				    transform->transform_fn = qcms_transform_data_rgba_out_lut_sse1;

		    } else
#endif
		    {
			    if (in_type == QCMS_DATA_RGB_8)
				    transform->transform_fn = qcms_transform_data_rgb_out_lut_precache;
			    else
				    transform->transform_fn = qcms_transform_data_rgba_out_lut_precache;
		    }
	    } else {
		    if (in_type == QCMS_DATA_RGB_8)
			    transform->transform_fn = qcms_transform_data_rgb_out_lut;
		    else
			    transform->transform_fn = qcms_transform_data_rgba_out_lut;
	    }

            
            transform->input_gamma_table_r = build_input_gamma_table(in->redTRC);
            transform->input_gamma_table_g = build_input_gamma_table(in->greenTRC);
            transform->input_gamma_table_b = build_input_gamma_table(in->blueTRC);

	    if (!transform->input_gamma_table_r || !transform->input_gamma_table_g || !transform->input_gamma_table_b) {
		    qcms_transform_release(transform);
		    return NO_MEM_TRANSFORM;
	    }

            
            in_matrix = build_colorant_matrix(in);
            out_matrix = build_colorant_matrix(out);
            out_matrix = matrix_invert(out_matrix);
            if (out_matrix.invalid) {
                qcms_transform_release(transform);
                return NULL;
            }
            result = matrix_multiply(out_matrix, in_matrix);

            

            transform->matrix[0][0] = result.m[0][0];
            transform->matrix[1][0] = result.m[0][1];
            transform->matrix[2][0] = result.m[0][2];
            transform->matrix[0][1] = result.m[1][0];
            transform->matrix[1][1] = result.m[1][1];
            transform->matrix[2][1] = result.m[1][2];
            transform->matrix[0][2] = result.m[2][0];
            transform->matrix[1][2] = result.m[2][1];
            transform->matrix[2][2] = result.m[2][2];

        } else if (in->color_space == GRAY_SIGNATURE) {
            if (in_type != QCMS_DATA_GRAY_8 &&
                    in_type != QCMS_DATA_GRAYA_8){
                assert(0 && "input type");
		free(transform);
                return NULL;
            }

            transform->input_gamma_table_gray = build_input_gamma_table(in->grayTRC);
	    if (!transform->input_gamma_table_gray) {
		    qcms_transform_release(transform);
		    return NO_MEM_TRANSFORM;
	    }

	    if (precache) {
		    if (in_type == QCMS_DATA_GRAY_8) {
			    transform->transform_fn = qcms_transform_data_gray_out_precache;
		    } else {
			    transform->transform_fn = qcms_transform_data_graya_out_precache;
		    }
	    } else {
		    if (in_type == QCMS_DATA_GRAY_8) {
			    transform->transform_fn = qcms_transform_data_gray_out_lut;
		    } else {
			    transform->transform_fn = qcms_transform_data_graya_out_lut;
		    }
	    }
	} else {
		assert(0 && "unexpected colorspace");
	}
	return transform;
}

#if defined(__GNUC__) && !defined(__x86_64__) && !defined(__amd64__)

__attribute__((__force_align_arg_pointer__))
#endif
void qcms_transform_data(qcms_transform *transform, void *src, void *dest, size_t length)
{
	transform->transform_fn(transform, src, dest, length);
}
