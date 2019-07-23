



















































#include "fdlibm.h"


#if _LIB_VERSION == _IEEE_ && !(defined(DARWIN) || defined(XP_MACOSX))
   int errno;
#  define EDOM 0
#  define ERANGE 0
#else
#  include <errno.h>
#endif


#ifndef _USE_WRITE
#include <stdio.h>			
#define	WRITE2(u,v)	fputs(u, stderr)
#else	
#include <unistd.h>			
#define	WRITE2(u,v)	write(2, u, v)
#undef fflush
#endif	

static double zero = 0.0;	

















































#ifdef __STDC__
	double __kernel_standard(double x, double y, int type, int *err)
#else
	double __kernel_standard(x,y,type, err)
     double x,y; int type;int *err;
#endif
{
	struct exception exc;
#ifndef HUGE_VAL	 
#define HUGE_VAL inf
	double inf = 0.0;
        fd_twoints u;

        u.d = inf;
	__HI(u) = 0x7ff00000;	
        inf = u.d;
#endif

    *err = 0;

#ifdef _USE_WRITE
	(void) fflush(stdout);
#endif
	exc.arg1 = x;
	exc.arg2 = y;
	switch(type) {
	    case 1:
		
		exc.type = DOMAIN;
		exc.name = "acos";
		exc.retval = zero;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if(_LIB_VERSION == _SVID_) {
		    (void) WRITE2("acos: DOMAIN error\n", 19);
		  }
		  *err = EDOM;
		}
		break;
	    case 2:
		
		exc.type = DOMAIN;
		exc.name = "asin";
		exc.retval = zero;
		if(_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if(_LIB_VERSION == _SVID_) {
		    	(void) WRITE2("asin: DOMAIN error\n", 19);
		  }
		  *err = EDOM;
		}
		break;
	    case 3:
		
		exc.arg1 = y;
		exc.arg2 = x;
		exc.type = DOMAIN;
		exc.name = "atan2";
		exc.retval = zero;
		if(_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if(_LIB_VERSION == _SVID_) {
			(void) WRITE2("atan2: DOMAIN error\n", 20);
		      }
		  *err = EDOM;
		}
		break;
	    case 4:
		
		exc.type = OVERFLOW;
		exc.name = "hypot";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = HUGE;
		else
		  exc.retval = HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 5:
		
		exc.type = OVERFLOW;
		exc.name = "cosh";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = HUGE;
		else
		  exc.retval = HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 6:
		
		exc.type = OVERFLOW;
		exc.name = "exp";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = HUGE;
		else
		  exc.retval = HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 7:
		
		exc.type = UNDERFLOW;
		exc.name = "exp";
		exc.retval = zero;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 8:
		
		exc.type = DOMAIN;	
		exc.name = "y0";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("y0: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 9:
		
		exc.type = DOMAIN;
		exc.name = "y0";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("y0: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 10:
		
		exc.type = DOMAIN;	
		exc.name = "y1";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("y1: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 11:
		
		exc.type = DOMAIN;
		exc.name = "y1";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("y1: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 12:
		
		exc.type = DOMAIN;	
		exc.name = "yn";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("yn: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 13:
		
		exc.type = DOMAIN;
		exc.name = "yn";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("yn: DOMAIN error\n", 17);
		      }
		  *err = EDOM;
		}
		break;
	    case 14:
		
		exc.type = OVERFLOW;
		exc.name = "lgamma";
                if (_LIB_VERSION == _SVID_)
                  exc.retval = HUGE;
                else
                  exc.retval = HUGE_VAL;
                if (_LIB_VERSION == _POSIX_)
			*err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        *err = ERANGE;
		}
		break;
	    case 15:
		
		exc.type = SING;
		exc.name = "lgamma";
                if (_LIB_VERSION == _SVID_)
                  exc.retval = HUGE;
                else
                  exc.retval = HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("lgamma: SING error\n", 19);
		      }
		  *err = EDOM;
		}
		break;
	    case 16:
		
		exc.type = SING;
		exc.name = "log";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("log: SING error\n", 16);
		      }
		  *err = EDOM;
		}
		break;
	    case 17:
		
		exc.type = DOMAIN;
		exc.name = "log";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("log: DOMAIN error\n", 18);
		      }
		  *err = EDOM;
		}
		break;
	    case 18:
		
		exc.type = SING;
		exc.name = "log10";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("log10: SING error\n", 18);
		      }
		  *err = EDOM;
		}
		break;
	    case 19:
		
		exc.type = DOMAIN;
		exc.name = "log10";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = -HUGE;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("log10: DOMAIN error\n", 20);
		      }
		  *err = EDOM;
		}
		break;
	    case 20:
		
		
		exc.type = DOMAIN;
		exc.name = "pow";
		exc.retval = zero;
		if (_LIB_VERSION != _SVID_) exc.retval = 1.0;
		else if (!fd_matherr(&exc)) {
			(void) WRITE2("pow(0,0): DOMAIN error\n", 23);
			*err = EDOM;
		}
		break;
	    case 21:
		
		exc.type = OVERFLOW;
		exc.name = "pow";
		if (_LIB_VERSION == _SVID_) {
		  exc.retval = HUGE;
		  y *= 0.5;
		  if(x<zero&&fd_rint(y)!=y) exc.retval = -HUGE;
		} else {
		  exc.retval = HUGE_VAL;
		  y *= 0.5;
		  if(x<zero&&fd_rint(y)!=y) exc.retval = -HUGE_VAL;
		}
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 22:
		
		exc.type = UNDERFLOW;
		exc.name = "pow";
		exc.retval =  zero;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 23:
		
		exc.type = DOMAIN;
		exc.name = "pow";
		if (_LIB_VERSION == _SVID_) 
		  exc.retval = zero;
		else
		  exc.retval = -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("pow(0,neg): DOMAIN error\n", 25);
		      }
		  *err = EDOM;
		}
		break;
	    case 24:
		
		exc.type = DOMAIN;
		exc.name = "pow";
		if (_LIB_VERSION == _SVID_) 
		    exc.retval = zero;
		else 
		    exc.retval = zero/zero;	
		if (_LIB_VERSION == _POSIX_) 
		   *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("neg**non-integral: DOMAIN error\n", 32);
		      }
		  *err = EDOM;
		}
		break;
	    case 25:
		
		exc.type = OVERFLOW;
		exc.name = "sinh";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = ( (x>zero) ? HUGE : -HUGE);
		else
		  exc.retval = ( (x>zero) ? HUGE_VAL : -HUGE_VAL);
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 26:
		
		exc.type = DOMAIN;
		exc.name = "sqrt";
		if (_LIB_VERSION == _SVID_)
		  exc.retval = zero;
		else
		  exc.retval = zero/zero;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("sqrt: DOMAIN error\n", 19);
		      }
		  *err = EDOM;
		}
		break;
            case 27:
                
                exc.type = DOMAIN;
                exc.name = "fmod";
                if (_LIB_VERSION == _SVID_)
                    exc.retval = x;
		else
		    exc.retval = zero/zero;
                if (_LIB_VERSION == _POSIX_)
                  *err = EDOM;
                else if (!fd_matherr(&exc)) {
                  if (_LIB_VERSION == _SVID_) {
                    (void) WRITE2("fmod:  DOMAIN error\n", 20);
                  }
                  *err = EDOM;
                }
                break;
            case 28:
                
                exc.type = DOMAIN;
                exc.name = "remainder";
                exc.retval = zero/zero;
                if (_LIB_VERSION == _POSIX_)
                  *err = EDOM;
                else if (!fd_matherr(&exc)) {
                  if (_LIB_VERSION == _SVID_) {
                    (void) WRITE2("remainder: DOMAIN error\n", 24);
                  }
                  *err = EDOM;
                }
                break;
            case 29:
                
                exc.type = DOMAIN;
                exc.name = "acosh";
                exc.retval = zero/zero;
                if (_LIB_VERSION == _POSIX_)
                  *err = EDOM;
                else if (!fd_matherr(&exc)) {
                  if (_LIB_VERSION == _SVID_) {
                    (void) WRITE2("acosh: DOMAIN error\n", 20);
                  }
                  *err = EDOM;
                }
                break;
            case 30:
                
                exc.type = DOMAIN;
                exc.name = "atanh";
                exc.retval = zero/zero;
                if (_LIB_VERSION == _POSIX_)
                  *err = EDOM;
                else if (!fd_matherr(&exc)) {
                  if (_LIB_VERSION == _SVID_) {
                    (void) WRITE2("atanh: DOMAIN error\n", 20);
                  }
                  *err = EDOM;
                }
                break;
            case 31:
                
                exc.type = SING;
                exc.name = "atanh";
		exc.retval = x/zero;	
                if (_LIB_VERSION == _POSIX_)
                  *err = EDOM;
                else if (!fd_matherr(&exc)) {
                  if (_LIB_VERSION == _SVID_) {
                    (void) WRITE2("atanh: SING error\n", 18);
                  }
                  *err = EDOM;
                }
                break;
	    case 32:
		
		exc.type = OVERFLOW;
		exc.name = "scalb";
		exc.retval = x > zero ? HUGE_VAL : -HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 33:
		
		exc.type = UNDERFLOW;
		exc.name = "scalb";
		exc.retval = fd_copysign(zero,x);
		if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
		else if (!fd_matherr(&exc)) {
			*err = ERANGE;
		}
		break;
	    case 34:
		
                exc.type = TLOSS;
                exc.name = "j0";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 35:
		
                exc.type = TLOSS;
                exc.name = "y0";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 36:
		
                exc.type = TLOSS;
                exc.name = "j1";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 37:
		
                exc.type = TLOSS;
                exc.name = "y1";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 38:
		
                exc.type = TLOSS;
                exc.name = "jn";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 39:
		
                exc.type = TLOSS;
                exc.name = "yn";
                exc.retval = zero;
                if (_LIB_VERSION == _POSIX_)
                        *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                        if (_LIB_VERSION == _SVID_) {
                                (void) WRITE2(exc.name, 2);
                                (void) WRITE2(": TLOSS error\n", 14);
                        }
                        *err = ERANGE;
                }        
		break;
	    case 40:
		
		exc.type = OVERFLOW;
		exc.name = "gamma";
                if (_LIB_VERSION == _SVID_)
                  exc.retval = HUGE;
                else
                  exc.retval = HUGE_VAL;
                if (_LIB_VERSION == _POSIX_)
		  *err = ERANGE;
                else if (!fd_matherr(&exc)) {
                  *err = ERANGE;
                }
		break;
	    case 41:
		
		exc.type = SING;
		exc.name = "gamma";
                if (_LIB_VERSION == _SVID_)
                  exc.retval = HUGE;
                else
                  exc.retval = HUGE_VAL;
		if (_LIB_VERSION == _POSIX_)
		  *err = EDOM;
		else if (!fd_matherr(&exc)) {
		  if (_LIB_VERSION == _SVID_) {
			(void) WRITE2("gamma: SING error\n", 18);
		      }
		  *err = EDOM;
		}
		break;
	    case 42:
		
		
		exc.type = DOMAIN;
		exc.name = "pow";
		exc.retval = x;
		if (_LIB_VERSION == _IEEE_ ||
		    _LIB_VERSION == _POSIX_) exc.retval = 1.0;
		else if (!fd_matherr(&exc)) {
			*err = EDOM;
		}
		break;
	}
	return exc.retval; 
}
