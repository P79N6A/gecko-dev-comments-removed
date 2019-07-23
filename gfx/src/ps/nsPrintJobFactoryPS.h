





































#ifndef nsPrintJobFactoryPS_h__
#define nsPrintJobFactoryPS_h__

#include "nscore.h"
#include "nsIPrintJobPS.h"







class nsPrintJobFactoryPS
{
public:
    










    static nsresult CreatePrintJob(nsIDeviceContextSpecPS *aSpec,
	    nsIPrintJobPS* &aPrintJob);
};


#endif 
