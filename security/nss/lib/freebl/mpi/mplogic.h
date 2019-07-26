









#ifndef _H_MPLOGIC_
#define _H_MPLOGIC_

#include "mpi.h"










#define MP_EVEN       MP_YES
#define MP_ODD        MP_NO



mp_err mpl_not(mp_int *a, mp_int *b);            
mp_err mpl_and(mp_int *a, mp_int *b, mp_int *c); 
mp_err mpl_or(mp_int *a, mp_int *b, mp_int *c);  
mp_err mpl_xor(mp_int *a, mp_int *b, mp_int *c); 



mp_err mpl_rsh(const mp_int *a, mp_int *b, mp_digit d);   
mp_err mpl_lsh(const mp_int *a, mp_int *b, mp_digit d);   



mp_err mpl_num_set(mp_int *a, int *num);         
mp_err mpl_num_clear(mp_int *a, int *num);       
mp_err mpl_parity(mp_int *a);                    



mp_err mpl_set_bit(mp_int *a, mp_size bitNum, mp_size value);
mp_err mpl_get_bit(const mp_int *a, mp_size bitNum);
mp_err mpl_get_bits(const mp_int *a, mp_size lsbNum, mp_size numBits);
mp_err mpl_significant_bits(const mp_int *a);

#endif 
