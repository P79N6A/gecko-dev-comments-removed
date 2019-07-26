



#ifndef GFX_TEXTUREPOOLOGL_H
#define GFX_TEXTUREPOOLOGL_H

#include "GLContext.h"

namespace mozilla {
namespace gl {





class TexturePoolOGL
{
public:
  
  
  static GLuint AcquireTexture();

  
  
  static void Fill(GLContext* aContext);

  
  static void Init();

  
  static void Shutdown();
};

} 
} 

#endif 
