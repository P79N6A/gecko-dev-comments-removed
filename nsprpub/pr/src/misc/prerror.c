




































#include "primpl.h"

#include <string.h>
#include <stdlib.h>

PR_IMPLEMENT(PRErrorCode) PR_GetError(void)
{
    PRThread *thread = PR_GetCurrentThread();
    return thread->errorCode;
}

PR_IMPLEMENT(PRInt32) PR_GetOSError(void)
{
    PRThread *thread = PR_GetCurrentThread();
    return thread->osErrorCode;
}

PR_IMPLEMENT(void) PR_SetError(PRErrorCode code, PRInt32 osErr)
{
    PRThread *thread = PR_GetCurrentThread();
    thread->errorCode = code;
    thread->osErrorCode = osErr;
    thread->errorStringLength = 0;
}

PR_IMPLEMENT(void) PR_SetErrorText(PRIntn textLength, const char *text)
{
    PRThread *thread = PR_GetCurrentThread();

    if (0 == textLength)
    {
	    if (NULL != thread->errorString)
	        PR_DELETE(thread->errorString);
	    thread->errorStringSize = 0;
    }
    else
    {
	    PRIntn size = textLength + 31;  
        if (thread->errorStringSize < textLength+1)  
        {
	        if (NULL != thread->errorString)
	            PR_DELETE(thread->errorString);
		    thread->errorString = (char*)PR_MALLOC(size);
            if ( NULL == thread->errorString ) {
                thread->errorStringSize = 0;
                thread->errorStringLength = 0;
                return;
            }
            thread->errorStringSize = size;
	    }
        memcpy(thread->errorString, text, textLength+1 );
    }
    thread->errorStringLength = textLength;
}

PR_IMPLEMENT(PRInt32) PR_GetErrorTextLength(void)
{
    PRThread *thread = PR_GetCurrentThread();
    return thread->errorStringLength;
}  

PR_IMPLEMENT(PRInt32) PR_GetErrorText(char *text)
{
    PRThread *thread = PR_GetCurrentThread();
    if (0 != thread->errorStringLength)
        memcpy(text, thread->errorString, thread->errorStringLength+1);
    return thread->errorStringLength;
}  


