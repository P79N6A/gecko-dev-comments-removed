





































#ifndef nsIPrintJobPS_h__
#define nsIPrintJobPS_h__

#include <stdio.h>

class nsIDeviceContextSpecPS;











class nsIPrintJobPS
{
public:
    


    virtual ~nsIPrintJobPS();

    
    friend class nsPrintJobFactoryPS;

    












    virtual nsresult SetNumCopies(int aNumCopies) = 0;

    









    virtual void SetJobTitle(const PRUnichar *aTitle) { }

    












    virtual nsresult StartSubmission(FILE **aHandle) = 0;

    











    virtual nsresult FinishSubmission() = 0;

protected:
    






    virtual nsresult Init(nsIDeviceContextSpecPS *aContext) = 0;
};


#endif 
