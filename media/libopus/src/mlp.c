


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "mlp.h"
#include "arch.h"
#include "tansig_table.h"
#define MAX_NEURONS 100

#ifdef FIXED_POINT
static inline opus_val16 tansig_approx(opus_val32 _x) 
{
	int i;
	opus_val16 xx; 
	
	opus_val16 dy, yy; 
	
	if (_x>=QCONST32(10,19))
		return QCONST32(1.,14);
	if (_x<=-QCONST32(10,19))
		return -QCONST32(1.,14);
	xx = EXTRACT16(SHR32(_x, 8));
	
	i = SHR32(ADD32(1024,MULT16_16(25, xx)),11);
	
	xx -= EXTRACT16(SHR32(MULT16_16(20972,i),8));
	
	
	yy = tansig_table[250+i];
	
	dy = 16384-MULT16_16_Q14(yy,yy);
	yy = yy + MULT16_16_Q14(MULT16_16_Q11(xx,dy),(16384 - MULT16_16_Q11(yy,xx)));
	return yy;
}
#else

static inline opus_val16 tansig_approx(opus_val16 x)
{
	int i;
	opus_val16 y, dy;
	opus_val16 sign=1;
    if (x>=8)
        return 1;
    if (x<=-8)
        return -1;
	if (x<0)
	{
	   x=-x;
	   sign=-1;
	}
	i = (int)floor(.5f+25*x);
	x -= .04f*i;
	y = tansig_table[i];
	dy = 1-y*y;
	y = y + x*dy*(1 - y*x);
	return sign*y;
}
#endif

void mlp_process(const MLP *m, const opus_val16 *in, opus_val16 *out)
{
	int j;
	opus_val16 hidden[MAX_NEURONS];
	const opus_val16 *W = m->weights;
	
	for (j=0;j<m->topo[1];j++)
	{
		int k;
		opus_val32 sum = SHL32(EXTEND32(*W++),8);
		for (k=0;k<m->topo[0];k++)
			sum = MAC16_16(sum, in[k],*W++);
		hidden[j] = tansig_approx(sum);
	}
	for (j=0;j<m->topo[2];j++)
	{
		int k;
		opus_val32 sum = SHL32(EXTEND32(*W++),14);
		for (k=0;k<m->topo[1];k++)
			sum = MAC16_16(sum, hidden[k], *W++);
		out[j] = tansig_approx(EXTRACT16(PSHR32(sum,17)));
	}
}

