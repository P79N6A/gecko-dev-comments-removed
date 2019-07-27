







#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"

#include "common/debug.h"
#include "common/mathutil.h"

namespace rx
{

TextureStorage::TextureStorage()
    : mFirstRenderTargetSerial(0),
      mRenderTargetSerialsLayerStride(0)
{}

void TextureStorage::initializeSerials(unsigned int rtSerialsToReserve, unsigned int rtSerialsLayerStride)
{
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(rtSerialsToReserve);
    mRenderTargetSerialsLayerStride = rtSerialsLayerStride;
}

unsigned int TextureStorage::getRenderTargetSerial(const gl::ImageIndex &index) const
{
    unsigned int layerOffset = (index.hasLayer() ? (static_cast<unsigned int>(index.layerIndex) * mRenderTargetSerialsLayerStride) : 0);
    return mFirstRenderTargetSerial + static_cast<unsigned int>(index.mipIndex) + layerOffset;
}

}
