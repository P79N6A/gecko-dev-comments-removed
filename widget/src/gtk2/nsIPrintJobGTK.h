





































#ifndef nsIPrintJobGTK_h__
#define nsIPrintJobGTK_h__

#include <stdio.h>

class nsDeviceContextSpecGTK;











class nsIPrintJobGTK
{
public:
    


    virtual ~nsIPrintJobGTK();

    
    friend class nsPrintJobFactoryGTK;

    












    virtual nsresult SetNumCopies(int aNumCopies) = 0;

    









    virtual void SetJobTitle(const PRUnichar *aTitle) { }

    












    virtual nsresult StartSubmission(FILE **aHandle) = 0;

    











    virtual nsresult FinishSubmission() = 0;

protected:
    






    virtual nsresult Init(nsDeviceContextSpecGTK *aContext) = 0;
};


#endif 
