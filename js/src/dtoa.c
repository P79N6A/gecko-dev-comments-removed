


































































































































































#ifndef Long
#define Long long
#endif
#ifndef ULong
typedef unsigned Long ULong;
#endif

#ifdef DEBUG
#include "stdio.h"
#define Bug(x) {fprintf(stderr, "%s\n", x); exit(1);}
#endif

#include "stdlib.h"
#include "string.h"

#ifdef USE_LOCALE
#include "locale.h"
#endif

#ifdef MALLOC
#ifdef KR_headers
extern char *MALLOC();
#else
extern void *MALLOC(size_t);
#endif
#else
#define MALLOC malloc
#endif

#ifndef Omit_Private_Memory
#ifndef PRIVATE_MEM
#define PRIVATE_MEM 2304
#endif
#define PRIVATE_mem ((PRIVATE_MEM+sizeof(double)-1)/sizeof(double))
static double private_mem[PRIVATE_mem], *pmem_next = private_mem;
#endif

#undef IEEE_Arith
#undef Avoid_Underflow
#ifdef IEEE_MC68k
#define IEEE_Arith
#endif
#ifdef IEEE_8087
#define IEEE_Arith
#endif

#ifdef IEEE_Arith
#ifndef NO_INFNAN_CHECK
#undef INFNAN_CHECK
#define INFNAN_CHECK
#endif
#else
#undef INFNAN_CHECK
#endif

#include "errno.h"

#ifdef Bad_float_h

#ifdef IEEE_Arith
#define DBL_DIG 15
#define DBL_MAX_10_EXP 308
#define DBL_MAX_EXP 1024
#define FLT_RADIX 2
#endif 

#ifdef IBM
#define DBL_DIG 16
#define DBL_MAX_10_EXP 75
#define DBL_MAX_EXP 63
#define FLT_RADIX 16
#define DBL_MAX 7.2370055773322621e+75
#endif

#ifdef VAX
#define DBL_DIG 16
#define DBL_MAX_10_EXP 38
#define DBL_MAX_EXP 127
#define FLT_RADIX 2
#define DBL_MAX 1.7014118346046923e+38
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647
#endif

#else 
#include "float.h"
#endif 

#ifndef __MATH_H__
#include "math.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONST
#ifdef KR_headers
#define CONST
#else
#define CONST const
#endif
#endif

#if defined(IEEE_8087) + defined(IEEE_MC68k) + defined(VAX) + defined(IBM) != 1
Exactly one of IEEE_8087, IEEE_MC68k, VAX, or IBM should be defined.
#endif

typedef union { double d; ULong L[2]; } U;

#define dval(x) ((x).d)
#ifdef IEEE_8087
#define word0(x) ((x).L[1])
#define word1(x) ((x).L[0])
#else
#define word0(x) ((x).L[0])
#define word1(x) ((x).L[1])
#endif





#if defined(IEEE_8087) + defined(VAX)
#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)
#else
#define Storeinc(a,b,c) (((unsigned short *)a)[0] = (unsigned short)b, \
((unsigned short *)a)[1] = (unsigned short)c, a++)
#endif







#ifdef IEEE_Arith
#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    0x100000
#define Exp_msk11   0x100000
#define Exp_mask  0x7ff00000
#define P 53
#define Bias 1023
#define Emin (-1022)
#define Exp_1  0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask  0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask  0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14
#ifndef NO_IEEE_Scale
#define Avoid_Underflow
#ifdef Flush_Denorm	
#undef Sudden_Underflow
#endif
#endif

#ifndef Flt_Rounds
#ifdef FLT_ROUNDS
#define Flt_Rounds FLT_ROUNDS
#else
#define Flt_Rounds 1
#endif
#endif 

#ifdef Honor_FLT_ROUNDS
#define Rounding rounding
#undef Check_FLT_ROUNDS
#define Check_FLT_ROUNDS
#else
#define Rounding Flt_Rounds
#endif

#else 
#undef Check_FLT_ROUNDS
#undef Honor_FLT_ROUNDS
#undef SET_INEXACT
#undef  Sudden_Underflow
#define Sudden_Underflow
#ifdef IBM
#undef Flt_Rounds
#define Flt_Rounds 0
#define Exp_shift  24
#define Exp_shift1 24
#define Exp_msk1   0x1000000
#define Exp_msk11  0x1000000
#define Exp_mask  0x7f000000
#define P 14
#define Bias 65
#define Exp_1  0x41000000
#define Exp_11 0x41000000
#define Ebits 8	/* exponent has 7 bits, but 8 is the right value in b2d */
#define Frac_mask  0xffffff
#define Frac_mask1 0xffffff
#define Bletch 4
#define Ten_pmax 22
#define Bndry_mask  0xefffff
#define Bndry_mask1 0xffffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 4
#define Tiny0 0x100000
#define Tiny1 0
#define Quick_max 14
#define Int_max 15
#else 
#undef Flt_Rounds
#define Flt_Rounds 1
#define Exp_shift  23
#define Exp_shift1 7
#define Exp_msk1    0x80
#define Exp_msk11   0x800000
#define Exp_mask  0x7f80
#define P 56
#define Bias 129
#define Exp_1  0x40800000
#define Exp_11 0x4080
#define Ebits 8
#define Frac_mask  0x7fffff
#define Frac_mask1 0xffff007f
#define Ten_pmax 24
#define Bletch 2
#define Bndry_mask  0xffff007f
#define Bndry_mask1 0xffff007f
#define LSB 0x10000
#define Sign_bit 0x8000
#define Log2P 1
#define Tiny0 0x80
#define Tiny1 0
#define Quick_max 15
#define Int_max 15
#endif 
#endif 

#ifndef IEEE_Arith
#define ROUND_BIASED
#endif

#ifdef RND_PRODQUOT
#define rounded_product(a,b) a = rnd_prod(a, b)
#define rounded_quotient(a,b) a = rnd_quot(a, b)
#ifdef KR_headers
extern double rnd_prod(), rnd_quot();
#else
extern double rnd_prod(double, double), rnd_quot(double, double);
#endif
#else
#define rounded_product(a,b) a *= b
#define rounded_quotient(a,b) a /= b
#endif

#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#define Big1 0xffffffff

#ifndef Pack_32
#define Pack_32
#endif

#ifdef KR_headers
#define FFFFFFFF ((((unsigned long)0xffff)<<16)|(unsigned long)0xffff)
#else
#define FFFFFFFF 0xffffffffUL
#endif

#ifdef NO_LONG_LONG
#undef ULLong
#ifdef Just_16
#undef Pack_32





#endif
#else	
#ifndef Llong
#define Llong long long
#endif
#ifndef ULLong
#define ULLong unsigned Llong
#endif
#endif 

#ifndef MULTIPLE_THREADS
#define ACQUIRE_DTOA_LOCK(n)
#define FREE_DTOA_LOCK(n)
#endif

#define Kmax 15

 struct
Bigint {
	struct Bigint *next;
	int k, maxwds, sign, wds;
	ULong x[1];
	};

 typedef struct Bigint Bigint;

 static Bigint *freelist[Kmax+1];

 static Bigint *
Balloc
#ifdef KR_headers
	(k) int k;
#else
	(int k)
#endif
{
	int x;
	Bigint *rv;
#ifndef Omit_Private_Memory
	size_t len;
#endif

	ACQUIRE_DTOA_LOCK(0);
	if ((rv = freelist[k])) {
		freelist[k] = rv->next;
		}
	else {
		x = 1 << k;
#ifdef Omit_Private_Memory
		rv = (Bigint *)MALLOC(sizeof(Bigint) + (x-1)*sizeof(ULong));
#else
		len = (sizeof(Bigint) + (x-1)*sizeof(ULong) + sizeof(double) - 1)
			/sizeof(double);
		if (pmem_next - private_mem + len <= PRIVATE_mem) {
			rv = (Bigint*)pmem_next;
			pmem_next += len;
			}
		else
			rv = (Bigint*)MALLOC(len*sizeof(double));
#endif
		rv->k = k;
		rv->maxwds = x;
		}
	FREE_DTOA_LOCK(0);
	rv->sign = rv->wds = 0;
	return rv;
	}

 static void
Bfree
#ifdef KR_headers
	(v) Bigint *v;
#else
	(Bigint *v)
#endif
{
	if (v) {
		ACQUIRE_DTOA_LOCK(0);
		v->next = freelist[v->k];
		freelist[v->k] = v;
		FREE_DTOA_LOCK(0);
		}
	}

#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign, \
y->wds*sizeof(Long) + 2*sizeof(int))

 static Bigint *
multadd
#ifdef KR_headers
	(b, m, a) Bigint *b; int m, a;
#else
	(Bigint *b, int m, int a)	
