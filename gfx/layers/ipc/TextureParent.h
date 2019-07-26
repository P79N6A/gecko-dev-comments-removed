




#ifndef MOZILLA_LAYERS_TEXTUREPARENT_H
#define MOZILLA_LAYERS_TEXTUREPARENT_H

#include "mozilla/layers/PTextureParent.h"
#include "mozilla/layers/CompositorTypes.h"
#include "CompositableHost.h"

namespace mozilla {
namespace layers {

class TextureHost;
class CompositableHost;
class TextureInfo;

class TextureParent : public PTextureParent
{
public:
  TextureParent(const TextureInfo& aInfo, CompositableParent* aCompositable);
  virtual ~TextureParent();

  void SetTextureHost(TextureHost* aHost);

  TextureHost* GetTextureHost() const;
  CompositableHost* GetCompositableHost() const;

  const TextureInfo& GetTextureInfo() const
  {
    return mTextureInfo;
  }

  bool SurfaceTypeChanged(SurfaceDescriptor::Type aNewSurfaceType);
  void SetCurrentSurfaceType(SurfaceDescriptor::Type aNewSurfaceType);
  SurfaceDescriptor::Type GetSurfaceType() const
  {
    return mLastSurfaceType;
  }

  uint64_t GetCompositorID();

  bool EnsureTextureHost(SurfaceDescriptor::Type aSurfaceType);
private:
  TextureInfo mTextureInfo;
  RefPtr<TextureHost> mTextureHost;
  SurfaceDescriptor::Type mLastSurfaceType;
};

} 
} 

#endif
