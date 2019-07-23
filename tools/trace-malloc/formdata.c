













































#include "formdata.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static void unhexcape(char* inPlace)





{
    if(NULL != inPlace)
    {
        int index1 = 0;
        int index2 = 0;
        int theLen = strlen(inPlace);
        
        for(; index1 <= theLen; index1++)
        {
            if('%' == inPlace[index1] && '\0' != inPlace[index1 + 1] && '\0' != inPlace[index1 + 2])
            {
                int unhex = 0;
                
                if('9' >= inPlace[index1 + 1])
                {
                    unhex |= ((inPlace[index1 + 1] - '0') << 4);
                }
                else
                {
                    unhex |= ((toupper(inPlace[index1 + 1]) - 'A' + 10) << 4);
                }
                
                if('9' >= inPlace[index1 + 2])
                {
                    unhex |= (inPlace[index1 + 2] - '0');
                }
                else
                {
                    unhex |= (toupper(inPlace[index1 + 2]) - 'A' + 10);
                }
                
                index1 += 2;
                inPlace[index1] = unhex;
            }
            
            inPlace[index2++] = inPlace[index1];
        }
    }
}


FormData* FormData_Create(const char* inFormData)
{
    FormData* retval = NULL;

    if(NULL != inFormData)
    {
        FormData* container = NULL;

        


        container = (FormData*)calloc(1, sizeof(FormData));
        if(NULL != container)
        {
            


            container->mStorage = strdup(inFormData);
            if(NULL != container->mStorage)
            {
                char* traverse = NULL;
                unsigned nvpairs = 1;
                unsigned storeLen = 0;

                



                for(traverse = container->mStorage; '\0' != *traverse; traverse++)
                {
                    if('&' == *traverse)
                    {
                        nvpairs++;
                    }
                }
                storeLen = (unsigned)(traverse - container->mStorage);

                


                container->mNArray = (char**)calloc(nvpairs * 2, sizeof(char*));
                if(NULL != container->mNArray)
                {
                    char* amp = NULL;
                    char* equ = NULL;

                    container->mVArray = &container->mNArray[nvpairs];

                    





                    for(traverse = container->mStorage; NULL != traverse; container->mNVCount++)
                    {
                        container->mNArray[container->mNVCount] = traverse;

                        amp = strchr(traverse, '&');
                        equ = strchr(traverse, '=');
                        traverse = NULL;

                        if(NULL != equ && (NULL == amp || equ < amp))
                        {
                            *equ++ = '\0';

                            container->mVArray[container->mNVCount] = equ;
                        }
                        else
                        {
                            container->mVArray[container->mNVCount] = (container->mStorage + storeLen);
                        }

                        if(NULL != amp)
                        {
                            *amp++ = '\0';

                            traverse = amp;
                        }

                        unhexcape(container->mNArray[container->mNVCount]);
                        unhexcape(container->mVArray[container->mNVCount]);
                    }

                    retval = container;
                }
            }
        }

        


        if(NULL == retval)
        {
            FormData_Destroy(container);
        }
    }

    return retval;
}


void FormData_Destroy(FormData* inDestroy)
{
    if(NULL != inDestroy)
    {
        unsigned traverse = 0;

        for(traverse = 0; traverse < inDestroy->mNVCount; traverse++)
        {
            if(NULL != inDestroy->mNArray)
            {
                inDestroy->mNArray[traverse] = NULL;
            }
            if(NULL != inDestroy->mVArray)
            {
                inDestroy->mVArray[traverse] = NULL;
            }
        }
        inDestroy->mNVCount = 0;

        if(NULL != inDestroy->mStorage)
        {
            free(inDestroy->mStorage);
            inDestroy->mStorage = NULL;
        }

        if(NULL != inDestroy->mNArray)
        {
            free(inDestroy->mNArray);
            inDestroy->mNArray = NULL;
            inDestroy->mVArray = NULL;
        }

        free(inDestroy);
        inDestroy = NULL;
    }
}