#endif
{
	int i, wds;
#ifdef ULLong
	ULong *x;
	ULLong carry, y;
#else
	ULong carry, *x, y;
#ifdef Pack_32
	ULong xi, z;
#endif
#endif
	Bigint *b1;

	wds = b->wds;
	x = b->x;
	i = 0;
	carry = a;
	do {
#ifdef ULLong
		y = *x * (ULLong)m + carry;
		carry = y >> 32;
		*x++ = (ULong) y & FFFFFFFF;
#else
#ifdef Pack_32
		xi = *x;
		y = (xi & 0xffff) * m + carry;
		z = (xi >> 16) * m + (y >> 16);
		carry = z >> 16;
		*x++ = (z << 16) + (y & 0xffff);
#else
		y = *x * m + carry;
		carry = y >> 16;
		*x++ = y & 0xffff;
#endif
#endif
		}
		while(++i < wds);
	if (carry) {
		if (wds >= b->maxwds) {
			b1 = Balloc(b->k+1);
			Bcopy(b1, b);
			Bfree(b);
			b = b1;
			}
		b->x[wds++] = (ULong) carry;
		b->wds = wds;
		}
	return b;
	}

 static Bigint *
s2b
#ifdef KR_headers
	(s, nd0, nd, y9) CONST char *s; int nd0, nd; ULong y9;
#else
	(CONST char *s, int nd0, int nd, ULong y9)
#endif
{
	Bigint *b;
	int i, k;
	Long x, y;

	x = (nd + 8) / 9;
	for(k = 0, y = 1; x > y; y <<= 1, k++) ;
#ifdef Pack_32
	b = Balloc(k);
	b->x[0] = y9;
	b->wds = 1;
#else
	b = Balloc(k+1);
	b->x[0] = y9 & 0xffff;
	b->wds = (b->x[1] = y9 >> 16) ? 2 : 1;
#endif

	i = 9;
	if (9 < nd0) {
		s += 9;
		do b = multadd(b, 10, *s++ - '0');
			while(++i < nd0);
		s++;
		}
	else
		s += 10;
	for(; i < nd; i++)
		b = multadd(b, 10, *s++ - '0');
	return b;
	}

 static int
hi0bits
#ifdef KR_headers
	(x) register ULong x;
#else
	(register ULong x)
#endif
{
	register int k = 0;

	if (!(x & 0xffff0000)) {
		k = 16;
		x <<= 16;
		}
	if (!(x & 0xff000000)) {
		k += 8;
		x <<= 8;
		}
	if (!(x & 0xf0000000)) {
		k += 4;
		x <<= 4;
		}
	if (!(x & 0xc0000000)) {
		k += 2;
		x <<= 2;
		}
	if (!(x & 0x80000000)) {
		k++;
		if (!(x & 0x40000000))
			return 32;
		}
	return k;
	}

 static int
lo0bits
#ifdef KR_headers
	(y) ULong *y;
#else
	(ULong *y)
#endif
{
	register int k;
	register ULong x = *y;

	if (x & 7) {
		if (x & 1)
			return 0;
		if (x & 2) {
			*y = x >> 1;
			return 1;
			}
		*y = x >> 2;
		return 2;
		}
	k = 0;
	if (!(x & 0xffff)) {
		k = 16;
		x >>= 16;
		}
	if (!(x & 0xff)) {
		k += 8;
		x >>= 8;
		}
	if (!(x & 0xf)) {
		k += 4;
		x >>= 4;
		}
	if (!(x & 0x3)) {
		k += 2;
		x >>= 2;
		}
	if (!(x & 1)) {
		k++;
		x >>= 1;
		if (!x)
			return 32;
		}
	*y = x;
	return k;
	}

 static Bigint *
i2b
#ifdef KR_headers
	(i) int i;
#else
	(int i)
#endif
{
	Bigint *b;

	b = Balloc(1);
	b->x[0] = i;
	b->wds = 1;
	return b;
	}

 static Bigint *
mult
#ifdef KR_headers
	(a, b) Bigint *a, *b;
#else
	(Bigint *a, Bigint *b)
#endif
{
	Bigint *c;
	int k, wa, wb, wc;
	ULong *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
	ULong y;
#ifdef ULLong
	ULLong carry, z;
#else
	ULong carry, z;
#ifdef Pack_32
	ULong z2;
#endif
#endif

	if (a->wds < b->wds) {
		c = a;
		a = b;
		b = c;
		}
	k = a->k;
	wa = a->wds;
	wb = b->wds;
	wc = wa + wb;
	if (wc > a->maxwds)
		k++;
	c = Balloc(k);
	for(x = c->x, xa = x + wc; x < xa; x++)
		*x = 0;
	xa = a->x;
	xae = xa + wa;
	xb = b->x;
	xbe = xb + wb;
	xc0 = c->x;
#ifdef ULLong
	for(; xb < xbe; xc0++) {
		if ((y = *xb++)) {
			x = xa;
			xc = xc0;
			carry = 0;
			do {
				z = *x++ * (ULLong)y + *xc + carry;
				carry = z >> 32;
				*xc++ = (ULong) z & FFFFFFFF;
				}
				while(x < xae);
			*xc = (ULong) carry;
			}
		}
#else
#ifdef Pack_32
	for(; xb < xbe; xb++, xc0++) {
		if (y = *xb & 0xffff) {
			x = xa;
			xc = xc0;
			carry = 0;
			do {
				z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
				carry = z >> 16;
				z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
				carry = z2 >> 16;
				Storeinc(xc, z2, z);
				}
				while(x < xae);
			*xc = carry;
			}
		if (y = *xb >> 16) {
			x = xa;
			xc = xc0;
			carry = 0;
			z2 = *xc;
			do {
				z = (*x & 0xffff) * y + (*xc >> 16) + carry;
				carry = z >> 16;
				Storeinc(xc, z, z2);
				z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
				carry = z2 >> 16;
				}
				while(x < xae);
			*xc = z2;
			}
		}
#else
	for(; xb < xbe; xc0++) {
		if (y = *xb++) {
			x = xa;
			xc = xc0;
			carry = 0;
			do {
				z = *x++ * y + *xc + carry;
				carry = z >> 16;
				*xc++ = z & 0xffff;
				}
				while(x < xae);
			*xc = carry;
			}
		}
#endif
#endif
	for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
	c->wds = wc;
	return c;
	}

 static Bigint *p5s;

 static Bigint *
pow5mult
#ifdef KR_headers
	(b, k) Bigint *b; int k;
#else
	(Bigint *b, int k)
#endif
{
	Bigint *b1, *p5, *p51;
	int i;
	static int p05[3] = { 5, 25, 125 };

	if ((i = k & 3))
		b = multadd(b, p05[i-1], 0);

	if (!(k >>= 2))
		return b;
	if (!(p5 = p5s)) {
		
#ifdef MULTIPLE_THREADS
		ACQUIRE_DTOA_LOCK(1);
		if (!(p5 = p5s)) {
			p5 = p5s = i2b(625);
			p5->next = 0;
			}
		FREE_DTOA_LOCK(1);
#else
		p5 = p5s = i2b(625);
		p5->next = 0;
#endif
		}
	for(;;) {
		if (k & 1) {
			b1 = mult(b, p5);
			Bfree(b);
			b = b1;
			}
		if (!(k >>= 1))
			break;
		if (!(p51 = p5->next)) {
#ifdef MULTIPLE_THREADS
			ACQUIRE_DTOA_LOCK(1);
			if (!(p51 = p5->next)) {
				p51 = p5->next = mult(p5,p5);
				p51->next = 0;
				}
			FREE_DTOA_LOCK(1);
#else
			p51 = p5->next = mult(p5,p5);
			p51->next = 0;
#endif
			}
		p5 = p51;
		}
	return b;
	}

 static Bigint *
lshift
#ifdef KR_headers
	(b, k) Bigint *b; int k;
#else
	(Bigint *b, int k)
#endif
{
	int i, k1, n, n1;
	Bigint *b1;
	ULong *x, *x1, *xe, z;

#ifdef Pack_32
	n = k >> 5;
#else
	n = k >> 4;
#endif
	k1 = b->k;
	n1 = n + b->wds + 1;
	for(i = b->maxwds; n1 > i; i <<= 1)
		k1++;
	b1 = Balloc(k1);
	x1 = b1->x;
	for(i = 0; i < n; i++)
		*x1++ = 0;
	x = b->x;
	xe = x + b->wds;
#ifdef Pack_32
	if (k &= 0x1f) {
		k1 = 32 - k;
		z = 0;
		do {
			*x1++ = *x << k | z;
			z = *x++ >> k1;
			}
			while(x < xe);
		if ((*x1 = z))
			++n1;
		}
#else
	if (k &= 0xf) {
		k1 = 16 - k;
		z = 0;
		do {
			*x1++ = *x << k  & 0xffff | z;
			z = *x++ >> k1;
			}
			while(x < xe);
		if (*x1 = z)
			++n1;
		}
#endif
	else do
		*x1++ = *x++;
		while(x < xe);
	b1->wds = n1 - 1;
	Bfree(b);
	return b1;
	}

 static int
cmp
#ifdef KR_headers
	(a, b) Bigint *a, *b;
#else
	(Bigint *a, Bigint *b)
