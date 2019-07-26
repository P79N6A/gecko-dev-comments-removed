




#ifndef CANVASIMAGECACHE_H_
#define CANVASIMAGECACHE_H_

namespace mozilla {
namespace dom {
class Element;
} 
} 
class nsHTMLCanvasElement;
class imgIRequest;
class gfxASurface;

#include "gfxPoint.h"

namespace mozilla {

class CanvasImageCache {
public:
  




  static void NotifyDrawImage(dom::Element* aImage,
                              nsHTMLCanvasElement* aCanvas,
                              imgIRequest* aRequest,
                              gfxASurface* aSurface,
                              const gfxIntSize& aSize);

  





  static gfxASurface* Lookup(dom::Element* aImage,
                             nsHTMLCanvasElement* aCanvas,
                             gfxIntSize* aSize);
};

}

#endif 
