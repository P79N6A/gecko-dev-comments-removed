





#ifndef nsPSPrinters_h___
#define nsPSPrinters_h___

#include "nsString.h"
#include "nsTArray.h"

class nsPSPrinterList {
    public:
        nsPSPrinterList();

        




        bool Enabled();

        











        void GetPrinterList(nsTArray<nsCString>& aList);

        enum PrinterType {
            kTypeUnknown,         
            kTypePS,              
            kTypeCUPS             
        };

        





        static PrinterType GetPrinterType(const nsACString& aName);
};

#endif 
