


























#ifndef _MLP_H_
#define _MLP_H_

#include "arch.h"

typedef struct {
	int layers;
	const int *topo;
	const float *weights;
} MLP;

void mlp_process(const MLP *m, const float *in, float *out);

#endif 
