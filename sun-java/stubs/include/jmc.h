



































#ifndef JMC_H
#define JMC_H

#include "jritypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JMC_PUBLIC_API JRI_PUBLIC_API 

typedef struct JMCInterfaceID {
	jint a, b, c, d;
} JMCInterfaceID;

#ifdef __cplusplus
#define EXTERN_C		extern "C"
#define EXTERN_C_WITHOUT_EXTERN "C"
#else
#undef EXTERN_C
#define EXTERN_C
#define EXTERN_C_WITHOUT_EXTERN
#endif 

typedef struct JMCException JMCException;

JRI_PUBLIC_API(void)
JMCException_Destroy(struct JMCException *);

#define JMC_EXCEPTION(resultPtr, exceptionToReturn)		 \
	(((resultPtr) != NULL)					 \
	 ? ((*(resultPtr) = (exceptionToReturn), resultPtr))	 \
	 : (JMCException_Destroy(exceptionToReturn), resultPtr))

#define JMC_EXCEPTION_RETURNED(resultPtr)			 \
	((resultPtr) != NULL && *(resultPtr) != NULL)

#define JMCEXCEPTION_OUT_OF_MEMORY	((struct JMCException*)-1)

#define JMC_DELETE_EXCEPTION(resultPtr)				 \
	(JMCException_Destroy(*(resultPtr)), *(resultPtr) = NULL)

#ifdef __cplusplus
} 
#endif 

#endif