#endif
{
	ULong *xa, *xa0, *xb, *xb0;
	int i, j;

	i = a->wds;
	j = b->wds;
#ifdef DEBUG
	if (i > 1 && !a->x[i-1])
		Bug("cmp called with a->x[a->wds-1] == 0");
	if (j > 1 && !b->x[j-1])
		Bug("cmp called with b->x[b->wds-1] == 0");
#endif
	if (i -= j)
		return i;
	xa0 = a->x;
	xa = xa0 + j;
	xb0 = b->x;
	xb = xb0 + j;
	for(;;) {
		if (*--xa != *--xb)
			return *xa < *xb ? -1 : 1;
		if (xa <= xa0)
			break;
		}
	return 0;
	}

 static Bigint *
diff
#ifdef KR_headers
	(a, b) Bigint *a, *b;
#else
	(Bigint *a, Bigint *b)
#endif
{
	Bigint *c;
	int i, wa, wb;
	ULong *xa, *xae, *xb, *xbe, *xc;
#ifdef ULLong
	ULLong borrow, y;
#else
	ULong borrow, y;
#ifdef Pack_32
	ULong z;
#endif
#endif

	i = cmp(a,b);
	if (!i) {
		c = Balloc(0);
		c->wds = 1;
		c->x[0] = 0;
		return c;
		}
	if (i < 0) {
		c = a;
		a = b;
		b = c;
		i = 1;
		}
	else
		i = 0;
	c = Balloc(a->k);
	c->sign = i;
	wa = a->wds;
	xa = a->x;
	xae = xa + wa;
	wb = b->wds;
	xb = b->x;
	xbe = xb + wb;
	xc = c->x;
	borrow = 0;
#ifdef ULLong
	do {
		y = (ULLong)*xa++ - *xb++ - borrow;
		borrow = y >> 32 & (ULong)1;
		*xc++ = (ULong) y & FFFFFFFF;
		}
		while(xb < xbe);
	while(xa < xae) {
		y = *xa++ - borrow;
		borrow = y >> 32 & (ULong)1;
		*xc++ = (ULong) y & FFFFFFFF;
		}
#else
#ifdef Pack_32
	do {
		y = (*xa & 0xffff) - (*xb & 0xffff) - borrow;
		borrow = (y & 0x10000) >> 16;
		z = (*xa++ >> 16) - (*xb++ >> 16) - borrow;
		borrow = (z & 0x10000) >> 16;
		Storeinc(xc, z, y);
		}
		while(xb < xbe);
	while(xa < xae) {
		y = (*xa & 0xffff) - borrow;
		borrow = (y & 0x10000) >> 16;
		z = (*xa++ >> 16) - borrow;
		borrow = (z & 0x10000) >> 16;
		Storeinc(xc, z, y);
		}
#else
	do {
		y = *xa++ - *xb++ - borrow;
		borrow = (y & 0x10000) >> 16;
		*xc++ = y & 0xffff;
		}
		while(xb < xbe);
	while(xa < xae) {
		y = *xa++ - borrow;
		borrow = (y & 0x10000) >> 16;
		*xc++ = y & 0xffff;
		}
#endif
#endif
	while(!*--xc)
		wa--;
	c->wds = wa;
	return c;
	}

 static double
ulp
#ifdef KR_headers
	(x) U x;
#else
	(U x)
#endif
{
	register Long L;
	U a;

	L = (word0(x) & Exp_mask) - (P-1)*Exp_msk1;
#ifndef Avoid_Underflow
#ifndef Sudden_Underflow
	if (L > 0) {
#endif
#endif
#ifdef IBM
		L |= Exp_msk1 >> 4;
#endif
		word0(a) = L;
		word1(a) = 0;
#ifndef Avoid_Underflow
#ifndef Sudden_Underflow
		}
	else {
		L = -L >> Exp_shift;
		if (L < Exp_shift) {
			word0(a) = 0x80000 >> L;
			word1(a) = 0;
			}
		else {
			word0(a) = 0;
			L -= Exp_shift;
			word1(a) = L >= 31 ? 1 : 1 << 31 - L;
			}
		}
#endif
#endif
	return dval(a);
	}

 static double
b2d
#ifdef KR_headers
	(a, e) Bigint *a; int *e;
#else
	(Bigint *a, int *e)
#endif
{
	ULong *xa, *xa0, w, y, z;
	int k;
	U d;
#ifdef VAX
	ULong d0, d1;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

	xa0 = a->x;
	xa = xa0 + a->wds;
	y = *--xa;
#ifdef DEBUG
	if (!y) Bug("zero y in b2d");
#endif
	k = hi0bits(y);
	*e = 32 - k;
#ifdef Pack_32
	if (k < Ebits) {
		d0 = Exp_1 | y >> (Ebits - k);
		w = xa > xa0 ? *--xa : 0;
		d1 = y << ((32-Ebits) + k) | w >> (Ebits - k);
		goto ret_d;
		}
	z = xa > xa0 ? *--xa : 0;
	if (k -= Ebits) {
		d0 = Exp_1 | y << k | z >> (32 - k);
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k | y >> (32 - k);
		}
	else {
		d0 = Exp_1 | y;
		d1 = z;
		}
#else
	if (k < Ebits + 16) {
		z = xa > xa0 ? *--xa : 0;
		d0 = Exp_1 | y << k - Ebits | z >> Ebits + 16 - k;
		w = xa > xa0 ? *--xa : 0;
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k + 16 - Ebits | w << k - Ebits | y >> 16 + Ebits - k;
		goto ret_d;
		}
	z = xa > xa0 ? *--xa : 0;
	w = xa > xa0 ? *--xa : 0;
	k -= Ebits + 16;
	d0 = Exp_1 | y << k + 16 | z << k | w >> 16 - k;
	y = xa > xa0 ? *--xa : 0;
	d1 = w << k + 16 | y << k;
#endif
 ret_d:
#ifdef VAX
	word0(d) = d0 >> 16 | d0 << 16;
	word1(d) = d1 >> 16 | d1 << 16;
#else
#undef d0
#undef d1
#endif
	return dval(d);
	}

 static Bigint *
d2b
#ifdef KR_headers
	(d, e, bits) U d; int *e, *bits;
#else
	(U d, int *e, int *bits)
#endif
{
	Bigint *b;
	int de, k;
	ULong *x, y, z;
#ifndef Sudden_Underflow
	int i;
#endif
#ifdef VAX
	ULong d0, d1;
	d0 = word0(d) >> 16 | word0(d) << 16;
	d1 = word1(d) >> 16 | word1(d) << 16;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

#ifdef Pack_32
	b = Balloc(1);
#else
	b = Balloc(2);
#endif
	x = b->x;

	z = d0 & Frac_mask;
	d0 &= 0x7fffffff;	
#ifdef Sudden_Underflow
	de = (int)(d0 >> Exp_shift);
#ifndef IBM
	z |= Exp_msk11;
#endif
#else
	if ((de = (int)(d0 >> Exp_shift)))
		z |= Exp_msk1;
#endif
#ifdef Pack_32
	if ((y = d1)) {
		if ((k = lo0bits(&y))) {
			x[0] = y | z << (32 - k);
			z >>= k;
			}
		else
			x[0] = y;
#ifndef Sudden_Underflow
		i =
#endif
		    b->wds = (x[1] = z) ? 2 : 1;
		}
	else {
#ifdef DEBUG
		if (!z)
			Bug("Zero passed to d2b");
#endif
		k = lo0bits(&z);
		x[0] = z;
#ifndef Sudden_Underflow
		i =
#endif
		    b->wds = 1;
		k += 32;
		}
#else
	if (y = d1) {
		if (k = lo0bits(&y))
			if (k >= 16) {
				x[0] = y | z << 32 - k & 0xffff;
				x[1] = z >> k - 16 & 0xffff;
				x[2] = z >> k;
				i = 2;
				}
			else {
				x[0] = y & 0xffff;
				x[1] = y >> 16 | z << 16 - k & 0xffff;
				x[2] = z >> k & 0xffff;
				x[3] = z >> k+16;
				i = 3;
				}
		else {
			x[0] = y & 0xffff;
			x[1] = y >> 16;
			x[2] = z & 0xffff;
			x[3] = z >> 16;
			i = 3;
			}
		}
	else {
#ifdef DEBUG
		if (!z)
			Bug("Zero passed to d2b");
#endif
		k = lo0bits(&z);
		if (k >= 16) {
			x[0] = z;
			i = 0;
			}
		else {
			x[0] = z & 0xffff;
			x[1] = z >> 16;
			i = 1;
			}
		k += 32;
		}
	while(!x[i])
		--i;
	b->wds = i + 1;
#endif
#ifndef Sudden_Underflow
	if (de) {
#endif
#ifdef IBM
		*e = (de - Bias - (P-1) << 2) + k;
		*bits = 4*P + 8 - k - hi0bits(word0(d) & Frac_mask);
#else
		*e = de - Bias - (P-1) + k;
		*bits = P - k;
#endif
#ifndef Sudden_Underflow
		}
	else {
		*e = de - Bias - (P-1) + 1 + k;
#ifdef Pack_32
		*bits = 32*i - hi0bits(x[i-1]);
#else
		*bits = (i+2)*16 - hi0bits(x[i]);
#endif
		}
#endif
	return b;
	}
#undef d0
#undef d1

 static double
ratio
#ifdef KR_headers
	(a, b) Bigint *a, *b;
