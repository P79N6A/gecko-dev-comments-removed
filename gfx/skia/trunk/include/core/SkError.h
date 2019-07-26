






#ifndef SkError_DEFINED
#define SkError_DEFINED





enum SkError {
    

    kNoError_SkError=0,

    



    kInvalidArgument_SkError,

    




    kInvalidOperation_SkError,

    



    kInvalidHandle_SkError,

    


    kInvalidPaint_SkError,

    

    kOutOfMemory_SkError,

    

    kParseError_SkError,

    


    kInternalError_SkError
};




SkError SkGetLastError();



void SkClearLastError();





typedef void (*SkErrorCallbackFunction)(SkError, void *);










void SkSetErrorCallback(SkErrorCallbackFunction cb, void *context);






const char *SkGetLastErrorString();

#endif 
