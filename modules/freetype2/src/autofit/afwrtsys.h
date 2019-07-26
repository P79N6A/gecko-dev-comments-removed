

















#ifndef __AFWRTSYS_H__
#define __AFWRTSYS_H__




#include "afdummy.h"
#include "aflatin.h"
#include "afcjk.h"
#include "afindic.h"
#ifdef FT_OPTION_AUTOFIT2
#include "aflatin2.h"
#endif

#endif 


  
  


  
  

  WRITING_SYSTEM( dummy,  DUMMY  )
  WRITING_SYSTEM( latin,  LATIN  )
  WRITING_SYSTEM( cjk,    CJK    )
  WRITING_SYSTEM( indic,  INDIC  )
#ifdef FT_OPTION_AUTOFIT2
  WRITING_SYSTEM( latin2, LATIN2 )
#endif



