







#include "bbs_rand.h"

#define SEED     1
#define MODULUS  2



static char *bbs_modulus = 
"75A2A6E1D27393B86562B9CE7279A8403CB4258A637DAB5233465373E37837383EDC"
"332282B8575927BC4172CE8C147B4894050EE9D2BDEED355C121037270CA2570D127"
"7D2390CD1002263326635CC6B259148DE3A1A03201980A925E395E646A5E9164B0EC"
"28559EBA58C87447245ADD0651EDA507056A1129E3A3E16E903D64B437";

static int    bbs_init = 0;  
static mp_int bbs_state;     


int           bbs_seed_size = (sizeof(bbs_modulus) / 2);

void         bbs_srand(unsigned char *data, int len)
{
  if((bbs_init & SEED) == 0) {
    mp_init(&bbs_state);
    bbs_init |= SEED;
  }

  mp_read_raw(&bbs_state, (char *)data, len);

} 

unsigned int bbs_rand(void)
{
  static mp_int   modulus;
  unsigned int    result = 0, ix;

  if((bbs_init & MODULUS) == 0) {
    mp_init(&modulus);
    mp_read_radix(&modulus, bbs_modulus, 16);
    bbs_init |= MODULUS;
  }

  for(ix = 0; ix < sizeof(unsigned int); ix++) {
    mp_digit   d;

    mp_sqrmod(&bbs_state, &modulus, &bbs_state);
    d = DIGIT(&bbs_state, 0);

    result = (result << CHAR_BIT) | (d & UCHAR_MAX);
  }

  return result;

} 



