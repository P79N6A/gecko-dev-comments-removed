






































class gfxASurface;
class nsIntSize;

namespace mozilla {

namespace Framebuffer {


















bool Open(nsIntSize* aScreenSize);




void Close();


gfxASurface* BackBuffer();


void Present();

} 

} 
