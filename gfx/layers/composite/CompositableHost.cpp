




#include "CompositableHost.h"
#include <map>                          
#include <utility>                      
#include "ContentHost.h"                
#include "Effects.h"                    
#include "gfxUtils.h"
#include "ImageHost.h"                  
#include "TiledContentHost.h"           
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureHost.h"  
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "gfxPlatform.h"                
#include "mozilla/layers/PCompositableParent.h"

namespace mozilla {
namespace layers {

class Compositor;









class CompositableParent : public PCompositableParent
{
public:
  CompositableParent(CompositableParentManager* aMgr,
                     const TextureInfo& aTextureInfo,
                     uint64_t aID = 0)
  {
    MOZ_COUNT_CTOR(CompositableParent);
    mHost = CompositableHost::Create(aTextureInfo);
    mHost->SetAsyncID(aID);
    if (aID) {
      CompositableMap::Set(aID, this);
    }
  }

  ~CompositableParent()
  {
    MOZ_COUNT_DTOR(CompositableParent);
    CompositableMap::Erase(mHost->GetAsyncID());
  }

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
  {
    if (mHost) {
      mHost->Detach(nullptr, CompositableHost::FORCE_DETACH);
    }
  }

  RefPtr<CompositableHost> mHost;
};

CompositableHost::CompositableHost(const TextureInfo& aTextureInfo)
  : mTextureInfo(aTextureInfo)
  , mAsyncID(0)
  , mCompositorID(0)
  , mCompositor(nullptr)
  , mLayer(nullptr)
  , mFlashCounter(0)
  , mAttached(false)
  , mKeepAttached(false)
{
  MOZ_COUNT_CTOR(CompositableHost);
}

CompositableHost::~CompositableHost()
{
  MOZ_COUNT_DTOR(CompositableHost);
}

PCompositableParent*
CompositableHost::CreateIPDLActor(CompositableParentManager* aMgr,
                                  const TextureInfo& aTextureInfo,
                                  uint64_t aID)
{
  return new CompositableParent(aMgr, aTextureInfo, aID);
}

bool
CompositableHost::DestroyIPDLActor(PCompositableParent* aActor)
{
  delete aActor;
  return true;
}

CompositableHost*
CompositableHost::FromIPDLActor(PCompositableParent* aActor)
{
  MOZ_ASSERT(aActor);
  return static_cast<CompositableParent*>(aActor)->mHost;
}

void
CompositableHost::UseTextureHost(TextureHost* aTexture)
{
  if (!aTexture) {
    return;
  }
  aTexture->SetCompositor(GetCompositor());
}

void
CompositableHost::UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                            TextureHost* aTextureOnWhite)
{
  MOZ_ASSERT(aTextureOnBlack && aTextureOnWhite);
  aTextureOnBlack->SetCompositor(GetCompositor());
  aTextureOnWhite->SetCompositor(GetCompositor());
}

void
CompositableHost::RemoveTextureHost(TextureHost* aTexture)
{}

void
CompositableHost::SetCompositor(Compositor* aCompositor)
{
  mCompositor = aCompositor;
}

bool
CompositableHost::AddMaskEffect(EffectChain& aEffects,
                                const gfx::Matrix4x4& aTransform,
                                bool aIs3D)
{
  CompositableTextureSourceRef source;
  RefPtr<TextureHost> host = GetAsTextureHost();

  if (!host) {
    NS_WARNING("Using compositable with no valid TextureHost as mask");
    return false;
  }

  if (!host->Lock()) {
    NS_WARNING("Failed to lock the mask texture");
    return false;
  }

  if (!host->BindTextureSource(source)) {
    NS_WARNING("The TextureHost was successfully locked but can't provide a TextureSource");
    host->Unlock();
    return false;
  }
  MOZ_ASSERT(source);

  RefPtr<EffectMask> effect = new EffectMask(source,
                                             source->GetSize(),
                                             aTransform);
  effect->mIs3D = aIs3D;
  aEffects.mSecondaryEffects[EffectTypes::MASK] = effect;
  return true;
}

void
CompositableHost::RemoveMaskEffect()
{
  RefPtr<TextureHost> host = GetAsTextureHost();
  if (host) {
    host->Unlock();
  }
}

 TemporaryRef<CompositableHost>
CompositableHost::Create(const TextureInfo& aTextureInfo)
{
  RefPtr<CompositableHost> result;
  switch (aTextureInfo.mCompositableType) {
  case CompositableType::IMAGE_BRIDGE:
    NS_ERROR("Cannot create an image bridge compositable this way");
    break;
  case CompositableType::CONTENT_INC:
    result = new ContentHostIncremental(aTextureInfo);
    break;
  case CompositableType::CONTENT_TILED:
    result = new TiledContentHost(aTextureInfo);
    break;
  case CompositableType::IMAGE:
    result = new ImageHost(aTextureInfo);
    break;
#ifdef MOZ_WIDGET_GONK
  case CompositableType::IMAGE_OVERLAY:
    result = new ImageHostOverlay(aTextureInfo);
    break;
#endif
  case CompositableType::CONTENT_SINGLE:
    result = new ContentHostSingleBuffered(aTextureInfo);
    break;
  case CompositableType::CONTENT_DOUBLE:
    result = new ContentHostDoubleBuffered(aTextureInfo);
    break;
  default:
    NS_ERROR("Unknown CompositableType");
  }
  return result;
}

#ifdef MOZ_DUMP_PAINTING
void
CompositableHost::DumpTextureHost(std::stringstream& aStream, TextureHost* aTexture)
{
  if (!aTexture) {
    return;
  }
  RefPtr<gfx::DataSourceSurface> dSurf = aTexture->GetAsSurface();
  if (!dSurf) {
    return;
  }
  gfxPlatform *platform = gfxPlatform::GetPlatform();
  RefPtr<gfx::DrawTarget> dt = platform->CreateDrawTargetForData(dSurf->GetData(),
                                                                 dSurf->GetSize(),
                                                                 dSurf->Stride(),
                                                                 dSurf->GetFormat());
  aStream << gfxUtils::GetAsDataURI(dt).get();
}
#endif

namespace CompositableMap {

typedef std::map<uint64_t, PCompositableParent*> CompositableMap_t;
static CompositableMap_t* sCompositableMap = nullptr;
bool IsCreated() {
  return sCompositableMap != nullptr;
}
PCompositableParent* Get(uint64_t aID)
{
  if (!IsCreated() || aID == 0) {
    return nullptr;
  }
  CompositableMap_t::iterator it = sCompositableMap->find(aID);
  if (it == sCompositableMap->end()) {
    return nullptr;
  }
  return it->second;
}
void Set(uint64_t aID, PCompositableParent* aParent)
{
  if (!IsCreated() || aID == 0) {
    return;
  }
  (*sCompositableMap)[aID] = aParent;
}
void Erase(uint64_t aID)
{
  if (!IsCreated() || aID == 0) {
    return;
  }
  CompositableMap_t::iterator it = sCompositableMap->find(aID);
  if (it != sCompositableMap->end()) {
    sCompositableMap->erase(it);
  }
}
void Clear()
{
  if (!IsCreated()) {
    return;
  }
  sCompositableMap->clear();
}
void Create()
{
  if (sCompositableMap == nullptr) {
    sCompositableMap = new CompositableMap_t;
  }
}
void Destroy()
{
  Clear();
  delete sCompositableMap;
  sCompositableMap = nullptr;
}

} 

} 
} 
