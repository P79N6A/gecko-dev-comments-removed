





































#ifndef nsCUPSShim_h___
#define nsCUPSShim_h___

#include "prtypes.h"
#include "psSharedCore.h"






typedef struct                          
{
    char          *name;                  
    char          *value;                 
} cups_option_t;

typedef struct               
{
    char          *name,       
                  *instance;   
    int           is_default;  
    int           num_options; 
    cups_option_t *options;    
} cups_dest_t;

typedef cups_dest_t* (PR_CALLBACK *CupsGetDestType)(const char *printer,
                                                    const char *instance,
                                                    int num_dests, 
                                                    cups_dest_t *dests);
typedef int (PR_CALLBACK *CupsGetDestsType)(cups_dest_t **dests);
typedef int (PR_CALLBACK *CupsFreeDestsType)(int         num_dests,
                                             cups_dest_t *dests);
typedef int (PR_CALLBACK *CupsPrintFileType)(const char    *printer,
                                             const char    *filename,
                                             const char    *title,
                                             int           num_options,
                                             cups_option_t *options);
typedef int (PR_CALLBACK *CupsTempFdType)(char *filename,
                                          int   length);
typedef int (PR_CALLBACK *CupsAddOptionType)(const char    *name,
                                             const char    *value,
                                             int           num_options,
                                             cups_option_t **options);

struct PRLibrary;

class NS_PSSHARED nsCUPSShim {
    public:
        nsCUPSShim() : mCupsLib(nsnull) { }
        ~nsCUPSShim();

        







        PRBool Init();

        



        PRBool IsInitialized() { return nsnull != mCupsLib; }

        


        CupsAddOptionType   mCupsAddOption;
        CupsFreeDestsType   mCupsFreeDests;
        CupsGetDestType     mCupsGetDest;
        CupsGetDestsType    mCupsGetDests;
        CupsPrintFileType   mCupsPrintFile;
        CupsTempFdType      mCupsTempFd;

    private:
        PRLibrary *mCupsLib;
};



#endif 
