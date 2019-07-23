












































#ifndef _MPI_PRIV_H_
#define _MPI_PRIV_H_ 1

#include "mpi.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if MP_DEBUG
#include <stdio.h>

#define DIAG(T,V) {fprintf(stderr,T);mp_print(V,stderr);fputc('\n',stderr);}
#else
#define DIAG(T,V)
#endif







#if MP_LOGTAB















extern const float s_logv_2[];
#define LOG_V_2(R)  s_logv_2[(R)]

#else







#include <math.h>
#define LOG_V_2(R)  (log(2.0)/log(R))

#endif 













#define  CARRYOUT(W)  (mp_digit)((W)>>DIGIT_BIT)
#define  ACCUM(W)     (mp_digit)(W)

#define MP_MIN(a,b)   (((a) < (b)) ? (a) : (b))
#define MP_MAX(a,b)   (((a) > (b)) ? (a) : (b))
#define MP_HOWMANY(a,b) (((a) + (b) - 1)/(b))
#define MP_ROUNDUP(a,b) (MP_HOWMANY(a,b) * (b))





#define  MP_LT       -1
#define  MP_EQ        0
#define  MP_GT        1

















#if MP_MACRO == 0
 void     s_mp_setz(mp_digit *dp, mp_size count); 
 void     s_mp_copy(const mp_digit *sp, mp_digit *dp, mp_size count); 
 void    *s_mp_alloc(size_t nb, size_t ni);       
 void     s_mp_free(void *ptr);                   
extern unsigned long mp_allocs;
extern unsigned long mp_frees;
extern unsigned long mp_copies;
#else

 


 #if MP_MEMSET == 0
  #define  s_mp_setz(dp, count) \
       {int ix;for(ix=0;ix<(count);ix++)(dp)[ix]=0;}
 #else
  #define  s_mp_setz(dp, count) memset(dp, 0, (count) * sizeof(mp_digit))
 #endif 

 #if MP_MEMCPY == 0
  #define  s_mp_copy(sp, dp, count) \
       {int ix;for(ix=0;ix<(count);ix++)(dp)[ix]=(sp)[ix];}
 #else
  #define  s_mp_copy(sp, dp, count) memcpy(dp, sp, (count) * sizeof(mp_digit))
 #endif 

 #define  s_mp_alloc(nb, ni)  calloc(nb, ni)
 #define  s_mp_free(ptr) {if(ptr) free(ptr);}
#endif 

mp_err   s_mp_grow(mp_int *mp, mp_size min);   
mp_err   s_mp_pad(mp_int *mp, mp_size min);    

#if MP_MACRO == 0
 void     s_mp_clamp(mp_int *mp);               
#else
 #define  s_mp_clamp(mp)\
  { mp_size used = MP_USED(mp); \
    while (used > 1 && DIGIT(mp, used - 1) == 0) --used; \
    MP_USED(mp) = used; \
  } 
#endif 

void     s_mp_exch(mp_int *a, mp_int *b);      

mp_err   s_mp_lshd(mp_int *mp, mp_size p);     
void     s_mp_rshd(mp_int *mp, mp_size p);     
mp_err   s_mp_mul_2d(mp_int *mp, mp_digit d);  
void     s_mp_div_2d(mp_int *mp, mp_digit d);  
void     s_mp_mod_2d(mp_int *mp, mp_digit d);  
void     s_mp_div_2(mp_int *mp);               
mp_err   s_mp_mul_2(mp_int *mp);               
mp_err   s_mp_norm(mp_int *a, mp_int *b, mp_digit *pd); 
                                               
mp_err   s_mp_add_d(mp_int *mp, mp_digit d);   
mp_err   s_mp_sub_d(mp_int *mp, mp_digit d);   
mp_err   s_mp_mul_d(mp_int *mp, mp_digit d);   
mp_err   s_mp_div_d(mp_int *mp, mp_digit d, mp_digit *r);
		                               
mp_err   s_mp_reduce(mp_int *x, const mp_int *m, const mp_int *mu);
                                               
mp_err   s_mp_add(mp_int *a, const mp_int *b); 
mp_err   s_mp_add_3arg(const mp_int *a, const mp_int *b, mp_int *c);
mp_err   s_mp_sub(mp_int *a, const mp_int *b); 
mp_err   s_mp_sub_3arg(const mp_int *a, const mp_int *b, mp_int *c);
mp_err   s_mp_add_offset(mp_int *a, mp_int *b, mp_size offset);
                                               
