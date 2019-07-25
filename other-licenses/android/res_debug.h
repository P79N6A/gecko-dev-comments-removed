




























#ifndef _RES_DEBUG_H_
#define _RES_DEBUG_H_

#ifndef DEBUG
#   define Dprint(cond, args)
#   define DprintQ(cond, args, query, size)
#   define Aerror(statp, file, string, error, address)
#   define Perror(statp, file, string, error)
#else
#   define Dprint(cond, args) if (cond) {fprintf args;} else {}
#   define DprintQ(cond, args, query, size) if (cond) {\
			fprintf args;\
			res_pquery(statp, query, size, stdout);\
		} else {}
#endif

#endif  
