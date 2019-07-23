






































#include "nsEmbedAPI.h"

#ifndef WIN32
#error This file is for Win32!
#endif

#ifdef MOZ_SUPPORTS_EMBEDDING_EVENT_PROCESSING

NS_METHOD NS_HandleEmbeddingEvent(nsEmbedNativeEvent &aEvent, PRBool &aWasHandled)
{
    aWasHandled = PR_FALSE;
    return NS_OK;
}

#endif 
