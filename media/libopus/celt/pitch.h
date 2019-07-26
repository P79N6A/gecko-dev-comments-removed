








































#ifndef PITCH_H
#define PITCH_H

#include "modes.h"

void pitch_downsample(celt_sig * restrict x[], opus_val16 * restrict x_lp,
      int len, int C);

void pitch_search(const opus_val16 * restrict x_lp, opus_val16 * restrict y,
                  int len, int max_pitch, int *pitch);

opus_val16 remove_doubling(opus_val16 *x, int maxperiod, int minperiod,
      int N, int *T0, int prev_period, opus_val16 prev_gain);

#endif