mp_err   s_mp_mul(mp_int *a, const mp_int *b); 
#if MP_SQUARE
mp_err   s_mp_sqr(mp_int *a);                  
#else
#define  s_mp_sqr(a) s_mp_mul(a, a)
#endif
mp_err   s_mp_div(mp_int *rem, mp_int *div, mp_int *quot); 
mp_err   s_mp_exptmod(const mp_int *a, const mp_int *b, const mp_int *m, mp_int *c);
mp_err   s_mp_2expt(mp_int *a, mp_digit k);    
int      s_mp_cmp(const mp_int *a, const mp_int *b); 
int      s_mp_cmp_d(const mp_int *a, mp_digit d); 
int      s_mp_ispow2(const mp_int *v);         
int      s_mp_ispow2d(mp_digit d);             

int      s_mp_tovalue(char ch, int r);          
char     s_mp_todigit(mp_digit val, int r, int low); 
int      s_mp_outlen(int bits, int r);          
mp_digit s_mp_invmod_radix(mp_digit P);   
mp_err   s_mp_invmod_odd_m( const mp_int *a, const mp_int *m, mp_int *c);
mp_err   s_mp_invmod_2d(    const mp_int *a, mp_size k,       mp_int *c);
mp_err   s_mp_invmod_even_m(const mp_int *a, const mp_int *m, mp_int *c);

#ifdef NSS_USE_COMBA

#define IS_POWER_OF_2(a) ((a) && !((a) & ((a)-1)))

void s_mp_mul_comba_4(const mp_int *A, const mp_int *B, mp_int *C);
void s_mp_mul_comba_8(const mp_int *A, const mp_int *B, mp_int *C);
void s_mp_mul_comba_16(const mp_int *A, const mp_int *B, mp_int *C);
void s_mp_mul_comba_32(const mp_int *A, const mp_int *B, mp_int *C);

void s_mp_sqr_comba_4(const mp_int *A, mp_int *B);
void s_mp_sqr_comba_8(const mp_int *A, mp_int *B);
void s_mp_sqr_comba_16(const mp_int *A, mp_int *B);
void s_mp_sqr_comba_32(const mp_int *A, mp_int *B);

#endif 


#if defined (__OS2__) && defined (__IBMC__)
#define MPI_ASM_DECL __cdecl
#else
#define MPI_ASM_DECL
#endif

#ifdef MPI_AMD64

mp_digit MPI_ASM_DECL s_mpv_mul_set_vec64(mp_digit*, mp_digit *, mp_size, mp_digit);
mp_digit MPI_ASM_DECL s_mpv_mul_add_vec64(mp_digit*, const mp_digit*, mp_size, mp_digit);


#define s_mpv_mul_d(a, a_len, b, c) \
	((mp_digit *)c)[a_len] = s_mpv_mul_set_vec64(c, a, a_len, b)


#define s_mpv_mul_d_add(a, a_len, b, c) \
	((mp_digit *)c)[a_len] = s_mpv_mul_add_vec64(c, a, a_len, b)


#else

void     MPI_ASM_DECL s_mpv_mul_d(const mp_digit *a, mp_size a_len,
                                        mp_digit b, mp_digit *c);
void     MPI_ASM_DECL s_mpv_mul_d_add(const mp_digit *a, mp_size a_len,
                                            mp_digit b, mp_digit *c);

#endif

void     MPI_ASM_DECL s_mpv_mul_d_add_prop(const mp_digit *a,
                                                mp_size a_len, mp_digit b, 
			                        mp_digit *c);
void     MPI_ASM_DECL s_mpv_sqr_add_prop(const mp_digit *a,
                                                mp_size a_len,
                                                mp_digit *sqrs);

mp_err   MPI_ASM_DECL s_mpv_div_2dx1d(mp_digit Nhi, mp_digit Nlo,
                            mp_digit divisor, mp_digit *quot, mp_digit *rem);


#define s_mp_mul_d_add_offset(a, b, c, off) \
(s_mpv_mul_d_add_prop(MP_DIGITS(a), MP_USED(a), b, MP_DIGITS(c) + off), MP_OKAY)

typedef struct {
  mp_int       N;	
  mp_digit     n0prime; 
  mp_size      b;	
} mp_mont_modulus;

mp_err s_mp_mul_mont(const mp_int *a, const mp_int *b, mp_int *c, 
	               mp_mont_modulus *mmm);
mp_err s_mp_redc(mp_int *T, mp_mont_modulus *mmm);












unsigned long s_mpi_getProcessorLineSize();


#endif