#else
	(Bigint *a, Bigint *b)
#endif
{
	U da, db;
	int k, ka, kb;

	dval(da) = b2d(a, &ka);
	dval(db) = b2d(b, &kb);
#ifdef Pack_32
	k = ka - kb + 32*(a->wds - b->wds);
#else
	k = ka - kb + 16*(a->wds - b->wds);
#endif
#ifdef IBM
	if (k > 0) {
		word0(da) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			dval(da) *= 1 << k;
		}
	else {
		k = -k;
		word0(db) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			dval(db) *= 1 << k;
		}
#else
	if (k > 0)
		word0(da) += k*Exp_msk1;
	else {
		k = -k;
		word0(db) += k*Exp_msk1;
		}
#endif
	return dval(da) / dval(db);
	}

 static CONST double
tens[] = {
		1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
		1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
		1e20, 1e21, 1e22
#ifdef VAX
		, 1e23, 1e24
#endif
		};

 static CONST double
#ifdef IEEE_Arith
bigtens[] = { 1e16, 1e32, 1e64, 1e128, 1e256 };
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128,
#ifdef Avoid_Underflow
		9007199254740992.*9007199254740992.e-256
		
#else
		1e-256
#endif
		};


#define Scale_Bit 0x10
#define n_bigtens 5
#else
#ifdef IBM
bigtens[] = { 1e16, 1e32, 1e64 };
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64 };
#define n_bigtens 3
#else
bigtens[] = { 1e16, 1e32 };
static CONST double tinytens[] = { 1e-16, 1e-32 };
#define n_bigtens 2
#endif
#endif

#ifdef INFNAN_CHECK

#ifndef NAN_WORD0
#define NAN_WORD0 0x7ff80000
#endif

#ifndef NAN_WORD1
#define NAN_WORD1 0
#endif

 static int
match
#ifdef KR_headers
	(sp, t) char **sp, *t;
#else
	(CONST char **sp, CONST char *t)
#endif
{
	int c, d;
	CONST char *s = *sp;

	while((d = *t++)) {
		if ((c = *++s) >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		if (c != d)
			return 0;
		}
	*sp = s + 1;
	return 1;
	}

#ifndef No_Hex_NaN
 static void
hexnan
#ifdef KR_headers
	(rvp, sp) U *rvp; CONST char **sp;
#else
	(U *rvp, CONST char **sp)
#endif
{
	ULong c, x[2];
	CONST char *s;
	int havedig, udx0, xshift;

	x[0] = x[1] = 0;
	havedig = xshift = 0;
	udx0 = 1;
	s = *sp;
	
	while((c = *(CONST unsigned char*)(s+1)) && c <= ' ')
		++s;
	if (s[1] == '0' && (s[2] == 'x' || s[2] == 'X'))
		s += 2;
	while((c = *(CONST unsigned char*)++s)) {
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'f')
			c += 10 - 'a';
		else if (c >= 'A' && c <= 'F')
			c += 10 - 'A';
		else if (c <= ' ') {
			if (udx0 && havedig) {
				udx0 = 0;
				xshift = 1;
				}
			continue;
			}
#ifdef GDTOA_NON_PEDANTIC_NANCHECK
		else if ( c == ')' && havedig) {
			*sp = s + 1;
			break;
			}
		else
			return;	
#else
		else {
			do {
				if ( c == ')') {
					*sp = s + 1;
					break;
					}
				} while((c = *++s));
			break;
			}
#endif
		havedig = 1;
		if (xshift) {
			xshift = 0;
			x[0] = x[1];
			x[1] = 0;
			}
		if (udx0)
			x[0] = (x[0] << 4) | (x[1] >> 28);
		x[1] = (x[1] << 4) | c;
		}
	if ((x[0] &= 0xfffff) || x[1]) {
		word0(*rvp) = Exp_mask | x[0];
		word1(*rvp) = x[1];
		}
	}
#endif 
#endif 

 static double
_strtod
#ifdef KR_headers
	(s00, se) CONST char *s00; char **se;
#else
	(CONST char *s00, char **se)
#endif
{
#ifdef Avoid_Underflow
	int scale;
#endif
	int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
		 e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
	CONST char *s, *s0, *s1;
	double aadj, adj;
	U aadj1, rv, rv0;
	Long L;
	ULong y, z;
	Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;
#ifdef SET_INEXACT
	int inexact, oldinexact;
#endif
#ifdef Honor_FLT_ROUNDS
	int rounding;
#endif
#ifdef USE_LOCALE
	CONST char *s2;
#endif

#ifdef __GNUC__
	delta = bb = bd = bs = 0;
#endif

	sign = nz0 = nz = 0;
	dval(rv) = 0.;
	for(s = s00;;s++) switch(*s) {
		case '-':
			sign = 1;
			
		case '+':
			if (*++s)
				goto break2;
			
		case 0:
			goto ret0;
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
			continue;
		default:
			goto break2;
		}
 break2:
	if (*s == '0') {
		nz0 = 1;
		while(*++s == '0') ;
		if (!*s)
			goto ret;
		}
	s0 = s;
	y = z = 0;
	for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
		if (nd < 9)
			y = 10*y + c - '0';
		else if (nd < 16)
			z = 10*z + c - '0';
	nd0 = nd;
#ifdef USE_LOCALE
	s1 = localeconv()->decimal_point;
	if (c == *s1) {
		c = '.';
		if (*++s1) {
			s2 = s;
			for(;;) {
				if (*++s2 != *s1) {
					c = 0;
					break;
					}
				if (!*++s1) {
					s = s2;
					break;
					}
				}
			}
		}
#endif
	if (c == '.') {
		c = *++s;
		if (!nd) {
			for(; c == '0'; c = *++s)
				nz++;
			if (c > '0' && c <= '9') {
				s0 = s;
				nf += nz;
				nz = 0;
				goto have_dig;
				}
			goto dig_done;
			}
		for(; c >= '0' && c <= '9'; c = *++s) {
 have_dig:
			nz++;
			if (c -= '0') {
				nf += nz;
				for(i = 1; i < nz; i++)
					if (nd++ < 9)
						y *= 10;
					else if (nd <= DBL_DIG + 1)
						z *= 10;
				if (nd++ < 9)
					y = 10*y + c;
				else if (nd <= DBL_DIG + 1)
					z = 10*z + c;
				nz = 0;
				}
			}
		}
 dig_done:
	e = 0;
	if (c == 'e' || c == 'E') {
		if (!nd && !nz && !nz0) {
			goto ret0;
			}
		s00 = s;
		esign = 0;
		switch(c = *++s) {
			case '-':
				esign = 1;
			case '+':
				c = *++s;
			}
		if (c >= '0' && c <= '9') {
			while(c == '0')
				c = *++s;
			if (c > '0' && c <= '9') {
				L = c - '0';
				s1 = s;
				while((c = *++s) >= '0' && c <= '9')
					L = 10*L + c - '0';
				if (s - s1 > 8 || L > 19999)
					


					e = 19999; 
				else
					e = (int)L;
				if (esign)
					e = -e;
				}
			else
				e = 0;
			}
		else
			s = s00;
		}
	if (!nd) {
		if (!nz && !nz0) {
#ifdef INFNAN_CHECK
			
			switch(c) {
			  case 'i':
			  case 'I':
				if (match(&s,"nf")) {
					--s;
					if (!match(&s,"inity"))
						++s;
					word0(rv) = 0x7ff00000;
					word1(rv) = 0;
					goto ret;
					}
				break;
			  case 'n':
			  case 'N':
				if (match(&s, "an")) {
					word0(rv) = NAN_WORD0;
					word1(rv) = NAN_WORD1;
#ifndef No_Hex_NaN
					if (*s == '(') 
						hexnan(&rv, &s);
#endif
					goto ret;
					}
			  }
#endif 
 ret0:
			s = s00;
			sign = 0;
			}
		goto ret;
		}
	e1 = e -= nf;

	




	if (!nd0)
		nd0 = nd;
	k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
	dval(rv) = y;
	if (k > 9) {
#ifdef SET_INEXACT
		if (k > DBL_DIG)
			oldinexact = get_inexact();
#endif
		dval(rv) = tens[k - 9] * dval(rv) + z;
		}
	bd0 = 0;
	if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
#ifndef Honor_FLT_ROUNDS
		&& Flt_Rounds == 1
#endif
#endif
			) {
		if (!e)
			goto ret;
		if (e > 0) {
			if (e <= Ten_pmax) {
#ifdef VAX
				goto vax_ovfl_check;
#else
#ifdef Honor_FLT_ROUNDS
				
				if (sign) {
					rv = -rv;
					sign = 0;
					}
#endif
				 rounded_product(dval(rv), tens[e]);
				goto ret;
#endif
				}
			i = DBL_DIG - nd;
			if (e <= Ten_pmax + i) {
				


#ifdef Honor_FLT_ROUNDS
				
				if (sign) {
					rv = -rv;
					sign = 0;
					}
#endif
				e -= i;
				dval(rv) *= tens[i];
#ifdef VAX
				


 vax_ovfl_check:
				word0(rv) -= P*Exp_msk1;
				 rounded_product(dval(rv), tens[e]);
				if ((word0(rv) & Exp_mask)
				 > Exp_msk1*(DBL_MAX_EXP+Bias-1-P))
					goto ovfl;
				word0(rv) += P*Exp_msk1;
#else
				 rounded_product(dval(rv), tens[e]);
#endif
				goto ret;
				}
			}
#ifndef Inaccurate_Divide
		else if (e >= -Ten_pmax) {
#ifdef Honor_FLT_ROUNDS
			
			if (sign) {
				rv = -rv;
				sign = 0;
				}
#endif
			 rounded_quotient(dval(rv), tens[-e]);
			goto ret;
			}
#endif
		}
	e1 += nd - k;

