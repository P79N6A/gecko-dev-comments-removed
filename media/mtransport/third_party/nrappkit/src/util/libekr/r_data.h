


















































































#ifndef _r_data_h
#define _r_data_h

typedef struct Data_ {
     UCHAR *data;
     int len;
} Data;

int r_data_create(Data **dp,UCHAR *d,int l);
int r_data_alloc(Data **dp, int l);
int r_data_make(Data *dp, UCHAR *d,int l);
int r_data_alloc_mem(Data *d,int l);
int r_data_destroy(Data **dp);
int r_data_destroy_v(void *v);
int r_data_destroy_vp(void **vp);
int r_data_copy(Data *dst,Data *src);
int r_data_zfree(Data *d);
int r_data_compare(Data *d1,Data *d2);

#define INIT_DATA(a,b,c) (a).data=b; (a).len=c
#define ATTACH_DATA(a,b) (a).data=b; (a).len=sizeof(b)
#define ZERO_DATA(a) (a).data=0; (a).len=0

#endif

