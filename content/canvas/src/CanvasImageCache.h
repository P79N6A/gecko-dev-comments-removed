




#ifndef CANVASIMAGECACHE_H_
#define CANVASIMAGECACHE_H_

namespace mozilla {
namespace dom {
class Element;
class HTMLCanvasElement;
} 
namespace gfx {
class SourceSurface;
} 
} 
class imgIRequest;
class gfxASurface;

#include "gfxPoint.h"

namespace mozilla {

class CanvasImageCache {
  typedef mozilla::gfx::SourceSurface SourceSurface;
public:
  




  static void NotifyDrawImage(dom::Element* aImage,
                              dom::HTMLCanvasElement* aCanvas,
                              imgIRequest* aRequest,
                              SourceSurface* aSource,
                              const gfxIntSize& aSize);

  





  static SourceSurface* Lookup(dom::Element* aImage,
                               dom::HTMLCanvasElement* aCanvas,
                               gfxIntSize* aSize);
};

}

#endif 