#ifdef IEEE_Arith
#ifdef SET_INEXACT
	inexact = 1;
	if (k <= DBL_DIG)
		oldinexact = get_inexact();
#endif
#ifdef Avoid_Underflow
	scale = 0;
#endif
#ifdef Honor_FLT_ROUNDS
	if ((rounding = Flt_Rounds) >= 2) {
		if (sign)
			rounding = rounding == 2 ? 0 : 2;
		else
			if (rounding != 2)
				rounding = 0;
		}
#endif
#endif 

	

	if (e1 > 0) {
		if ((i = e1 & 15))
			dval(rv) *= tens[i];
		if (e1 &= ~15) {
			if (e1 > DBL_MAX_10_EXP) {
 ovfl:
#ifndef NO_ERRNO
				errno = ERANGE;
#endif
				
#ifdef IEEE_Arith
#ifdef Honor_FLT_ROUNDS
				switch(rounding) {
				  case 0: 
				  case 3: 
					word0(rv) = Big0;
					word1(rv) = Big1;
					break;
				  default:
					word0(rv) = Exp_mask;
					word1(rv) = 0;
				  }
#else 
				word0(rv) = Exp_mask;
				word1(rv) = 0;
#endif 
#ifdef SET_INEXACT
				
				dval(rv0) = 1e300;
				dval(rv0) *= dval(rv0);
#endif
#else 
				word0(rv) = Big0;
				word1(rv) = Big1;
#endif 
				if (bd0)
					goto retfree;
				goto ret;
				}
			e1 >>= 4;
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= bigtens[j];
		
			word0(rv) -= P*Exp_msk1;
			dval(rv) *= bigtens[j];
			if ((z = word0(rv) & Exp_mask)
			 > Exp_msk1*(DBL_MAX_EXP+Bias-P))
				goto ovfl;
			if (z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)) {
				
				
				word0(rv) = Big0;
				word1(rv) = Big1;
				}
			else
				word0(rv) += P*Exp_msk1;
			}
		}
	else if (e1 < 0) {
		e1 = -e1;
		if ((i = e1 & 15))
			dval(rv) /= tens[i];
		if (e1 >>= 4) {
			if (e1 >= 1 << n_bigtens)
				goto undfl;
#ifdef Avoid_Underflow
			if (e1 & Scale_Bit)
				scale = 2*P;
			for(j = 0; e1 > 0; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= tinytens[j];
			if (scale && (j = 2*P + 1 - ((word0(rv) & Exp_mask)
						>> Exp_shift)) > 0) {
				
				if (j >= 32) {
					word1(rv) = 0;
					if (j >= 53)
					 word0(rv) = (P+2)*Exp_msk1;
					else
					 word0(rv) &= 0xffffffff << (j-32);
					}
				else
					word1(rv) &= 0xffffffff << j;
				}
#else
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= tinytens[j];
			
			dval(rv0) = dval(rv);
			dval(rv) *= tinytens[j];
			if (!dval(rv)) {
				dval(rv) = 2.*dval(rv0);
				dval(rv) *= tinytens[j];
#endif
				if (!dval(rv)) {
 undfl:
					dval(rv) = 0.;
#ifndef NO_ERRNO
					errno = ERANGE;
#endif
					if (bd0)
						goto retfree;
					goto ret;
					}
#ifndef Avoid_Underflow
				word0(rv) = Tiny0;
				word1(rv) = Tiny1;
				


				}
#endif
			}
		}

	

	

	bd0 = s2b(s0, nd0, nd, y);

	for(;;) {
		bd = Balloc(bd0->k);
		Bcopy(bd, bd0);
		bb = d2b(rv, &bbe, &bbbits);	
		bs = i2b(1);

		if (e >= 0) {
			bb2 = bb5 = 0;
			bd2 = bd5 = e;
			}
		else {
			bb2 = bb5 = -e;
			bd2 = bd5 = 0;
			}
		if (bbe >= 0)
			bb2 += bbe;
		else
			bd2 -= bbe;
		bs2 = bb2;
#ifdef Honor_FLT_ROUNDS
		if (rounding != 1)
			bs2++;
#endif
#ifdef Avoid_Underflow
		j = bbe - scale;
		i = j + bbbits - 1;	
		if (i < Emin)	
			j += P - Emin;
		else
			j = P + 1 - bbbits;
#else 
#ifdef Sudden_Underflow
#ifdef IBM
		j = 1 + 4*P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
		j = P + 1 - bbbits;
#endif
#else 
		j = bbe;
		i = j + bbbits - 1;	
		if (i < Emin)	
			j += P - Emin;
		else
			j = P + 1 - bbbits;
#endif 
#endif 
		bb2 += j;
		bd2 += j;
#ifdef Avoid_Underflow
		bd2 += scale;
#endif
		i = bb2 < bd2 ? bb2 : bd2;
		if (i > bs2)
			i = bs2;
		if (i > 0) {
			bb2 -= i;
			bd2 -= i;
			bs2 -= i;
			}
		if (bb5 > 0) {
			bs = pow5mult(bs, bb5);
			bb1 = mult(bs, bb);
			Bfree(bb);
			bb = bb1;
			}
		if (bb2 > 0)
			bb = lshift(bb, bb2);
		if (bd5 > 0)
			bd = pow5mult(bd, bd5);
		if (bd2 > 0)
			bd = lshift(bd, bd2);
		if (bs2 > 0)
			bs = lshift(bs, bs2);
		delta = diff(bb, bd);
		dsign = delta->sign;
		delta->sign = 0;
		i = cmp(delta, bs);
#ifdef Honor_FLT_ROUNDS
		if (rounding != 1) {
			if (i < 0) {
				
				if (!delta->x[0] && delta->wds <= 1) {
					
#ifdef SET_INEXACT
					inexact = 0;
#endif
					break;
					}
				if (rounding) {
					if (dsign) {
						adj = 1.;
						goto apply_adj;
						}
					}
				else if (!dsign) {
					adj = -1.;
					if (!word1(rv)
					 && !(word0(rv) & Frac_mask)) {
						y = word0(rv) & Exp_mask;
#ifdef Avoid_Underflow
						if (!scale || y > 2*P*Exp_msk1)
#else
						if (y)
#endif
						  {
						  delta = lshift(delta,Log2P);
						  if (cmp(delta, bs) <= 0)
							adj = -0.5;
						  }
						}
 apply_adj:
#ifdef Avoid_Underflow
					if (scale && (y = word0(rv) & Exp_mask)
						<= 2*P*Exp_msk1)
					  word0(adj) += (2*P+1)*Exp_msk1 - y;
#else
#ifdef Sudden_Underflow
					if ((word0(rv) & Exp_mask) <=
							P*Exp_msk1) {
						word0(rv) += P*Exp_msk1;
						dval(rv) += adj*ulp(rv);
						word0(rv) -= P*Exp_msk1;
						}
					else
#endif 
#endif 
					dval(rv) += adj*ulp(rv);
					}
				break;
				}
			adj = ratio(delta, bs);
			if (adj < 1.)
				adj = 1.;
			if (adj <= 0x7ffffffe) {
				
				y = adj;
				if (y != adj) {
					if (!((rounding>>1) ^ dsign))
						y++;
					adj = y;
					}
				}
#ifdef Avoid_Underflow
			if (scale && (y = word0(rv) & Exp_mask) <= 2*P*Exp_msk1)
				word0(adj) += (2*P+1)*Exp_msk1 - y;
#else
#ifdef Sudden_Underflow
			if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
				word0(rv) += P*Exp_msk1;
				adj *= ulp(rv);
				if (dsign)
					dval(rv) += adj;
				else
					dval(rv) -= adj;
				word0(rv) -= P*Exp_msk1;
				goto cont;
				}
#endif 
#endif 
			adj *= ulp(rv);
			if (dsign)
				dval(rv) += adj;
			else
				dval(rv) -= adj;
			goto cont;
			}
#endif 

		if (i < 0) {
			


			if (dsign || word1(rv) || word0(rv) & Bndry_mask
#ifdef IEEE_Arith
#ifdef Avoid_Underflow
			 || (word0(rv) & Exp_mask) <= (2*P+1)*Exp_msk1
#else
			 || (word0(rv) & Exp_mask) <= Exp_msk1
#endif
#endif
				) {
#ifdef SET_INEXACT
				if (!delta->x[0] && delta->wds <= 1)
					inexact = 0;
#endif
				break;
				}
			if (!delta->x[0] && delta->wds <= 1) {
				
#ifdef SET_INEXACT
				inexact = 0;
#endif
				break;
				}
			delta = lshift(delta,Log2P);
			if (cmp(delta, bs) > 0)
				goto drop_down;
			break;
			}
		if (i == 0) {
			
			if (dsign) {
				if ((word0(rv) & Bndry_mask1) == Bndry_mask1
				 &&  word1(rv) == (
#ifdef Avoid_Underflow
			(scale && (y = word0(rv) & Exp_mask) <= 2*P*Exp_msk1)
		? (0xffffffff & (0xffffffff << (2*P+1-(y>>Exp_shift)))) :
#endif
						   0xffffffff)) {
					
					word0(rv) = (word0(rv) & Exp_mask)
						+ Exp_msk1
#ifdef IBM
						| Exp_msk1 >> 4
#endif
						;
					word1(rv) = 0;
#ifdef Avoid_Underflow
					dsign = 0;
#endif
					break;
					}
				}
			else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
 drop_down:
				
#ifdef Sudden_Underflow 
				L = word0(rv) & Exp_mask;
#ifdef IBM
				if (L <  Exp_msk1)
#else
#ifdef Avoid_Underflow
				if (L <= (scale ? (2*P+1)*Exp_msk1 : Exp_msk1))
#else
				if (L <= Exp_msk1)
#endif 
#endif 
					goto undfl;
				L -= Exp_msk1;
#else 
#ifdef Avoid_Underflow
				if (scale) {
					L = word0(rv) & Exp_mask;
					if (L <= (2*P+1)*Exp_msk1) {
						if (L > (P+2)*Exp_msk1)
							
							
							break;
						
						goto undfl;
						}
					}
#endif 
				L = (word0(rv) & Exp_mask) - Exp_msk1;
#endif 
				word0(rv) = L | Bndry_mask1;
				word1(rv) = 0xffffffff;
#ifdef IBM
				goto cont;
#else
				break;
#endif
				}
#ifndef ROUND_BIASED
			if (!(word1(rv) & LSB))
				break;
#endif
			if (dsign)
				dval(rv) += ulp(rv);
#ifndef ROUND_BIASED
			else {
				dval(rv) -= ulp(rv);
#ifndef Sudden_Underflow
				if (!dval(rv))
					goto undfl;
#endif
				}
#ifdef Avoid_Underflow
			dsign = 1 - dsign;
#endif
#endif
			break;
			}
		if ((aadj = ratio(delta, bs)) <= 2.) {
			if (dsign)
				aadj = dval(aadj1) = 1.;
			else if (word1(rv) || word0(rv) & Bndry_mask) {
#ifndef Sudden_Underflow
				if (word1(rv) == Tiny1 && !word0(rv))
					goto undfl;
#endif
				aadj = 1.;
				dval(aadj1) = -1.;
				}
			else {
				
				

				if (aadj < 2./FLT_RADIX)
					aadj = 1./FLT_RADIX;
				else
					aadj *= 0.5;
				dval(aadj1) = -aadj;
				}
			}
		else {
			aadj *= 0.5;
			dval(aadj1) = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
			switch(Rounding) {
				case 2: 
					dval(aadj1) -= 0.5;
					break;
				case 0: 
				case 3: 
					dval(aadj1) += 0.5;
				}
#else
			if (Flt_Rounds == 0)
				dval(aadj1) += 0.5;
#endif 
			}
		y = word0(rv) & Exp_mask;

		

		if (y == Exp_msk1*(DBL_MAX_EXP+Bias-1)) {
			dval(rv0) = dval(rv);
			word0(rv) -= P*Exp_msk1;
			adj = dval(aadj1) * ulp(rv);
			dval(rv) += adj;
			if ((word0(rv) & Exp_mask) >=
					Exp_msk1*(DBL_MAX_EXP+Bias-P)) {
				if (word0(rv0) == Big0 && word1(rv0) == Big1)
					goto ovfl;
				word0(rv) = Big0;
				word1(rv) = Big1;
				goto cont;
				}
			else
				word0(rv) += P*Exp_msk1;
			}
		else {
#ifdef Avoid_Underflow
			if (scale && y <= 2*P*Exp_msk1) {
				if (aadj <= 0x7fffffff) {
					if ((z = (ULong) aadj) <= 0)
						z = 1;
					aadj = z;
					dval(aadj1) = dsign ? aadj : -aadj;
					}
				word0(aadj1) += (2*P+1)*Exp_msk1 - y;
				}
			adj = dval(aadj1) * ulp(rv);
			dval(rv) += adj;
#else
#ifdef Sudden_Underflow
			if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
				dval(rv0) = dval(rv);
				word0(rv) += P*Exp_msk1;
				adj = dval(aadj1) * ulp(rv);
				dval(rv) += adj;
#ifdef IBM
				if ((word0(rv) & Exp_mask) <  P*Exp_msk1)
#else
				if ((word0(rv) & Exp_mask) <= P*Exp_msk1)
#endif
					{
					if (word0(rv0) == Tiny0
					 && word1(rv0) == Tiny1)
						goto undfl;
					word0(rv) = Tiny0;
					word1(rv) = Tiny1;
					goto cont;
					}
				else
					word0(rv) -= P*Exp_msk1;
				}
			else {
				adj = dval(aadj1) * ulp(rv);
				dval(rv) += adj;
				}
