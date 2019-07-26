



#ifndef MPI_AMD64
#error This file only works on AMD64 platforms.
#endif

#include <mpi-priv.h>








void MPI_ASM_DECL s_mpv_mul_d_add_prop(const mp_digit *a, mp_size a_len,
                                       mp_digit b, mp_digit *c)
{
  mp_digit w;
  mp_digit d;

  d = s_mpv_mul_add_vec64(c, a, a_len, b);
  c += a_len;
  while (d) {
    w = c[0] + d;
    d = (w < c[0] || w < d);
    *c++ = w;
  }
}

