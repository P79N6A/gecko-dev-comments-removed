




































#include "nspr.h"



















































#define SUPPORT_LSL
#define SUPPORT_DLS
#define SUPPORT_EPLF
#define SUPPORT_DOS
#define SUPPORT_VMS
#define SUPPORT_CMS
#define SUPPORT_OS2
#define SUPPORT_W16

struct list_state
{
  void           *magic;        
  PRTime         now_time;      
  PRExplodedTime now_tm;        
  PRInt32        lstyle;        
  PRInt32        parsed_one;    
  char           carry_buf[84]; 
  PRUint32       carry_buf_len; 
  PRUint32       numlines;      
};

struct list_result
{
  PRInt32           fe_type;      
  const char *      fe_fname;     
  PRUint32          fe_fnlen;     
  const char *      fe_lname;     
  PRUint32          fe_lnlen;     
  char              fe_size[40];  
  PRExplodedTime    fe_time;      
  PRInt32           fe_cinfs;     
                                  
};

int ParseFTPList(const char *line, 
                 struct list_state *state,
                 struct list_result *result );

