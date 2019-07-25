














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





GLboolean GenMipMap2D( GLubyte *src, GLubyte **dst, int srcWidth, int srcHeight, int *dstWidth, int *dstHeight )
{
   int x,
       y;
   int texelSize = 3;

   *dstWidth = srcWidth / 2;
   if ( *dstWidth <= 0 )
      *dstWidth = 1;

   *dstHeight = srcHeight / 2;
   if ( *dstHeight <= 0 )
      *dstHeight = 1;

   *dst = malloc ( sizeof(GLubyte) * texelSize * (*dstWidth) * (*dstHeight) );
   if ( *dst == NULL )
      return GL_FALSE;

   for ( y = 0; y < *dstHeight; y++ )
   {
      for( x = 0; x < *dstWidth; x++ )
      {
         int srcIndex[4];
         float r = 0.0f,
               g = 0.0f,
               b = 0.0f;
         int sample;

         
         
         srcIndex[0] = 
            (((y * 2) * srcWidth) + (x * 2)) * texelSize;
         srcIndex[1] = 
            (((y * 2) * srcWidth) + (x * 2 + 1)) * texelSize; 
         srcIndex[2] = 
            ((((y * 2) + 1) * srcWidth) + (x * 2)) * texelSize;
         srcIndex[3] = 
            ((((y * 2) + 1) * srcWidth) + (x * 2 + 1)) * texelSize;

         
         for ( sample = 0; sample < 4; sample++ )
         {
            r += src[srcIndex[sample]];
            g += src[srcIndex[sample] + 1];
            b += src[srcIndex[sample] + 2];
         }

         
         r /= 4.0;
         g /= 4.0;
         b /= 4.0;

         
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize ] = (GLubyte)( r );
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize + 1] = (GLubyte)( g );
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize + 2] = (GLubyte)( b );
      }
   }

   return GL_TRUE;
}




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




GLuint CreateMipMappedTexture2D( )
{
   
   GLuint textureId;
   int    width = 256,
          height = 256;
   int    level;
   GLubyte *pixels;
   GLubyte *prevImage;
   GLubyte *newImage;
      
   pixels = GenCheckImage( width, height, 8 );
   if ( pixels == NULL )
      return 0;

   
   glGenTextures ( 1, &textureId );

   
   glBindTexture ( GL_TEXTURE_2D, textureId );

   
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 
                  0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
   
   level = 1;
   prevImage = &pixels[0];
   
   while ( width > 1 && height > 1 )
   {
      int newWidth,
          newHeight;

      
      GenMipMap2D( prevImage, &newImage, width, height, 
                   &newWidth, &newHeight );

      
      glTexImage2D( GL_TEXTURE_2D, level, GL_RGB, 
                    newWidth, newHeight, 0, GL_RGB,
                    GL_UNSIGNED_BYTE, newImage );

      
      free ( prevImage );

      
      prevImage = newImage;
      level++;

      
      width = newWidth;
      height = newHeight;
   }

   free ( newImage );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
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

   
   userData->textureId = CreateMipMappedTexture2D ();

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLfloat vVertices[] = { -0.5f,  0.5f, 0.0f, 1.5f,  
                            0.0f,  0.0f,              
                           -0.5f, -0.5f, 0.0f, 0.75f, 
                            0.0f,  1.0f,              
                            0.5f, -0.5f, 0.0f, 0.75f, 
                            1.0f,  1.0f,              
                            0.5f,  0.5f, 0.0f, 1.5f,  
                            1.0f,  0.0f               
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

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glUniform1f ( userData->offsetLoc, -0.6f );   
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

   
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glUniform1f ( userData->offsetLoc, 0.6f );
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

   esCreateWindow ( &esContext, TEXT("MipMap 2D"), 320, 240, ES_WINDOW_RGB );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
