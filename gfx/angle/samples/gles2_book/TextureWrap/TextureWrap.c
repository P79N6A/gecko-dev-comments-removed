














#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   
   GLuint programObject;

   
   GLint  positionLoc;
   GLint  texCoordLoc;

   
   GLint samplerLoc;

   
   GLint offsetLoc;

   
   GLuint textureId;

} UserData;




GLubyte* GenCheckImage( int width, int height, int checkSize )
{
   int x,
       y;
   GLubyte *pixels = malloc( width * height * 3 );
   
   if ( pixels == NULL )
      return NULL;

   for ( y = 0; y < height; y++ )
      for ( x = 0; x < width; x++ )
      {
         GLubyte rColor = 0;
         GLubyte bColor = 0;

         if ( ( x / checkSize ) % 2 == 0 )
         {
            rColor = 255 * ( ( y / checkSize ) % 2 );
            bColor = 255 * ( 1 - ( ( y / checkSize ) % 2 ) );
         }
         else
         {
            bColor = 255 * ( ( y / checkSize ) % 2 );
            rColor = 255 * ( 1 - ( ( y / checkSize ) % 2 ) );
         }

         pixels[(y * height + x) * 3] = rColor;
         pixels[(y * height + x) * 3 + 1] = 0;
         pixels[(y * height + x) * 3 + 2] = bColor; 
      } 

   return pixels;
}




GLuint CreateTexture2D( )
{
   
   GLuint textureId;
   int    width = 256,
          height = 256;
   GLubyte *pixels;
      
   pixels = GenCheckImage( width, height, 64 );
   if ( pixels == NULL )
      return 0;

   
   glGenTextures ( 1, &textureId );

   
   glBindTexture ( GL_TEXTURE_2D, textureId );

   
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 
                  0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
   
   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

   return textureId;

}





int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLbyte vShaderStr[] =
      "uniform float u_offset;      \n"
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   gl_Position.x += u_offset;\n"
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

   
   userData->offsetLoc = glGetUniformLocation( userData->programObject, "u_offset" );

   
   userData->textureId = CreateTexture2D ();

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLfloat vVertices[] = { -0.3f,  0.3f, 0.0f, 1.0f,  
                           -1.0f,  -1.0f,              
                           -0.3f, -0.3f, 0.0f, 1.0f, 
                           -1.0f,  2.0f,              
                            0.3f, -0.3f, 0.0f, 1.0f, 
                            2.0f,  2.0f,              
                            0.3f,  0.3f, 0.0f, 1.0f,  
                            2.0f,  -1.0f               
                         };
   GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
      
   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   glClear ( GL_COLOR_BUFFER_BIT );

   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->positionLoc, 4, GL_FLOAT, 
                           GL_FALSE, 6 * sizeof(GLfloat), vVertices );
   
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 6 * sizeof(GLfloat), &vVertices[4] );

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );

   
   glUniform1i ( userData->samplerLoc, 0 );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glUniform1f ( userData->offsetLoc, -0.7f );   
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glUniform1f ( userData->offsetLoc, 0.0f );
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
   glUniform1f ( userData->offsetLoc, 0.7f );
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

   esCreateWindow ( &esContext, TEXT("Texture Wrap"), 640, 480, ES_WINDOW_RGB );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