#else 
			






			if (y <= (P-1)*Exp_msk1 && aadj > 1.) {
				dval(aadj1) = (double)(int)(aadj + 0.5);
				if (!dsign)
					dval(aadj1) = -dval(aadj1);
				}
			adj = dval(aadj1) * ulp(rv);
			dval(rv) += adj;
#endif 
#endif
			}
		z = word0(rv) & Exp_mask;
#ifndef SET_INEXACT
#ifdef Avoid_Underflow
		if (!scale)
#endif
		if (y == z) {
			
			L = (Long)aadj;
			aadj -= L;
			
			if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
				if (aadj < .4999999 || aadj > .5000001)
					break;
				}
			else if (aadj < .4999999/FLT_RADIX)
				break;
			}
#endif
 cont:
		Bfree(bb);
		Bfree(bd);
		Bfree(bs);
		Bfree(delta);
		}
#ifdef SET_INEXACT
	if (inexact) {
		if (!oldinexact) {
			word0(rv0) = Exp_1 + (70 << Exp_shift);
			word1(rv0) = 0;
			dval(rv0) += 1.;
			}
		}
	else if (!oldinexact)
		clear_inexact();
#endif
#ifdef Avoid_Underflow
	if (scale) {
		word0(rv0) = Exp_1 - 2*P*Exp_msk1;
		word1(rv0) = 0;
		dval(rv) *= dval(rv0);
#ifndef NO_ERRNO
		
		if (word0(rv) == 0 && word1(rv) == 0)
			errno = ERANGE;
#endif
		}
#endif 
#ifdef SET_INEXACT
	if (inexact && !(word0(rv) & Exp_mask)) {
		
		dval(rv0) = 1e-300;
		dval(rv0) *= dval(rv0);
		}
#endif
 retfree:
	Bfree(bb);
	Bfree(bd);
	Bfree(bs);
	Bfree(bd0);
	Bfree(delta);
 ret:
	if (se)
		*se = (char *)s;
	return sign ? -dval(rv) : dval(rv);
	}

 static int
quorem
#ifdef KR_headers
	(b, S) Bigint *b, *S;
#else
	(Bigint *b, Bigint *S)
#endif
{
	int n;
	ULong *bx, *bxe, q, *sx, *sxe;
#ifdef ULLong
	ULLong borrow, carry, y, ys;
#else
	ULong borrow, carry, y, ys;
#ifdef Pack_32
	ULong si, z, zs;
#endif
#endif

	n = S->wds;
#ifdef DEBUG
	 if (b->wds > n)
		Bug("oversize b in quorem");
#endif
	if (b->wds < n)
		return 0;
	sx = S->x;
	sxe = sx + --n;
	bx = b->x;
	bxe = bx + n;
	q = *bxe / (*sxe + 1);	
#ifdef DEBUG
	 if (q > 9)
		Bug("oversized quotient in quorem");
#endif
	if (q) {
		borrow = 0;
		carry = 0;
		do {
#ifdef ULLong
			ys = *sx++ * (ULLong)q + carry;
			carry = ys >> 32;
			y = *bx - (ys & FFFFFFFF) - borrow;
			borrow = y >> 32 & (ULong)1;
			*bx++ = (ULong) y & FFFFFFFF;
#else
#ifdef Pack_32
			si = *sx++;
			ys = (si & 0xffff) * q + carry;
			zs = (si >> 16) * q + (ys >> 16);
			carry = zs >> 16;
			y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
			borrow = (y & 0x10000) >> 16;
			z = (*bx >> 16) - (zs & 0xffff) - borrow;
			borrow = (z & 0x10000) >> 16;
			Storeinc(bx, z, y);
#else
			ys = *sx++ * q + carry;
			carry = ys >> 16;
			y = *bx - (ys & 0xffff) - borrow;
			borrow = (y & 0x10000) >> 16;
			*bx++ = y & 0xffff;
#endif
#endif
			}
			while(sx <= sxe);
		if (!*bxe) {
			bx = b->x;
			while(--bxe > bx && !*bxe)
				--n;
			b->wds = n;
			}
		}
	if (cmp(b, S) >= 0) {
		q++;
		borrow = 0;
		carry = 0;
		bx = b->x;
		sx = S->x;
		do {
#ifdef ULLong
			ys = *sx++ + carry;
			carry = ys >> 32;
			y = *bx - (ys & FFFFFFFF) - borrow;
			borrow = y >> 32 & (ULong)1;
			*bx++ = (ULong) y & FFFFFFFF;
#else
#ifdef Pack_32
			si = *sx++;
			ys = (si & 0xffff) + carry;
			zs = (si >> 16) + (ys >> 16);
			carry = zs >> 16;
			y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
			borrow = (y & 0x10000) >> 16;
			z = (*bx >> 16) - (zs & 0xffff) - borrow;
			borrow = (z & 0x10000) >> 16;
			Storeinc(bx, z, y);
#else
			ys = *sx++ + carry;
			carry = ys >> 16;
			y = *bx - (ys & 0xffff) - borrow;
			borrow = (y & 0x10000) >> 16;
			*bx++ = y & 0xffff;
#endif
#endif
			}
			while(sx <= sxe);
		bx = b->x;
		bxe = bx + n;
		if (!*bxe) {
			while(--bxe > bx && !*bxe)
				--n;
			b->wds = n;
			}
		}
	return q;
	}

