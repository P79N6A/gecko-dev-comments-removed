





































#ifndef nsPrintJobFactoryGTK_h__
#define nsPrintJobFactoryGTK_h__

#include "nscore.h"
#include "nsIPrintJobGTK.h"
#include "nsDeviceContextSpecG.h"







class nsPrintJobFactoryGTK
{
public:
    










    static nsresult CreatePrintJob(nsDeviceContextSpecGTK *aSpec,
	    nsIPrintJobGTK* &aPrintJob);
};


#endif 
