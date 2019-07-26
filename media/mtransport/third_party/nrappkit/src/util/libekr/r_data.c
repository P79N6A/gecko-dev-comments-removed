

















































































static char *RCSSTRING __UNUSED__ =(char *)"$Id: r_data.c,v 1.2 2006/08/16 19:39:17 adamcain Exp $";

#include <string.h>
#include <r_common.h>
#include <r_data.h>
#include <string.h>

int r_data_create(dp,d,l)
  Data **dp;
  UCHAR *d;
  int l;
  {
    Data *d_=0;
    int _status;

    if(!(d_=(Data *)RCALLOC(sizeof(Data))))
      ABORT(R_NO_MEMORY);
    if(!(d_->data=(UCHAR *)RMALLOC(l)))
      ABORT(R_NO_MEMORY);

    memcpy(d_->data,d,l);
    d_->len=l;

    *dp=d_;

    _status=0;
  abort:
    if(_status)
      r_data_destroy(&d_);

    return(_status);
  }


int r_data_alloc_mem(d,l)
  Data *d;
  int l;
  {
    int _status;

    if(!(d->data=(UCHAR *)RMALLOC(l)))
      ABORT(R_NO_MEMORY);
    d->len=l;

    _status=0;
  abort:
    return(_status);
  }

int r_data_alloc(dp,l)
  Data **dp;
  int l;
  {
    Data *d_=0;
    int _status;

    if(!(d_=(Data *)RCALLOC(sizeof(Data))))
      ABORT(R_NO_MEMORY);
    if(!(d_->data=(UCHAR *)RCALLOC(l)))
      ABORT(R_NO_MEMORY);

    d_->len=l;

    *dp=d_;
    _status=0;
  abort:
    if(_status)
      r_data_destroy(&d_);

    return(_status);
  }

int r_data_make(dp,d,l)
  Data *dp;
  UCHAR *d;
  int l;
  {
    if(!(dp->data=(UCHAR *)RMALLOC(l)))
      ERETURN(R_NO_MEMORY);

    memcpy(dp->data,d,l);
    dp->len=l;

    return(0);
  }

int r_data_destroy(dp)
  Data **dp;
  {
    if(!dp || !*dp)
      return(0);

    if((*dp)->data)
      RFREE((*dp)->data);

    RFREE(*dp);
    *dp=0;

    return(0);
  }

int r_data_destroy_v(v)
  void *v;
  {
    Data *d;

    if(!v)
      return(0);

    d=(Data *)v;
    r_data_zfree(d);

    RFREE(d);

    return(0);
  }

int r_data_destroy_vp(v)
  void **v;
  {
    Data *d;

    if(!v || !*v)
      return(0);

    d=(Data *)*v;
    r_data_zfree(d);

    *v=0;
    RFREE(d);

    return(0);
  }

int r_data_copy(dst,src)
  Data *dst;
  Data *src;
  {
    if(!(dst->data=(UCHAR *)RMALLOC(src->len)))
      ERETURN(R_NO_MEMORY);
    memcpy(dst->data,src->data,dst->len=src->len);
    return(0);
  }

int r_data_zfree(d)
  Data *d;
  {
    if(!d)
      return(0);
    if(!d->data)
      return(0);
    memset(d->data,0,d->len);
    RFREE(d->data);
    return(0);
  }

int r_data_compare(d1,d2)
  Data *d1;
  Data *d2;
  {
    if(d1->len<d2->len)
      return(-1);
    if(d2->len<d1->len)
      return(-1);
    return(memcmp(d1->data,d2->data,d1->len));
  }

