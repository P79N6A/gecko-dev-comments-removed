



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
  int32_t        lstyle;        
  int32_t        parsed_one;    
  char           carry_buf[84]; 
  uint32_t       carry_buf_len; 
  uint32_t       numlines;      
};

struct list_result
{
  int32_t           fe_type;      
  const char *      fe_fname;     
  uint32_t          fe_fnlen;     
  const char *      fe_lname;     
  uint32_t          fe_lnlen;     
  char              fe_size[40];  
  PRExplodedTime    fe_time;      
  int32_t           fe_cinfs;     
                                  
};

int ParseFTPList(const char *line, 
                 struct list_state *state,
                 struct list_result *result );

