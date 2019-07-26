






#include "mpi-priv.h"
#include <unistd.h>

#include <stddef.h>

#include <strings.h>

extern void multacc512( 
   int             length,        
   const mp_digit *scalaraddr,    
   const mp_digit *multiplicand,  
   mp_digit *      result);       

extern void maxpy_little(
   int             length,        
   const mp_digit *scalaraddr,    
   const mp_digit *multiplicand,  
   mp_digit *      result);       

extern void add_diag_little(
   int            length,       
   const mp_digit *root,         
   mp_digit *      result);      

void 
s_mpv_sqr_add_prop(const mp_digit *pa, mp_size a_len, mp_digit *ps)
{
    add_diag_little(a_len, pa, ps);
}

#define MAX_STACK_DIGITS 258
#define MULTACC512_LEN   (512 / MP_DIGIT_BIT)
#define HP_MPY_ADD_FN    (a_len == MULTACC512_LEN ? multacc512 : maxpy_little)


void 
s_mpv_mul_d(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
    mp_digit x[MAX_STACK_DIGITS];
    mp_digit *px = x;
    size_t   xSize = 0;

    if (a == c) {
	if (a_len > MAX_STACK_DIGITS) {
	    xSize = sizeof(mp_digit) * (a_len + 2);
	    px = malloc(xSize);
	    if (!px)
		return;
	}
	memcpy(px, a, a_len * sizeof(*a));
	a = px;
    }
    s_mp_setz(c, a_len + 1);
    HP_MPY_ADD_FN(a_len, &b, a, c);
    if (px != x && px) {
	memset(px, 0, xSize);
	free(px);
    }
}


void     
s_mpv_mul_d_add(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
    c[a_len] = 0;	
    HP_MPY_ADD_FN(a_len, &b, a, c);
}


void     
s_mpv_mul_d_add_prop(const mp_digit *a, mp_size a_len, mp_digit b, 
			 mp_digit *c)
{
    HP_MPY_ADD_FN(a_len, &b, a, c);
}