#ifndef MULTIPLE_THREADS
 static char *dtoa_result;
#endif

 static char *
#ifdef KR_headers
rv_alloc(i) int i;
#else
rv_alloc(int i)
#endif
{
	int j, k, *r;

	j = sizeof(ULong);
	for(k = 0;
		sizeof(Bigint) - sizeof(ULong) - sizeof(int) + j <= (unsigned) i;
		j <<= 1)
			k++;
	r = (int*)Balloc(k);
	*r = k;
	return
#ifndef MULTIPLE_THREADS
	dtoa_result =
#endif
		(char *)(r+1);
	}

 static char *
#ifdef KR_headers
nrv_alloc(s, rve, n) char *s, **rve; int n;
#else
nrv_alloc(CONST char *s, char **rve, int n)
#endif
{
	char *rv, *t;

	t = rv = rv_alloc(n);
	while((*t = *s++)) t++;
	if (rve)
		*rve = t;
	return rv;
	}







 void
#ifdef KR_headers
freedtoa(s) char *s;
#else
freedtoa(char *s)
#endif
{
	Bigint *b = (Bigint *)((int *)s - 1);
	b->maxwds = 1 << (b->k = *(int*)b);
	Bfree(b);
#ifndef MULTIPLE_THREADS
	if (s == dtoa_result)
		dtoa_result = 0;
#endif
	}



































 static char *
dtoa
#ifdef KR_headers
	(d, mode, ndigits, decpt, sign, rve)
	U d; int mode, ndigits, *decpt, *sign; char **rve;
#else
	(U d, int mode, int ndigits, int *decpt, int *sign, char **rve)
#endif
{
 

































	int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1,
		j, j1, k, k0, k_check, leftright, m2, m5, s2, s5,
		spec_case, try_quick;
	Long L;
#ifndef Sudden_Underflow
	int denorm;
	ULong x;
#endif
	Bigint *b, *b1, *delta, *mlo, *mhi, *S;
	U d2, eps;
	double ds;
	char *s, *s0;
#ifdef Honor_FLT_ROUNDS
	int rounding;
#endif
#ifdef SET_INEXACT
	int inexact, oldinexact;
#endif

#ifdef __GNUC__
	ilim = ilim1 = 0;
	mlo = NULL;
#endif

#ifndef MULTIPLE_THREADS
	if (dtoa_result) {
		freedtoa(dtoa_result);
		dtoa_result = 0;
		}
#endif

	if (word0(d) & Sign_bit) {
		
		*sign = 1;
		word0(d) &= ~Sign_bit;	
		}
	else
		*sign = 0;

#if defined(IEEE_Arith) + defined(VAX)
#ifdef IEEE_Arith
	if ((word0(d) & Exp_mask) == Exp_mask)
#else
	if (word0(d)  == 0x8000)
#endif
		{
		
		*decpt = 9999;
#ifdef IEEE_Arith
		if (!word1(d) && !(word0(d) & 0xfffff))
			return nrv_alloc("Infinity", rve, 8);
#endif
		return nrv_alloc("NaN", rve, 3);
		}
#endif
#ifdef IBM
	dval(d) += 0; 
#endif
	if (!dval(d)) {
		*decpt = 1;
		return nrv_alloc("0", rve, 1);
		}

#ifdef SET_INEXACT
	try_quick = oldinexact = get_inexact();
	inexact = 1;
#endif
#ifdef Honor_FLT_ROUNDS
	if ((rounding = Flt_Rounds) >= 2) {
		if (*sign)
			rounding = rounding == 2 ? 0 : 2;
		else
			if (rounding != 2)
				rounding = 0;
		}
#endif

	b = d2b(d, &be, &bbits);
#ifdef Sudden_Underflow
	i = (int)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1));
#else
	if ((i = (int)(word0(d) >> Exp_shift1 & (Exp_mask>>Exp_shift1)))) {
#endif
		dval(d2) = dval(d);
		word0(d2) &= Frac_mask1;
		word0(d2) |= Exp_11;
#ifdef IBM
		if (j = 11 - hi0bits(word0(d2) & Frac_mask))
			dval(d2) /= 1 << j;
#endif

		





















		i -= Bias;
#ifdef IBM
		i <<= 2;
		i += j;
#endif
#ifndef Sudden_Underflow
		denorm = 0;
		}
	else {
		

		i = bbits + be + (Bias + (P-1) - 1);
		x = i > 32  ? word0(d) << (64 - i) | word1(d) >> (i - 32)
			    : word1(d) << (32 - i);
		dval(d2) = x;
		word0(d2) -= 31*Exp_msk1; 
		i -= (Bias + (P-1) - 1) + 1;
		denorm = 1;
		}
#endif
	ds = (dval(d2)-1.5)*0.289529654602168 + 0.1760912590558 + i*0.301029995663981;
	k = (int)ds;
	if (ds < 0. && ds != k)
		k--;	
	k_check = 1;
	if (k >= 0 && k <= Ten_pmax) {
		if (dval(d) < tens[k])
			k--;
		k_check = 0;
		}
	j = bbits - i - 1;
	if (j >= 0) {
		b2 = 0;
		s2 = j;
		}
	else {
		b2 = -j;
		s2 = 0;
		}
	if (k >= 0) {
		b5 = 0;
		s5 = k;
		s2 += k;
		}
	else {
		b2 -= k;
		b5 = -k;
		s5 = 0;
		}
	if (mode < 0 || mode > 9)
		mode = 0;

#ifndef SET_INEXACT
#ifdef Check_FLT_ROUNDS
	try_quick = Rounding == 1;
#else
	try_quick = 1;
#endif
#endif 

	if (mode > 5) {
		mode -= 4;
		try_quick = 0;
		}
	leftright = 1;
	switch(mode) {
		case 0:
		case 1:
			ilim = ilim1 = -1;
			i = 18;
			ndigits = 0;
			break;
		case 2:
			leftright = 0;
			
		case 4:
			if (ndigits <= 0)
				ndigits = 1;
			ilim = ilim1 = i = ndigits;
			break;
		case 3:
			leftright = 0;
			
		case 5:
			i = ndigits + k + 1;
			ilim = i;
			ilim1 = i - 1;
			if (i <= 0)
				i = 1;
		}
	s = s0 = rv_alloc(i);

#ifdef Honor_FLT_ROUNDS
	if (mode > 1 && rounding != 1)
		leftright = 0;
