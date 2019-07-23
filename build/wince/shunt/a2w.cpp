







































#include "mozce_internal.h"

int a2w_buffer(const char* inACPString, int inACPChars, unsigned short* outWideString, int inWideChars)
{
    int retval = 0;
    
    


    if(NULL != outWideString && 0 != inWideChars)
    {
        *outWideString = L'\0';
    }
    
    


    if(NULL != inACPString && 0 != inACPChars && (0 == inWideChars || NULL != outWideString))
    {
        


        retval = MultiByteToWideChar(
                                     CP_ACP,
                                     0,
                                     inACPString,
                                     inACPChars,
                                     outWideString,
                                     inWideChars
                                     );
    }
    
    return retval;
}


unsigned short* a2w_malloc(const char* inACPString, int inACPChars, int* outWideChars)
{
    LPWSTR retval = NULL;
    
    


    if(NULL != outWideChars)
    {
        *outWideChars = 0;
    }
    
    



    if(-1 == inACPChars)
    {
        if(NULL != inACPString)
        {
            


            inACPChars = (int)strlen(inACPString) + 1;
        }
        else
        {
            inACPChars = 0;
        }
    }
    
    


    if(NULL != inACPString && 0 != inACPChars)
    {
        int charsRequired = 0;
        
        


        charsRequired = a2w_buffer(inACPString, inACPChars, NULL, 0);
        if(0 != charsRequired)
        {
            LPWSTR heapBuffer = NULL;
            
            heapBuffer = (LPWSTR)malloc((size_t)charsRequired * sizeof(WCHAR));
            if(NULL != heapBuffer)
            {
                int wideChars = 0;
                
                


                wideChars = a2w_buffer(inACPString, inACPChars, heapBuffer, charsRequired);
                if(0 != wideChars)
                {
                    retval = heapBuffer;
                    if(NULL != outWideChars)
                    {
                        *outWideChars = wideChars;
                    }
                }
                else
                {
                    



                    free(heapBuffer);
                }
            }
        }
    }
    
    return retval;
}
