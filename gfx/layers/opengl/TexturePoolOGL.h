



#ifndef GFX_TEXTUREPOOLOGL_H
#define GFX_TEXTUREPOOLOGL_H

#include "GLContextTypes.h"             

namespace mozilla {
namespace gl {

class GLContext;





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
