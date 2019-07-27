




































































#ifndef _LIBUTIL_PROFILE_H_
#define _LIBUTIL_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} 
#endif
  







#include <stdio.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>








typedef struct {
	char *name;		


	int32 count;		
} pctr_t;








 
SPHINXBASE_EXPORT
pctr_t* pctr_new (
	char *name   
	);



 

SPHINXBASE_EXPORT
void pctr_reset (pctr_t *ctr  
	);



 
SPHINXBASE_EXPORT
void pctr_print(FILE *fp,      
		pctr_t *ctr   
	);



 
SPHINXBASE_EXPORT
void pctr_increment (pctr_t *ctr, 
		     int32 inc   
	);




SPHINXBASE_EXPORT
void pctr_free(pctr_t* ctr  
	);







typedef struct {
	const char *name;		

	float64 t_cpu;		
	float64 t_elapsed;		
	float64 t_tot_cpu;		
	float64 t_tot_elapsed;	
	float64 start_cpu;		
	float64 start_elapsed;	
} ptmr_t;




SPHINXBASE_EXPORT
void ptmr_start (ptmr_t *tmr 
	);


SPHINXBASE_EXPORT
void ptmr_stop (ptmr_t *tmr  
	);


SPHINXBASE_EXPORT
void ptmr_reset (ptmr_t *tmr  
	);



SPHINXBASE_EXPORT
void ptmr_init (ptmr_t *tmr 
	);






SPHINXBASE_EXPORT
void ptmr_reset_all (ptmr_t *tmr 
	);





SPHINXBASE_EXPORT
void ptmr_print_all (FILE *fp,    
		     ptmr_t *tmr, 
		     float64 norm
	);






SPHINXBASE_EXPORT
int32 host_pclk (int32 dummy);







SPHINXBASE_EXPORT
int32 host_endian ( void );

#ifdef __cplusplus
}
#endif

#endif
