





































#ifndef nsIPrintJobGTK_h__
#define nsIPrintJobGTK_h__

#include "nsCOMPtr.h"

class nsDeviceContextSpecGTK;
class nsILocalFile;











class nsIPrintJobGTK
{
public:
    virtual ~nsIPrintJobGTK();

    
    friend class nsPrintJobFactoryGTK;

    












    virtual nsresult SetNumCopies(int aNumCopies)
            { return NS_ERROR_NOT_IMPLEMENTED; }

    









    virtual void SetJobTitle(const PRUnichar *aTitle) { }

    












    nsresult GetSpoolFile(nsILocalFile **aFile); 

    














    virtual nsresult Submit() = 0;

protected:
    








    virtual nsresult Init(nsDeviceContextSpecGTK *aContext) = 0;
    
    
    nsCOMPtr<nsILocalFile> mSpoolFile;
};


#endif 
