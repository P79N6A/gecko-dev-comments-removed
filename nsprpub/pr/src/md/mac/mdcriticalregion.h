






































#ifndef mdcriticalregion_h___
#define mdcriticalregion_h___


#ifndef __MULTIPROCESSING__
#include <Multiprocessing.h>
#endif

typedef struct OpaqueMDCriticalRegionID*  MDCriticalRegionID;

OSStatus MD_CriticalRegionCreate(MDCriticalRegionID * pMDCriticalRegionID);

OSStatus MD_CriticalRegionDelete(MDCriticalRegionID pMDCriticalRegionID);

OSStatus MD_CriticalRegionEnter(MDCriticalRegionID pMDCriticalRegionID, Duration pTimeout);

OSStatus MD_CriticalRegionExit(MDCriticalRegionID pMDCriticalRegionID);

#endif 

