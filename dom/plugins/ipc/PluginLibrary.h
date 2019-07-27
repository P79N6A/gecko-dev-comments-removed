





#ifndef mozilla_PluginLibrary_h
#define mozilla_PluginLibrary_h 1

#include "prlink.h"
#include "npapi.h"
#include "npfunctions.h"
#include "nscore.h"
#include "nsTArray.h"
#include "nsError.h"
#include "mozilla/EventForwards.h"
#include "nsSize.h"
#include "nsRect.h"

class gfxContext;
class nsCString;
class nsNPAPIPlugin;

namespace mozilla {
namespace layers {
class Image;
class ImageContainer;
}
}


namespace mozilla {

class PluginLibrary
{
public:
  virtual ~PluginLibrary() { }

  



  virtual void SetPlugin(nsNPAPIPlugin* plugin) = 0;

  virtual bool HasRequiredFunctions() = 0;

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) = 0;
#else
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) = 0;
#endif
  virtual nsresult NP_Shutdown(NPError* error) = 0;
  virtual nsresult NP_GetMIMEDescription(const char** mimeDesc) = 0;
  virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                               void *aValue, NPError* error) = 0;
#if defined(XP_WIN) || defined(XP_MACOSX)
  virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) = 0;
#endif
  virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                           uint16_t mode, int16_t argc, char* argn[],
                           char* argv[], NPSavedData* saved,
                           NPError* error) = 0;

  virtual nsresult NPP_ClearSiteData(const char* site, uint64_t flags,
                                     uint64_t maxAge) = 0;
  virtual nsresult NPP_GetSitesWithData(InfallibleTArray<nsCString>& aResult) = 0;

  virtual nsresult AsyncSetWindow(NPP instance, NPWindow* window) = 0;
  virtual nsresult GetImageContainer(NPP instance, mozilla::layers::ImageContainer** aContainer) = 0;
  virtual nsresult GetImageSize(NPP instance, nsIntSize* aSize) = 0;
  virtual bool IsOOP() = 0;
#if defined(XP_MACOSX)
  virtual nsresult IsRemoteDrawingCoreAnimation(NPP instance, bool *aDrawing) = 0;
  virtual nsresult ContentsScaleFactorChanged(NPP instance, double aContentsScaleFactor) = 0;
#endif

  




  virtual nsresult SetBackgroundUnknown(NPP instance) = 0;
  virtual nsresult BeginUpdateBackground(NPP instance,
                                         const nsIntRect&, gfxContext**) = 0;
  virtual nsresult EndUpdateBackground(NPP instance,
                                       gfxContext*, const nsIntRect&) = 0;
  virtual nsresult GetRunID(uint32_t* aRunID) = 0;
};


} 

#endif  
