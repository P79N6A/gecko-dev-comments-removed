





































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
#if defined(XP_WIN) || defined(XP_MACOSX)
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
#if defined(XP_WIN) || defined(XP_MACOSX)
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
        if (!mNP_GetMIMEDescription)
            return false;

        mNP_GetValue = (NP_GetValueFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetValue");
        if (!mNP_GetValue)
            return false;

#if defined(XP_WIN) || defined(XP_MACOSX)
        mNP_GetEntryPoints = (NP_GetEntryPointsFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetEntryPoints");
        if (!mNP_GetEntryPoints)
            return false;
#endif
        return true;
    }

#if defined(XP_UNIX) && !defined(XP_MACOSX)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) {
        if (mNP_Initialize) {
            *error = mNP_Initialize(bFuncs, pFuncs);
        } else {
            NP_InitializeFunc pfNP_Initialize = (NP_InitializeFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_Initialize");
            if (!pfNP_Initialize)
                return NS_ERROR_FAILURE;
            *error = pfNP_Initialize(bFuncs, pFuncs);
        }

        
        mNPP_New = pFuncs->newp;
        return NS_OK;
    }
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) {
        if (mNP_Initialize) {
            *error = mNP_Initialize(bFuncs);
        } else {
            NP_InitializeFunc pfNP_Initialize = (NP_InitializeFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_Initialize");
            if (!pfNP_Initialize)
                return NS_ERROR_FAILURE;
            *error = pfNP_Initialize(bFuncs);
        }

        return NS_OK;
    }
#endif

    virtual nsresult NP_Shutdown(NPError* error) {
        if (mNP_Shutdown) {
            *error = mNP_Shutdown();
        } else {
            NP_ShutdownFunc pfNP_Shutdown = (NP_ShutdownFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_Shutdown");
            if (!pfNP_Shutdown)
                return NS_ERROR_FAILURE;
            *error = pfNP_Shutdown();
        }

        return NS_OK;
    }

    virtual nsresult NP_GetMIMEDescription(char** mimeDesc) {
        if (mNP_GetMIMEDescription) {
            *mimeDesc = mNP_GetMIMEDescription();
        }
        else {
            NP_GetMIMEDescriptionFunc pfNP_GetMIMEDescription =
                (NP_GetMIMEDescriptionFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_GetMIMEDescription");
            if (!pfNP_GetMIMEDescription) {
                *mimeDesc = "";
                return NS_ERROR_FAILURE;
            }
            *mimeDesc = pfNP_GetMIMEDescription();
        }

        return NS_OK;
    }

    virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                                 void *aValue, NPError* error) {
        if (mNP_GetValue) {
            *error = mNP_GetValue(future, aVariable, aValue);
        } else {
            NP_GetValueFunc pfNP_GetValue = (NP_GetValueFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_GetValue");
            if (!pfNP_GetValue)
                return NS_ERROR_FAILURE;
            *error = pfNP_GetValue(future, aVariable, aValue);
        }

        return NS_OK;
    }

#if defined(XP_WIN) || defined(XP_MACOSX)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) {
        if (mNP_GetEntryPoints) {
            *error = mNP_GetEntryPoints(pFuncs);
        } else {
            NP_GetEntryPointsFunc pfNP_GetEntryPoints = (NP_GetEntryPointsFunc)
                PR_FindFunctionSymbol(mLibrary, "NP_GetEntryPoints");
            if (!pfNP_GetEntryPoints)
                return NS_ERROR_FAILURE;
            *error = pfNP_GetEntryPoints(pFuncs);
        }

        
        mNPP_New = pFuncs->newp;
        return NS_OK;
    }
#endif

    virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                             uint16_t mode, int16_t argc, char* argn[],
                             char* argv[], NPSavedData* saved,
                             NPError* error) {
        if (!mNPP_New)
            return NS_ERROR_FAILURE;
        *error = mNPP_New(pluginType, instance, mode, argc, argn, argv, saved);
        return NS_OK;
    }

private:
    NP_InitializeFunc mNP_Initialize;
    NP_ShutdownFunc mNP_Shutdown;
    NP_GetMIMEDescriptionFunc mNP_GetMIMEDescription;
    NP_GetValueFunc mNP_GetValue;
#if defined(XP_WIN) || defined(XP_MACOSX)
    NP_GetEntryPointsFunc mNP_GetEntryPoints;
#endif
    NPP_NewProcPtr mNPP_New;
    PRLibrary* mLibrary;
};


} 

#endif  
