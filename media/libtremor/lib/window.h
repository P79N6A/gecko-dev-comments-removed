
















#ifndef _V_WINDOW_
#define _V_WINDOW_

extern const void *_vorbis_window(int type,int left);
extern void _vorbis_apply_window(ogg_int32_t *d,const void *window[2],
				 long *blocksizes,
				 int lW,int W,int nW);


#endif
