







































#ifndef nsPrintdGTK_h___
#define nsPrintdGTK_h___

#include <limits.h>

PR_BEGIN_EXTERN_C




#ifndef NS_LEGAL_SIZE
#define NS_LETTER_SIZE    0
#define NS_LEGAL_SIZE     1
#define NS_EXECUTIVE_SIZE 2
#define NS_A4_SIZE        3
#define NS_A3_SIZE        4

#define NS_PORTRAIT  0
#define NS_LANDSCAPE 1
#endif

#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX	_POSIX_PATH_MAX
#else
#define PATH_MAX	256
#endif
#endif

typedef struct unixprdata {
        PRBool toPrinter;          
        PRBool fpf;                
        PRBool grayscale;          
        int size;                   
        int orientation;            
        char command[ PATH_MAX ];   
        char path[ PATH_MAX ];      
        char printer[256];          
        int  copies;                
	PRBool cancel;		    
	float left;		    
	float right;		    
	float top;		    
	float bottom;		    
} UnixPrData;

void UnixPrDialog(UnixPrData *prData);

PR_END_EXTERN_C

#endif 

