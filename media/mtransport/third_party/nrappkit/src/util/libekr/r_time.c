

















































































static char *RCSSTRING __UNUSED__ ="$Id: r_time.c,v 1.5 2008/11/26 03:22:02 adamcain Exp $";

#include <r_common.h>
#include <r_time.h>


int r_timeval_diff(t1,t0,diff)
  struct timeval *t1;
  struct timeval *t0;
  struct timeval *diff;
  {
    long d;

    if(t0->tv_sec > t1->tv_sec)
      ERETURN(R_BAD_ARGS);
    if((t0->tv_sec == t1->tv_sec) && (t0->tv_usec > t1->tv_usec))
      ERETURN(R_BAD_ARGS);

    
    if(t0->tv_usec <= t1->tv_usec){
      diff->tv_sec=t1->tv_sec - t0->tv_sec;
      diff->tv_usec=t1->tv_usec - t0->tv_usec;
      return(0);
    }

    
    d=t0->tv_usec - t1->tv_usec;
    if(t1->tv_sec < (t0->tv_sec + 1))
      ERETURN(R_BAD_ARGS);
    diff->tv_sec=t1->tv_sec - (t0->tv_sec + 1);
    diff->tv_usec=1000000 - d;

    return(0);
  }

int r_timeval_add(t1,t2,sum)
  struct timeval *t1;
  struct timeval *t2;
  struct timeval *sum;
  {
    long tv_sec,tv_usec,d;

    tv_sec=t1->tv_sec + t2->tv_sec;

    d=t1->tv_usec + t2->tv_usec;
    if(d>1000000){
      tv_sec++;
      tv_usec=d-1000000;
    }
    else{
      tv_usec=d;
    }

    sum->tv_sec=tv_sec;
    sum->tv_usec=tv_usec;

    return(0);
  }

int r_timeval_cmp(t1,t2)
  struct timeval *t1;
  struct timeval *t2;
  {
    if(t1->tv_sec>t2->tv_sec)
      return(1);
    if(t1->tv_sec<t2->tv_sec)
      return(-1);
    if(t1->tv_usec>t2->tv_usec)
      return(1);
    if(t1->tv_usec<t2->tv_usec)
      return(-1);
    return(0);
  }


UINT8 r_timeval2int(tv)
  struct timeval *tv;
  {
    UINT8 r=0;

    r=(tv->tv_sec);
    r*=1000000;
    r+=tv->tv_usec;

    return r;
  }

int r_int2timeval(UINT8 t,struct timeval *tv)
  {
    tv->tv_sec=t/1000000;
    tv->tv_usec=t%1000000;

    return(0);
  }

UINT8 r_gettimeint()
  {

#ifdef WIN32
    struct timeval tv = {0,0};

#else
    struct timeval tv;

    gettimeofday(&tv,0);
#endif

    return r_timeval2int(&tv);
  }


int r_timeval_diff_usec(struct timeval *t1, struct timeval *t0, INT8 *diff)
  {
    int r,_status;
    int sign;
    struct timeval tmp;

    sign = 1;
    if (r=r_timeval_diff(t1, t0, &tmp)) {
        if (r == R_BAD_ARGS) {
            sign = -1;
            if (r=r_timeval_diff(t0, t1, &tmp))
                ABORT(r);
        }
    }

    


    *diff = ((tmp.tv_sec * (1000*1000)) + tmp.tv_usec) * sign;

    _status = 0;
  abort:
    return(_status);
  }


int r_timeval_diff_ms(struct timeval *t1, struct timeval *t0, INT8 *diff)
  {
    int r,_status;
    int sign;
    struct timeval tmp;

    sign = 1;
    if (r=r_timeval_diff(t1, t0, &tmp)) {
        if (r == R_BAD_ARGS) {
            sign = -1;
            if (r=r_timeval_diff(t0, t1, &tmp))
                ABORT(r);
        }
    }

    


    *diff = ((tmp.tv_sec * 1000) + (tmp.tv_usec / 1000)) * sign;

    _status = 0;
  abort:
    return(_status);
  }

