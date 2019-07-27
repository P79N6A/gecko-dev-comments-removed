
























































#ifndef _LIBUTIL_CMD_LN_H_
#define _LIBUTIL_CMD_LN_H_

#include <stdio.h>
#include <stdarg.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>







  

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif





typedef struct arg_s {
	char const *name;   
	int type;           
	char const *deflt;  
	char const *doc;    
} arg_t;








#define ARG_REQUIRED (1<<0)



#define ARG_INTEGER  (1<<1)



#define ARG_FLOATING (1<<2)



#define ARG_STRING   (1<<3)



#define ARG_BOOLEAN  (1<<4)



#define ARG_STRING_LIST  (1<<5)




#define REQARG_INTEGER (ARG_INTEGER | ARG_REQUIRED)



#define REQARG_FLOATING (ARG_FLOATING | ARG_REQUIRED)



#define REQARG_STRING (ARG_STRING | ARG_REQUIRED)



#define REQARG_BOOLEAN (ARG_BOOLEAN | ARG_REQUIRED)




#define ARG_INT32   ARG_INTEGER



#define ARG_FLOAT32 ARG_FLOATING



#define ARG_FLOAT64 ARG_FLOATING



#define REQARG_INT32 (ARG_INT32 | ARG_REQUIRED)



#define REQARG_FLOAT32 (ARG_FLOAT32 | ARG_REQUIRED)



#define REQARG_FLOAT64 (ARG_FLOAT64 | ARG_REQUIRED)







#define ARG_STRINGIFY(s) ARG_STRINGIFY1(s)
#define ARG_STRINGIFY1(s) #s





typedef struct cmd_ln_s cmd_ln_t;



















SPHINXBASE_EXPORT
cmd_ln_t *cmd_ln_init(cmd_ln_t *inout_cmdln, arg_t const *defn, int32 strict, ...);






SPHINXBASE_EXPORT
cmd_ln_t *cmd_ln_retain(cmd_ln_t *cmdln);






SPHINXBASE_EXPORT
int cmd_ln_free_r(cmd_ln_t *cmdln);




















SPHINXBASE_EXPORT
cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, 

                         arg_t const *defn,	
                         int32 argc,		
                         char *argv[],		
                         int32 strict           

    );







SPHINXBASE_EXPORT
cmd_ln_t *cmd_ln_parse_file_r(cmd_ln_t *inout_cmdln, 

                              arg_t const *defn,   
                              char const *filename,
 
                              int32 strict         

    );




SPHINXBASE_EXPORT
anytype_t *cmd_ln_access_r(cmd_ln_t *cmdln, char const *name);















SPHINXBASE_EXPORT
char const *cmd_ln_str_r(cmd_ln_t *cmdln, char const *name);















SPHINXBASE_EXPORT
char const **cmd_ln_str_list_r(cmd_ln_t *cmdln, char const *name);












SPHINXBASE_EXPORT
long cmd_ln_int_r(cmd_ln_t *cmdln, char const *name);












SPHINXBASE_EXPORT
double cmd_ln_float_r(cmd_ln_t *cmdln, char const *name);




#define cmd_ln_boolean_r(c,n) (cmd_ln_int_r(c,n) != 0)









SPHINXBASE_EXPORT
void cmd_ln_set_str_r(cmd_ln_t *cmdln, char const *name, char const *str);








SPHINXBASE_EXPORT
void cmd_ln_set_int_r(cmd_ln_t *cmdln, char const *name, long iv);








SPHINXBASE_EXPORT
void cmd_ln_set_float_r(cmd_ln_t *cmdln, char const *name, double fv);




#define cmd_ln_set_boolean_r(c,n,b) (cmd_ln_set_int_r(c,n,(b)!=0))




#define cmd_ln_int32_r(c,n)	(int32)cmd_ln_int_r(c,n)
#define cmd_ln_float32_r(c,n)	(float32)cmd_ln_float_r(c,n)
#define cmd_ln_float64_r(c,n)	(float64)cmd_ln_float_r(c,n)
#define cmd_ln_set_int32_r(c,n,i)   cmd_ln_set_int_r(c,n,i)
#define cmd_ln_set_float32_r(c,n,f) cmd_ln_set_float_r(c,n,(double)f)
#define cmd_ln_set_float64_r(c,n,f) cmd_ln_set_float_r(c,n,(double)f)







SPHINXBASE_EXPORT
int cmd_ln_exists_r(cmd_ln_t *cmdln, char const *name);








SPHINXBASE_EXPORT
void cmd_ln_print_help_r (cmd_ln_t *cmdln, FILE *fp, const arg_t *defn);








SPHINXBASE_EXPORT
int32 cmd_ln_parse(const arg_t *defn,  
                   int32 argc,	       
                   char *argv[],       
                   int32 strict        

	);










SPHINXBASE_EXPORT
int32 cmd_ln_parse_file(const arg_t *defn,   
			char const *filename, 
                        int32 strict         

	);






SPHINXBASE_EXPORT
void cmd_ln_appl_enter(int argc,   
		       char *argv[], 
		       char const* default_argfn, 
		       const arg_t *defn 
	);








SPHINXBASE_EXPORT
void cmd_ln_appl_exit(void);







SPHINXBASE_EXPORT
cmd_ln_t *cmd_ln_get(void);











#define cmd_ln_exists(name)	cmd_ln_exists_r(cmd_ln_get(), name)







#define cmd_ln_access(name)	cmd_ln_access_r(cmd_ln_get(), name)







#define cmd_ln_str(name)	cmd_ln_str_r(cmd_ln_get(), name)







#define cmd_ln_str_list(name)	cmd_ln_str_list_r(cmd_ln_get(), name)







#define cmd_ln_int32(name)	(int32)cmd_ln_int_r(cmd_ln_get(), name)






#define cmd_ln_float32(name)	(float32)cmd_ln_float_r(cmd_ln_get(), name)






#define cmd_ln_float64(name)	(float64)cmd_ln_float_r(cmd_ln_get(), name)






#define cmd_ln_boolean(name)	cmd_ln_boolean_r(cmd_ln_get(), name)







#define cmd_ln_set_str(n,s)     cmd_ln_set_str_r(cmd_ln_get(),n,s)






#define cmd_ln_set_int32(n,i)   cmd_ln_set_int_r(cmd_ln_get(),n,i)






#define cmd_ln_set_float32(n,f) cmd_ln_set_float_r(cmd_ln_get(),n,f)






#define cmd_ln_set_float64(n,f) cmd_ln_set_float_r(cmd_ln_get(),n,f)






#define cmd_ln_set_boolean(n,b) cmd_ln_set_boolean_r(cmd_ln_get(),n,b)








#define cmd_ln_print_help(f,d) cmd_ln_print_help_r(cmd_ln_get(),f,d)





SPHINXBASE_EXPORT
void cmd_ln_free (void);


#ifdef __cplusplus
}
#endif

#endif


