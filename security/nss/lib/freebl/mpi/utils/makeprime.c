












#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>



#include "mpi.h"
#include "mpprime.h"













mp_err   make_prime(mp_int *p, int nr);


int main(int argc, char *argv[])
{
  mp_int    start;
  mp_err    res;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <start-value>\n", argv[0]);
    return 1;
  }
	    
  mp_init(&start);
  if(argv[1][0] == '0' && tolower(argv[1][1]) == 'x') {
    mp_read_radix(&start, argv[1] + 2, 16);
  } else {
    mp_read_radix(&start, argv[1], 10);
  }
  mp_abs(&start, &start);

  if((res = make_prime(&start, 5)) != MP_OKAY) {
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));
    mp_clear(&start);

    return 1;

  } else {
    char  *buf = malloc(mp_radix_size(&start, 10));

    mp_todecimal(&start, buf);
    printf("%s\n", buf);
    free(buf);
    
    mp_clear(&start);

    return 0;
  }
  
} 



mp_err   make_prime(mp_int *p, int nr)
{
  mp_err  res;

  if(mp_iseven(p)) {
    mp_add_d(p, 1, p);
  }

  do {
    mp_digit   which = prime_tab_size;

    
    if((res = mpp_divis_primes(p, &which)) == MP_YES)
      continue;
    else if(res != MP_NO)
      goto CLEANUP;

    
    if((res = mpp_fermat(p, 2)) == MP_NO)
      continue;
    else if(res != MP_YES)
      goto CLEANUP;

    
    if((res = mpp_pprime(p, nr)) == MP_YES)
      break;
    else if(res != MP_NO)
      goto CLEANUP;
      
  } while((res = mp_add_d(p, 2, p)) == MP_OKAY);

 CLEANUP:
  return res;

} 



