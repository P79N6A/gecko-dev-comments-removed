





































#ifndef nsPrintJobFactoryGTK_h__
#define nsPrintJobFactoryGTK_h__

class nsIPrintJobGTK;
class nsDeviceContextSpecGTK;







class nsPrintJobFactoryGTK
{
public:
    










    static nsresult CreatePrintJob(nsDeviceContextSpecGTK *aSpec,
	    nsIPrintJobGTK* &aPrintJob);
};


#endif 
