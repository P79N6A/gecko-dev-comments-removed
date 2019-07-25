






















#ifndef _QCMS_MATRIX_H
#define _QCMS_MATRIX_H

struct vector {
        float v[3];
};

struct vector matrix_eval(struct matrix mat, struct vector v);
float matrix_det(struct matrix mat);
struct matrix matrix_identity(void);
struct matrix matrix_multiply(struct matrix a, struct matrix b);
struct matrix matrix_invert(struct matrix mat);

struct matrix matrix_invalid(void);

#endif
