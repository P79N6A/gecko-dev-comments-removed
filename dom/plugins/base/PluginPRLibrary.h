





#ifndef PluginPRLibrary_h
#define PluginPRLibrary_h 1

#include "mozilla/PluginLibrary.h"
#include "nsNPAPIPlugin.h"
#include "npfunctions.h"

namespace mozilla {

class PluginPRLibrary : public PluginLibrary
{
public:
    PluginPRLibrary(const char* aFilePath, PRLibrary* aLibrary) :
#if defined(XP_UNIX) && !defined(XP_MACOSX)
        mNP_Initialize(nullptr),
#else
        mNP_Initialize(nullptr),
#endif
        mNP_Shutdown(nullptr),
        mNP_GetMIMEDescription(nullptr),
#if defined(XP_UNIX) && !defined(XP_MACOSX)
        mNP_GetValue(nullptr),
#endif
#if defined(XP_WIN) || defined(XP_MACOSX)
        mNP_GetEntryPoints(nullptr),
#endif
        mNPP_New(nullptr),
        mNPP_ClearSiteData(nullptr),
        mNPP_GetSitesWithData(nullptr),
        mLibrary(aLibrary),
        mFilePath(aFilePath)
    {
        NS_ASSERTION(mLibrary, "need non-null lib");
        
    }

    virtual ~PluginPRLibrary()
    {
        
    }

    virtual void SetPlugin(nsNPAPIPlugin*) MOZ_OVERRIDE { }

    virtual bool HasRequiredFunctions() MOZ_OVERRIDE {
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

#if defined(XP_UNIX) && !defined(XP_MACOSX)
        mNP_GetValue = (NP_GetValueFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetValue");
        if (!mNP_GetValue)
            return false;
#endif

#if defined(XP_WIN) || defined(XP_MACOSX)
        mNP_GetEntryPoints = (NP_GetEntryPointsFunc)
            PR_FindFunctionSymbol(mLibrary, "NP_GetEntryPoints");
        if (!mNP_GetEntryPoints)
            return false;
#endif
        return true;
    }

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* aNetscapeFuncs,
                                   NPPluginFuncs* aFuncs, NPError* aError) MOZ_OVERRIDE;
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* aNetscapeFuncs,
                                   NPError* aError) MOZ_OVERRIDE;
#endif

    virtual nsresult NP_Shutdown(NPError* aError) MOZ_OVERRIDE;
    virtual nsresult NP_GetMIMEDescription(const char** aMimeDesc) MOZ_OVERRIDE;

    virtual nsresult NP_GetValue(void* aFuture, NPPVariable aVariable,
                                 void* aValue, NPError* aError) MOZ_OVERRIDE;

#if defined(XP_WIN) || defined(XP_MACOSX)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* aFuncs, NPError* aError) MOZ_OVERRIDE;
#endif

    virtual nsresult NPP_New(NPMIMEType aPluginType, NPP aInstance,
                             uint16_t aMode, int16_t aArgc, char* aArgn[],
                             char* aArgv[], NPSavedData* aSaved,
                             NPError* aError) MOZ_OVERRIDE;

    virtual nsresult NPP_ClearSiteData(const char* aSite, uint64_t aFlags,
                                       uint64_t aMaxAge) MOZ_OVERRIDE;
    virtual nsresult NPP_GetSitesWithData(InfallibleTArray<nsCString>& aResult) MOZ_OVERRIDE;

    virtual nsresult AsyncSetWindow(NPP aInstance, NPWindow* aWindow) MOZ_OVERRIDE;
    virtual nsresult GetImageContainer(NPP aInstance, mozilla::layers::ImageContainer** aContainer) MOZ_OVERRIDE;
    virtual nsresult GetImageSize(NPP aInstance, nsIntSize* aSize) MOZ_OVERRIDE;
    virtual bool IsOOP() MOZ_OVERRIDE { return false; }
#if defined(XP_MACOSX)
    virtual nsresult IsRemoteDrawingCoreAnimation(NPP aInstance, bool* aDrawing) MOZ_OVERRIDE;
    virtual nsresult ContentsScaleFactorChanged(NPP aInstance, double aContentsScaleFactor) MOZ_OVERRIDE;
#endif
    virtual nsresult SetBackgroundUnknown(NPP instance) MOZ_OVERRIDE;
    virtual nsresult BeginUpdateBackground(NPP instance,
                                           const nsIntRect&, gfxContext** aCtx) MOZ_OVERRIDE;
    virtual nsresult EndUpdateBackground(NPP instance,
                                         gfxContext* aCtx, const nsIntRect&) MOZ_OVERRIDE;
    virtual void GetLibraryPath(nsACString& aPath) { aPath.Assign(mFilePath); }

private:
    NP_InitializeFunc mNP_Initialize;
    NP_ShutdownFunc mNP_Shutdown;
    NP_GetMIMEDescriptionFunc mNP_GetMIMEDescription;
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    NP_GetValueFunc mNP_GetValue;
#endif
#if defined(XP_WIN) || defined(XP_MACOSX)
    NP_GetEntryPointsFunc mNP_GetEntryPoints;
#endif
    NPP_NewProcPtr mNPP_New;
    NPP_ClearSiteDataPtr mNPP_ClearSiteData;
    NPP_GetSitesWithDataPtr mNPP_GetSitesWithData;
    PRLibrary* mLibrary;
    nsCString mFilePath;
};


} 

#endif  
