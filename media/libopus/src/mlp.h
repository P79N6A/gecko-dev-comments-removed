


























#ifndef _MLP_H_
#define _MLP_H_

#include "arch.h"

typedef struct {
	int layers;
	const int *topo;
	const opus_val16 *weights;
} MLP;

void mlp_process(const MLP *m, const opus_val16 *in, opus_val16 *out);

#endif 
