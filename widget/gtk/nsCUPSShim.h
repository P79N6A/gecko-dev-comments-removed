





#ifndef nsCUPSShim_h___
#define nsCUPSShim_h___

#include "prtypes.h"





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

typedef cups_dest_t* (*CupsGetDestType)(const char *printer,
                                        const char *instance,
                                        int num_dests,
                                        cups_dest_t *dests);
typedef int (*CupsGetDestsType)(cups_dest_t **dests);
typedef int (*CupsFreeDestsType)(int         num_dests,
                                 cups_dest_t *dests);
typedef int (*CupsPrintFileType)(const char    *printer,
                                 const char    *filename,
                                 const char    *title,
                                 int           num_options,
                                 cups_option_t *options);
typedef int (*CupsTempFdType)(char *filename,
                              int   length);
typedef int (*CupsAddOptionType)(const char    *name,
                                 const char    *value,
                                 int           num_options,
                                 cups_option_t **options);

struct PRLibrary;


class nsCUPSShim {
    public:
        







        bool Init();

        



        bool IsInitialized() { return nullptr != mCupsLib; }

        


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
