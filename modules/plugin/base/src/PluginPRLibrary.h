





































#ifndef PluginPRLibrary_h
#define PluginPRLibrary_h 1

#include "mozilla/PluginLibrary.h"
#include "nsNPAPIPlugin.h"

namespace mozilla {

class PluginPRLibrary : public PluginLibrary
{
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    typedef NPError (*NP_InitializeFunc)(NPNetscapeFuncs*, NPPluginFuncs*);
#else
    typedef NPError (OSCALL *NP_InitializeFunc)(NPNetscapeFuncs*);
#endif
    typedef NPError (OSCALL *NP_ShutdownFunc)();
    typedef char* (*NP_GetMIMEDescriptionFunc)();
    typedef NPError (*NP_GetValueFunc)(void *, NPPVariable, void*);
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
    typedef NPError (OSCALL *NP_GetEntryPointsFunc)(NPPluginFuncs*);
#endif

public:
    PluginPRLibrary(const char* aFilePath, PRLibrary* aLibrary) :
#if defined(XP_UNIX) && !defined(XP_MACOSX)
        mNP_Initialize(nsnull),
#else
        mNP_Initialize(nsnull),
#endif
        mNP_Shutdown(nsnull),
        mNP_GetMIMEDescription(nsnull),
        mNP_GetValue(nsnull),
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
        mNP_GetEntryPoints(nsnull),
#endif
        mNPP_New(nsnull),
        mLibrary(aLibrary)
    {
        NS_ASSERTION(mLibrary, "need non-null lib");
        
    }

    virtual ~PluginPRLibrary()
    {
        
    }

    virtual void SetPlugin(nsNPAPIPlugin*) { }

    virtual bool HasRequiredFunctions() {
        mNP_Initialize = (NP_InitializeFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_Initialize");
        if (!mNP_Initialize)
            return false;

        mNP_Shutdown = (NP_ShutdownFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_Shutdown");
        if (!mNP_Shutdown)
            return false;

        mNP_GetMIMEDescription = (NP_GetMIMEDescriptionFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetMIMEDescription");
#ifndef XP_MACOSX
        if (!mNP_GetMIMEDescription)
            return false;
#endif

        mNP_GetValue = (NP_GetValueFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetValue");
#ifndef XP_MACOSX
        if (!mNP_GetValue)
            return false;
#endif

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
        mNP_GetEntryPoints = (NP_GetEntryPointsFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetEntryPoints");
        if (!mNP_GetEntryPoints)
            return false;
#endif
        return true;
    }

#if defined(XP_UNIX) && !defined(XP_MACOSX)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs,
                                   NPPluginFuncs* pFuncs, NPError* error);
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs,
                                   NPError* error);
#endif

    virtual nsresult NP_Shutdown(NPError* error);
    virtual nsresult NP_GetMIMEDescription(const char** mimeDesc);

    virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                                 void *aValue, NPError* error);

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error);
#endif

    virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                             uint16_t mode, int16_t argc, char* argn[],
                             char* argv[], NPSavedData* saved,
                             NPError* error);

private:
    NP_InitializeFunc mNP_Initialize;
    NP_ShutdownFunc mNP_Shutdown;
    NP_GetMIMEDescriptionFunc mNP_GetMIMEDescription;
    NP_GetValueFunc mNP_GetValue;
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
    NP_GetEntryPointsFunc mNP_GetEntryPoints;
#endif
    NPP_NewProcPtr mNPP_New;
    PRLibrary* mLibrary;
};


} 

#endif  
