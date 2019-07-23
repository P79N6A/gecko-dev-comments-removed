





































#ifndef nsPrintJobPS_h__
#define nsPrintJobPS_h__

#include "nsCUPSShim.h"
#include "nsDebug.h"
#include "nsIDeviceContext.h"   
#include "nsILocalFile.h"
#include "nsIPrintJobPS.h"
#include "nsString.h"
#include "nsTempfilePS.h"




class nsPrintJobPreviewPS : public nsIPrintJobPS {
    public:
        


        nsresult StartSubmission(FILE **aHandle)
            { return NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED; }

        nsresult FinishSubmission()
            { return NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED; }

        nsresult SetNumCopies(int aNumCopies)
            { return NS_ERROR_NOT_IMPLEMENTED; }

    protected:
        
        nsresult Init(nsIDeviceContextSpecPS *);
};



class nsPrintJobFilePS : public nsIPrintJobPS {
    public:
        nsPrintJobFilePS();
        ~nsPrintJobFilePS();

        
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();

        nsresult SetNumCopies(int aNumCopies)
            { return NS_ERROR_NOT_IMPLEMENTED; }

    protected:
        
        nsresult Init(nsIDeviceContextSpecPS *);

        



        void SetDestHandle(FILE *aHandle) { mDestHandle = aHandle; }

        



        FILE *GetDestHandle() { return mDestHandle; }

        





        void SetDestination(const char *aDest) { mDestination = aDest; }

        



        nsCString& GetDestination() { return mDestination; }


    private:
        FILE *mDestHandle;                  
        nsCString mDestination;
};

#ifdef VMS





class nsPrintJobVMSCmdPS : public nsPrintJobFilePS {
    public:
        
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();

    protected:
        nsresult Init(nsIDeviceContextSpecPS *);

    private:
        nsCString mPrinterName;
        nsTempfilePS mTempFactory;
        nsCOMPtr<nsILocalFile> mTempFile;
};

#else   





class nsPrintJobPipePS : public nsPrintJobFilePS {
    public:
        
        ~nsPrintJobPipePS();
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();

    protected:
        nsresult Init(nsIDeviceContextSpecPS *);

    private:
        nsCString mPrinterName;
};






class nsPrintJobCUPS : public nsPrintJobFilePS {
    public:
        nsresult StartSubmission(FILE **aHandle);
        nsresult FinishSubmission();
        nsresult SetNumCopies(int aNumCopies);
        void SetJobTitle(const PRUnichar *aTitle);

    protected:
        nsresult Init(nsIDeviceContextSpecPS *);

    private:
        nsCUPSShim mCups;
        nsCString mPrinterName;
        nsCString mNumCopies;
        nsCString mJobTitle;        
};
#endif  

#endif 
