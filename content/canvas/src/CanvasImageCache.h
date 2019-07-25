




































#ifndef CANVASIMAGECACHE_H_
#define CANVASIMAGECACHE_H_

class nsIDOMElement;
class nsHTMLCanvasElement;
class imgIRequest;
class gfxASurface;

#include "gfxPoint.h"

namespace mozilla {

class CanvasImageCache {
public:
  




  static void NotifyDrawImage(nsIDOMElement* aImage,
                              nsHTMLCanvasElement* aCanvas,
                              imgIRequest* aRequest,
                              gfxASurface* aSurface,
                              const gfxIntSize& aSize);

  





  static gfxASurface* Lookup(nsIDOMElement* aImage,
                             nsHTMLCanvasElement* aCanvas,
                             gfxIntSize* aSize);
};

}

#endif 
