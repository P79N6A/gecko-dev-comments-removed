





































#ifndef nsPSPrinters_h___
#define nsPSPrinters_h___

#include "nsString.h"
#include "nsTArray.h"
#include "prtypes.h"

class nsIPrefService;
class nsIPrefBranch;
class nsCUPSShim;

class nsPSPrinterList {
    public:
        




        nsresult Init();

        




        PRBool Enabled();

        











        void GetPrinterList(nsTArray<nsCString>& aList);

        enum PrinterType {
            kTypeUnknown,         
            kTypePS,              
            kTypeCUPS             
        };

        





        static PrinterType GetPrinterType(const nsACString& aName);

    private:
        nsCOMPtr<nsIPrefService> mPrefSvc;
        nsCOMPtr<nsIPrefBranch> mPref;
};

#endif 
