





































#ifndef nsPSPrinters_h___
#define nsPSPrinters_h___

#include "nsString.h"
#include "nsVoidArray.h"
#include "prtypes.h"
#include "nsCUPSShim.h"
#include "psSharedCore.h"

class nsIPrefService;
class nsIPrefBranch;
class nsCUPSShim;

class NS_PSSHARED nsPSPrinterList {
    public:
        




        nsresult Init();

        




        PRBool Enabled();

        











        void GetPrinterList(nsCStringArray& aList);

        enum PrinterType {
            kTypeUnknown,         
            kTypePS,              
            kTypeCUPS             
        };

        





        static PrinterType GetPrinterType(const nsACString& aName);

    private:
        nsCOMPtr<nsIPrefService> mPrefSvc;
        nsCOMPtr<nsIPrefBranch> mPref;
        nsCUPSShim mCups;
};

#endif 
