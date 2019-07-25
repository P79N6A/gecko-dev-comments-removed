






















#include <stdlib.h>
#include "qcmsint.h"
#include "matrix.h"

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

struct matrix matrix_invalid(void)
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


