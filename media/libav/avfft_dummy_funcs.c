














#include "libavcodec/fft.h"

void
ff_fft_init_aarch64(FFTContext *s)
{
}

void
ff_fft_init_arm(FFTContext *s)
{
}

void
ff_fft_init_ppc(FFTContext *s)
{
}

void
ff_fft_fixed_init_arm(FFTContext *s)
{
}

void
ff_rdft_init_arm(RDFTContext *s)
{
}

int
ff_get_cpu_flags_aarch64(void)
{
  return 0;
}

int
ff_get_cpu_flags_arm(void)
{
  return 0;
}

int
ff_get_cpu_flags_ppc(void)
{
  return 0;
}

void
ff_mdct_calcw_c(FFTContext *s, FFTDouble *out, const FFTSample *input)
{
}
