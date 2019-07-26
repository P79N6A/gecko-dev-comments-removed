




#ifndef GFX_LAYERSLOGGING_H
#define GFX_LAYERSLOGGING_H

#include "FrameMetrics.h"               
#include "GraphicsFilter.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "nsAString.h"
#include "nsPrintfCString.h"            
#include "nsRegion.h"                   
#include "nscore.h"                     

class gfx3DMatrix;
struct gfxRGBA;
struct nsIntPoint;
struct nsIntRect;
struct nsIntSize;

namespace mozilla {
namespace gfx {
class Matrix4x4;
template <class units> struct RectTyped;
}

namespace layers {

nsACString&
AppendToString(nsACString& s, const void* p,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const GraphicsFilter& f,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, FrameMetrics::ViewID n,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const gfxRGBA& c,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const gfx3DMatrix& m,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const nsIntPoint& p,
               const char* pfx="", const char* sfx="");

template<class T>
nsACString&
AppendToString(nsACString& s, const mozilla::gfx::PointTyped<T>& p,
               const char* pfx="", const char* sfx="")
{
  s += pfx;
  s += nsPrintfCString("(x=%f, y=%f)", p.x, p.y);
  return s += sfx;
}

nsACString&
AppendToString(nsACString& s, const nsIntRect& r,
               const char* pfx="", const char* sfx="");

template<class T>
nsACString&
AppendToString(nsACString& s, const mozilla::gfx::RectTyped<T>& r,
               const char* pfx="", const char* sfx="")
{
  s += pfx;
  s.AppendPrintf(
    "(x=%f, y=%f, w=%f, h=%f)",
    r.x, r.y, r.width, r.height);
  return s += sfx;
}

nsACString&
AppendToString(nsACString& s, const nsIntRegion& r,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const nsIntSize& sz,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const FrameMetrics& m,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const mozilla::gfx::IntSize& size,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const mozilla::gfx::Matrix4x4& m,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, const mozilla::gfx::Filter filter,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, mozilla::layers::TextureFlags flags,
               const char* pfx="", const char* sfx="");

nsACString&
AppendToString(nsACString& s, mozilla::gfx::SurfaceFormat format,
               const char* pfx="", const char* sfx="");

} 
} 

#endif 
