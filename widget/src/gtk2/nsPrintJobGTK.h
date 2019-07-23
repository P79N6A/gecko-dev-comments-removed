





































#ifndef nsPrintJobGTK_h__
#define nsPrintJobGTK_h__

#include "nsCUPSShim.h"
#include "nsDebug.h"
#include "nsIDeviceContext.h"   
#include "nsILocalFile.h"
#include "nsIPrintJobGTK.h"
#include "nsString.h"
#include "nsTempfilePS.h"
#include "nsDeviceContextSpecG.h"



class nsPrintJobPreviewGTK : public nsIPrintJobGTK {
    public:
        


        nsresult StartSubmission(FILE **aHandle)
            { return NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED; }

        nsresult FinishSubmission()
            { return NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED; }

        nsresult SetNumCopies(int aNumCopies)
            { return NS_ERROR_NOT_IMPLEMENTED; }

    protected:
        
        nsresult Init(nsDeviceContextSpecGTK *);
};



class nsPrintJobFileGTK : public nsIPrintJobGTK {
    public:
        nsPrintJobFileGTK();
        ~nsPrintJobFileGTK();

        
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();

        nsresult SetNumCopies(int aNumCopies)
            { return NS_ERROR_NOT_IMPLEMENTED; }

    protected:
        
        nsresult Init(nsDeviceContextSpecGTK *);

        



        void SetDestHandle(FILE *aHandle) { mDestHandle = aHandle; }

        



        FILE *GetDestHandle() { return mDestHandle; }

        





        void SetDestination(const char *aDest) { mDestination = aDest; }

        



        nsCString& GetDestination() { return mDestination; }


    private:
        FILE *mDestHandle;                  
        nsCString mDestination;
};





class nsPrintJobPipeGTK : public nsPrintJobFileGTK {
    public:
        
        ~nsPrintJobPipeGTK();
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();

    protected:
        nsresult Init(nsDeviceContextSpecGTK *);

    private:
        nsCString mPrinterName;
};






class nsPrintJobCUPS : public nsPrintJobFileGTK {
    public:
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();
        nsresult SetNumCopies(int aNumCopies);
        void SetJobTitle(const PRUnichar *aTitle);

    protected:
        nsresult Init(nsDeviceContextSpecGTK *);

    private:
        nsCUPSShim mCups;
        nsCString mPrinterName;
        nsCString mNumCopies;
        nsCString mJobTitle;        
};

#endif 
