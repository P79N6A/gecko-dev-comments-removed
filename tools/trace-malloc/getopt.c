






















































#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getopt.c    4.12 (Berkeley) 6/1/90";
#endif 

#include <stdio.h>
#include <string.h>
#define index strchr
#define rindex strrchr




int     opterr = 1,             
	optind = 1,             
	optopt;                 
char    *optarg;                

#define BADCH   (int)'?'
#define EMSG    ""

getopt(int nargc, char **nargv, char *ostr)
{
	static char *place = EMSG;              
	register char *oli;                     
	char *p;

	if (!*place) {                          
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return(EOF);
		}
		if (place[1] && *++place == '-') {      
			++optind;
			place = EMSG;
			return(EOF);
		}
	}                                       
	if ((optopt = (int)*place++) == (int)':' ||
	    !(oli = index(ostr, optopt))) {
		



		if (optopt == (int)'-')
			return(EOF);
		if (!*place)
			++optind;
		if (opterr) {
			if (!(p = rindex(*nargv, '/')))
				p = *nargv;
			else
				++p;
			(void)fprintf(stderr, "%s: illegal option -- %c\n",
			    p, optopt);
		}
		return(BADCH);
	}
	if (*++oli != ':') {                    
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {                                  
		if (*place)                     
			optarg = place;
		else if (nargc <= ++optind) {   
			place = EMSG;
			if (!(p = rindex(*nargv, '/')))
				p = *nargv;
			else
				++p;
			if (opterr)
				(void)fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    p, optopt);
			return(BADCH);
		}
		else                            
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return(optopt);                         
}
