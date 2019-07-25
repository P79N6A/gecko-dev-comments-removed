















#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   
   GLuint programObject;

   
   GLint  positionLoc;
   GLint  texCoordLoc;

   
   GLint samplerLoc;

   
   GLuint textureId;

} UserData;




GLuint CreateSimpleTexture2D( )
{
   
   GLuint textureId;
   
   
   GLubyte pixels[4 * 3] =
   {  
      255,   0,   0, 
        0, 255,   0, 
        0,   0, 255, 
      255, 255,   0  
   };

   
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

   
   glGenTextures ( 1, &textureId );

   
   glBindTexture ( GL_TEXTURE_2D, textureId );

   
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

}





int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLbyte vShaderStr[] =  
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";
   
   GLbyte fShaderStr[] =  
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "}                                                   \n";

   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   
   
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   
   userData->textureId = CreateSimpleTexture2D ();

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLfloat vVertices[] = { -0.5f,  0.5f, 0.0f,  
                            0.0f,  0.0f,        
                           -0.5f, -0.5f, 0.0f,  
                            0.0f,  1.0f,        
                            0.5f, -0.5f, 0.0f,  
                            1.0f,  1.0f,        
                            0.5f,  0.5f, 0.0f,  
                            1.0f,  0.0f         
                         };
   GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
      
   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   glClear ( GL_COLOR_BUFFER_BIT );

   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 5 * sizeof(GLfloat), vVertices );
   
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );

   
   glUniform1i ( userData->samplerLoc, 0 );

   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}




void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   
   glDeleteTextures ( 1, &userData->textureId );

   
   glDeleteProgram ( userData->programObject );
}


int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, TEXT("Simple Texture 2D"), 320, 240, ES_WINDOW_RGB );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
