







































#ifndef NSEMBEDAPI_H
#define NSEMBEDAPI_H

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"








































extern "C" NS_HIDDEN NS_METHOD
NS_InitEmbedding(nsILocalFile *aMozBinDirectory,
                 nsIDirectoryServiceProvider *aAppFileLocProvider,
                 nsStaticModuleInfo const *aStaticComponents = nsnull,
                 PRUint32 aStaticComponentCount = 0);














extern "C" NS_HIDDEN NS_METHOD
NS_TermEmbedding();






#undef MOZ_SUPPORTS_EMBEDDING_EVENT_PROCESSING


#if defined (WIN32) || defined (WINCE)
#include "windows.h"





typedef MSG nsEmbedNativeEvent;
#define MOZ_SUPPORTS_EMBEDDING_EVENT_PROCESSING
#endif


#ifdef XP_OS2
#include "os2.h"






typedef QMSG nsEmbedNativeEvent;
#define MOZ_SUPPORTS_EMBEDDING_EVENT_PROCESSING
#endif








#ifdef MOZ_SUPPORTS_EMBEDDING_EVENT_PROCESSING
















extern "C" NS_HIDDEN NS_METHOD
NS_HandleEmbeddingEvent(nsEmbedNativeEvent &aEvent, PRBool &aWasHandled);

#endif 

#endif 

