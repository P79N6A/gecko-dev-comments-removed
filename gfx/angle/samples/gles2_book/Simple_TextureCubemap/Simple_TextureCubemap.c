













#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   
   GLuint programObject;

   
   GLint  positionLoc;
   GLint  normalLoc;

   
   GLint samplerLoc;

   
   GLuint textureId;

   
   int      numIndices;
   GLfloat *vertices;
   GLfloat *normals;
   GLushort  *indices;

} UserData;




GLuint CreateSimpleTextureCubemap( )
{
   GLuint textureId;
   
   GLubyte cubePixels[6][3] =
   {
      
      255, 0, 0,
      
      0, 255, 0, 
      
      0, 0, 255,
      
      255, 255, 0,
      
      255, 0, 255,
      
      255, 255, 255
   };
   
   
   glGenTextures ( 1, &textureId );

   
   glBindTexture ( GL_TEXTURE_CUBE_MAP, textureId );
   
   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[0] );

   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[1] );

   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[2] );

   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[3] );

   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[4] );

   
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, 1, 0, 
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[5] );

   
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

}





int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLbyte vShaderStr[] =  
      "attribute vec4 a_position;   \n"
      "attribute vec3 a_normal;     \n"
      "varying vec3 v_normal;       \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_normal = a_normal;      \n"
      "}                            \n";
   
   GLbyte fShaderStr[] =  
      "precision mediump float;                            \n"
      "varying vec3 v_normal;                              \n"
      "uniform samplerCube s_texture;                      \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = textureCube( s_texture, v_normal );\n"
      "}                                                   \n";

   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   userData->normalLoc = glGetAttribLocation ( userData->programObject, "a_normal" );
   
   
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   
   userData->textureId = CreateSimpleTextureCubemap ();

   
   userData->numIndices = esGenSphere ( 20, 0.75f, &userData->vertices, &userData->normals, 
                                        NULL, &userData->indices );

   
   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
      
   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   glClear ( GL_COLOR_BUFFER_BIT );

   
   glCullFace ( GL_BACK );
   glEnable ( GL_CULL_FACE );
   
   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 0, userData->vertices );
   
   glVertexAttribPointer ( userData->normalLoc, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->normals );

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->normalLoc );

   
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_CUBE_MAP, userData->textureId );

   
   glUniform1i ( userData->samplerLoc, 0 );

   glDrawElements ( GL_TRIANGLES, userData->numIndices, 
                    GL_UNSIGNED_SHORT, userData->indices );

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}




void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   
   glDeleteTextures ( 1, &userData->textureId );

   
   glDeleteProgram ( userData->programObject );

   free ( userData->vertices );
   free ( userData->normals );
}


int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, TEXT("Simple Texture Cubemap"), 320, 240, ES_WINDOW_RGB );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