#endif

	if (ilim >= 0 && ilim <= Quick_max && try_quick) {

		

		i = 0;
		dval(d2) = dval(d);
		k0 = k;
		ilim0 = ilim;
		ieps = 2; 
		if (k > 0) {
			ds = tens[k&0xf];
			j = k >> 4;
			if (j & Bletch) {
				
				j &= Bletch - 1;
				dval(d) /= bigtens[n_bigtens-1];
				ieps++;
				}
			for(; j; j >>= 1, i++)
				if (j & 1) {
					ieps++;
					ds *= bigtens[i];
					}
			dval(d) /= ds;
			}
		else if ((j1 = -k)) {
			dval(d) *= tens[j1 & 0xf];
			for(j = j1 >> 4; j; j >>= 1, i++)
				if (j & 1) {
					ieps++;
					dval(d) *= bigtens[i];
					}
			}
		if (k_check && dval(d) < 1. && ilim > 0) {
			if (ilim1 <= 0)
				goto fast_failed;
			ilim = ilim1;
			k--;
			dval(d) *= 10.;
			ieps++;
			}
		dval(eps) = ieps*dval(d) + 7.;
		word0(eps) -= (P-1)*Exp_msk1;
		if (ilim == 0) {
			S = mhi = 0;
			dval(d) -= 5.;
			if (dval(d) > dval(eps))
				goto one_digit;
			if (dval(d) < -dval(eps))
				goto no_digits;
			goto fast_failed;
			}
#ifndef No_leftright
		if (leftright) {
			


			dval(eps) = 0.5/tens[ilim-1] - dval(eps);
			for(i = 0;;) {
				L = (ULong) dval(d);
				dval(d) -= L;
				*s++ = '0' + (int)L;
				if (dval(d) < dval(eps))
					goto ret1;
				if (1. - dval(d) < dval(eps))
					goto bump_up;
				if (++i >= ilim)
					break;
				dval(eps) *= 10.;
				dval(d) *= 10.;
				}
			}
		else {
#endif

			dval(eps) *= tens[ilim-1];
			for(i = 1;; i++, dval(d) *= 10.) {
				L = (Long)(dval(d));
				if (!(dval(d) -= L))
					ilim = i;
				*s++ = '0' + (int)L;
				if (i == ilim) {
					if (dval(d) > 0.5 + dval(eps))
						goto bump_up;
					else if (dval(d) < 0.5 - dval(eps)) {
						while(*--s == '0');
						s++;
						goto ret1;
						}
					break;
					}
				}
#ifndef No_leftright
			}
#endif
 fast_failed:
		s = s0;
		dval(d) = dval(d2);
		k = k0;
		ilim = ilim0;
		}

	

	if (be >= 0 && k <= Int_max) {
		
		ds = tens[k];
		if (ndigits < 0 && ilim <= 0) {
			S = mhi = 0;
			if (ilim < 0 || dval(d) < 5*ds)
				goto no_digits;
			goto one_digit;
			}
		for(i = 1;; i++, dval(d) *= 10.) {
			L = (Long)(dval(d) / ds);
			dval(d) -= L*ds;
#ifdef Check_FLT_ROUNDS
			
			if (dval(d) < 0) {
				L--;
				dval(d) += ds;
				}
#endif
			*s++ = '0' + (int)L;
			if (!dval(d)) {
#ifdef SET_INEXACT
				inexact = 0;
#endif
				break;
				}
			if (i == ilim) {
#ifdef Honor_FLT_ROUNDS
				if (mode > 1)
				switch(rounding) {
				  case 0: goto ret1;
				  case 2: goto bump_up;
				  }
#endif
				dval(d) += dval(d);
				if (dval(d) > ds || (dval(d) == ds && L & 1)) {
 bump_up:
					while(*--s == '9')
						if (s == s0) {
							k++;
							*s = '0';
							break;
							}
					++*s++;
					}
				break;
				}
			}
		goto ret1;
		}

	m2 = b2;
	m5 = b5;
	mhi = mlo = 0;
	if (leftright) {
		i =
#ifndef Sudden_Underflow
			denorm ? be + (Bias + (P-1) - 1 + 1) :
#endif
#ifdef IBM
			1 + 4*P - 3 - bbits + ((bbits + be - 1) & 3);
#else
			1 + P - bbits;
#endif
		b2 += i;
		s2 += i;
		mhi = i2b(1);
		}
	if (m2 > 0 && s2 > 0) {
		i = m2 < s2 ? m2 : s2;
		b2 -= i;
		m2 -= i;
		s2 -= i;
		}
	if (b5 > 0) {
		if (leftright) {
			if (m5 > 0) {
				mhi = pow5mult(mhi, m5);
				b1 = mult(mhi, b);
				Bfree(b);
				b = b1;
				}
			if ((j = b5 - m5))
				b = pow5mult(b, j);
			}
		else
			b = pow5mult(b, b5);
		}
	S = i2b(1);
	if (s5 > 0)
		S = pow5mult(S, s5);

	

	spec_case = 0;
	if ((mode < 2 || leftright)
#ifdef Honor_FLT_ROUNDS
			&& rounding == 1
#endif
				) {
		if (!word1(d) && !(word0(d) & Bndry_mask)
#ifndef Sudden_Underflow
		 && word0(d) & (Exp_mask & ~Exp_msk1)
#endif
				) {
			
			b2 += Log2P;
			s2 += Log2P;
			spec_case = 1;
			}
		}

	






#ifdef Pack_32
	if ((i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0x1f))
		i = 32 - i;
#else
	if (i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0xf)
		i = 16 - i;
#endif
	if (i > 4) {
		i -= 4;
		b2 += i;
		m2 += i;
		s2 += i;
		}
	else if (i < 4) {
		i += 28;
		b2 += i;
		m2 += i;
		s2 += i;
		}
	if (b2 > 0)
		b = lshift(b, b2);
	if (s2 > 0)
		S = lshift(S, s2);
	if (k_check) {
		if (cmp(b,S) < 0) {
			k--;
			b = multadd(b, 10, 0);	
			if (leftright)
				mhi = multadd(mhi, 10, 0);
			ilim = ilim1;
			}
		}
	if (ilim <= 0 && (mode == 3 || mode == 5)) {
		if (ilim < 0 || cmp(b,S = multadd(S,5,0)) < 0) {
			
 no_digits:
			
			*s++ = '0';
			k = 0;
			goto ret;
			}
 one_digit:
		*s++ = '1';
		k++;
		goto ret;
		}
	if (leftright) {
		if (m2 > 0)
			mhi = lshift(mhi, m2);

		



		mlo = mhi;
		if (spec_case) {
			mhi = Balloc(mhi->k);
			Bcopy(mhi, mlo);
			mhi = lshift(mhi, Log2P);
			}

		for(i = 1;;i++) {
			dig = quorem(b,S) + '0';
			


			j = cmp(b, mlo);
			delta = diff(S, mhi);
			j1 = delta->sign ? 1 : cmp(b, delta);
			Bfree(delta);
#ifndef ROUND_BIASED
			if (j1 == 0 && mode != 1 && !(word1(d) & 1)
#ifdef Honor_FLT_ROUNDS
				&& rounding >= 1
#endif
								   ) {
				if (dig == '9')
					goto round_9_up;
				if (j > 0)
					dig++;
#ifdef SET_INEXACT
				else if (!b->x[0] && b->wds <= 1)
					inexact = 0;
#endif
				*s++ = dig;
				goto ret;
				}
#endif
			if (j < 0 || (j == 0 && mode != 1
#ifndef ROUND_BIASED
							&& !(word1(d) & 1)
#endif
					)) {
				if (!b->x[0] && b->wds <= 1) {
#ifdef SET_INEXACT
					inexact = 0;
#endif
					goto accept_dig;
					}
#ifdef Honor_FLT_ROUNDS
				if (mode > 1)
				 switch(rounding) {
				  case 0: goto accept_dig;
				  case 2: goto keep_dig;
				  }
#endif 
				if (j1 > 0) {
					b = lshift(b, 1);
					j1 = cmp(b, S);
					if ((j1 > 0 || (j1 == 0 && dig & 1))
					&& dig++ == '9')
						goto round_9_up;
					}
 accept_dig:
				*s++ = dig;
				goto ret;
				}
			if (j1 > 0) {
#ifdef Honor_FLT_ROUNDS
				if (!rounding)
					goto accept_dig;
#endif
				if (dig == '9') { 
 round_9_up:
					*s++ = '9';
					goto roundoff;
					}
				*s++ = dig + 1;
				goto ret;
				}
#ifdef Honor_FLT_ROUNDS
 keep_dig:
#endif
			*s++ = dig;
			if (i == ilim)
				break;
			b = multadd(b, 10, 0);
			if (mlo == mhi)
				mlo = mhi = multadd(mhi, 10, 0);
			else {
				mlo = multadd(mlo, 10, 0);
				mhi = multadd(mhi, 10, 0);
				}
			}
		}
	else
		for(i = 1;; i++) {
			*s++ = dig = quorem(b,S) + '0';
			if (!b->x[0] && b->wds <= 1) {
#ifdef SET_INEXACT
				inexact = 0;
#endif
				goto ret;
				}
			if (i >= ilim)
				break;
			b = multadd(b, 10, 0);
			}

	

#ifdef Honor_FLT_ROUNDS
	switch(rounding) {
	  case 0: goto trimzeros;
	  case 2: goto roundoff;
	  }
#endif
	b = lshift(b, 1);
	j = cmp(b, S);
	if (j >= 0) {  
 roundoff:
		while(*--s == '9')
			if (s == s0) {
				k++;
				*s++ = '1';
				goto ret;
				}
		++*s++;
		}
	else {
#ifdef Honor_FLT_ROUNDS
 trimzeros:
#endif
		while(*--s == '0');
		s++;
		}
 ret:
	Bfree(S);
	if (mhi) {
		if (mlo && mlo != mhi)
			Bfree(mlo);
		Bfree(mhi);
		}
 ret1:
#ifdef SET_INEXACT
	if (inexact) {
		if (!oldinexact) {
			word0(d) = Exp_1 + (70 << Exp_shift);
			word1(d) = 0;
			dval(d) += 1.;
			}
		}
	else if (!oldinexact)
		clear_inexact();
#endif
	Bfree(b);
	*s = 0;
	*decpt = k + 1;
	if (rve)
		*rve = s;
	return s0;
	}
#ifdef __cplusplus
}
#endif
