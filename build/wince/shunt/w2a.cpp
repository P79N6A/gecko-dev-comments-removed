







































#include "mozce_internal.h"

int w2a_buffer(const unsigned short* inWideString, int inWideChars, char* outACPString, int inACPChars)
{
    int retval = 0;

    


    if(NULL != outACPString && 0 != inACPChars)
    {
        *outACPString = '\0';
    }

    


    if(NULL != inWideString && 0 != inWideChars && (0 == inACPChars || NULL != outACPString))
    {
        


        retval = WideCharToMultiByte(
                                     CP_ACP,
                                     0,
                                     inWideString,
                                     inWideChars,
                                     outACPString,
                                     inACPChars,
                                     NULL,
                                     NULL
                                     );
    }

    return retval;
}


char* w2a_malloc(const unsigned short* inWideString, int inWideChars, int* outACPChars)
{
    LPSTR retval = NULL;

    


    if(NULL != outACPChars)
    {
        *outACPChars = 0;
    }

    



    if(-1 == inWideChars)
    {
        if(NULL != inWideString)
        {
            


            inWideChars = (int)wcslen(inWideString) + 1;
        }
        else
        {
            inWideChars = 0;
        }
    }

    


    if(NULL != inWideString && 0 != inWideChars)
    {
        int charsRequired = 0;

        


        charsRequired = w2a_buffer(inWideString, inWideChars, NULL, 0);
        if(0 != charsRequired)
        {
            LPSTR heapBuffer = NULL;

            heapBuffer = (LPSTR)malloc((size_t)charsRequired * sizeof(CHAR));
            if(NULL != heapBuffer)
            {
                int acpChars = 0;

                


                acpChars = w2a_buffer(inWideString, inWideChars, heapBuffer, charsRequired);
                if(0 != acpChars)
                {
                    retval = heapBuffer;
                    if(NULL != outACPChars)
                    {
                        *outACPChars = acpChars;
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
