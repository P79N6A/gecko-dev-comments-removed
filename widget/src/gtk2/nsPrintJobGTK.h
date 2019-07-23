





































#ifndef nsPrintJobGTK_h__
#define nsPrintJobGTK_h__

#include "nsCUPSShim.h"
#include "nsDebug.h"
#include "nsIDeviceContext.h"   
#include "nsILocalFile.h"
#include "nsIPrintJobGTK.h"
#include "nsString.h"
#include "nsDeviceContextSpecG.h"



class nsPrintJobPreviewGTK : public nsIPrintJobGTK {
    public:
        


        virtual nsresult Submit()
            { return NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED; }

    protected:
        virtual nsresult Init(nsDeviceContextSpecGTK *);
        nsresult InitSpoolFile(PRUint32 aPermissions);
};



class nsPrintJobFileGTK : public nsPrintJobPreviewGTK {
    public:
        virtual nsresult Submit();

    protected:
        virtual nsresult Init(nsDeviceContextSpecGTK *);
        nsCOMPtr<nsILocalFile> mDestFile;
};


class nsPrintJobPipeGTK : public nsPrintJobPreviewGTK {
    public:
        virtual nsresult Submit();

    protected:
        virtual nsresult Init(nsDeviceContextSpecGTK *);

    private:
        nsCString mCommand;
        nsCString mPrinterName;
};



class nsPrintJobCUPS : public nsIPrintJobGTK {
    public:
        virtual nsresult Submit();
        virtual nsresult SetNumCopies(int aNumCopies);
        virtual void SetJobTitle(const PRUnichar *aTitle);

    protected:
        virtual nsresult Init(nsDeviceContextSpecGTK *);

    private:
        nsCUPSShim mCups;
        nsCString mPrinterName;
        nsCString mNumCopies;
        nsCString mJobTitle;        
        nsCString mSpoolName;
};

#endif 
