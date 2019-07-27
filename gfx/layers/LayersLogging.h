




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

void
AppendToString(std::stringstream& aStream, const void* p,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const GraphicsFilter& f,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, FrameMetrics::ViewID n,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const gfxRGBA& c,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const nsIntPoint& p,
               const char* pfx="", const char* sfx="");

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::PointTyped<T>& p,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx << p << sfx;
}

void
AppendToString(std::stringstream& aStream, const nsIntRect& r,
               const char* pfx="", const char* sfx="");

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::RectTyped<T>& r,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx;
  aStream << nsPrintfCString(
    "(x=%f, y=%f, w=%f, h=%f)",
    r.x, r.y, r.width, r.height).get();
  aStream << sfx;
}

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::IntRectTyped<T>& r,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx;
  aStream << nsPrintfCString(
    "(x=%d, y=%d, w=%d, h=%d)",
    r.x, r.y, r.width, r.height).get();
  aStream << sfx;
}

void
AppendToString(std::stringstream& aStream, const nsIntRegion& r,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const nsIntSize& sz,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const FrameMetrics& m,
               const char* pfx="", const char* sfx="", bool detailed = false);

void
AppendToString(std::stringstream& aStream, const ScrollableLayerGuid& s,
               const char* pfx="", const char* sfx="");

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::MarginTyped<T>& m,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx;
  aStream << nsPrintfCString(
    "(l=%f, t=%f, r=%f, b=%f)",
    m.left, m.top, m.right, m.bottom).get();
  aStream << sfx;
}

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::SizeTyped<T>& sz,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx;
  aStream << nsPrintfCString(
    "(w=%f, h=%f)",
    sz.width, sz.height).get();
  aStream << sfx;
}

template<class T>
void
AppendToString(std::stringstream& aStream, const mozilla::gfx::IntSizeTyped<T>& sz,
               const char* pfx="", const char* sfx="")
{
  aStream << pfx;
  aStream << nsPrintfCString(
    "(w=%d, h=%d)",
    sz.width, sz.height).get();
  aStream << sfx;
}

void
AppendToString(std::stringstream& aStream, const mozilla::gfx::Matrix4x4& m,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const mozilla::gfx::Matrix5x4& m,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, const mozilla::gfx::Filter filter,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, mozilla::layers::TextureFlags flags,
               const char* pfx="", const char* sfx="");

void
AppendToString(std::stringstream& aStream, mozilla::gfx::SurfaceFormat format,
               const char* pfx="", const char* sfx="");


template <typename T>
std::string
Stringify(const T& obj)
{
  std::stringstream ss;
  AppendToString(ss, obj);
  return ss.str();
}

} 
} 




void print_stderr(std::stringstream& aStr);
void fprint_stderr(FILE* aFile, std::stringstream& aStr);

#endif 
