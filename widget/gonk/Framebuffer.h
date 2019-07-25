






































class gfxASurface;
class nsIntRegion;
class nsIntSize;

namespace mozilla {

namespace Framebuffer {


















bool Open(nsIntSize* aScreenSize);




void Close();

bool GetSize(nsIntSize *aScreenSize);


gfxASurface* BackBuffer();



void Present(const nsIntRegion& aUpdated);

} 

} 
