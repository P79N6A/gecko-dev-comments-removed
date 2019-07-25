














#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   
   GLuint programObject;

   
   GLint  positionLoc;

   
   GLint  colorLoc;

} UserData;




int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLbyte vShaderStr[] =  
      "attribute vec4 a_position;   \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "}                            \n";
   
   GLbyte fShaderStr[] =  
      "precision mediump float;  \n"
      "uniform vec4  u_color;    \n"
      "void main()               \n"
      "{                         \n"
      "  gl_FragColor = u_color; \n"
      "}                         \n";

   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   
   
   userData->colorLoc = glGetUniformLocation ( userData->programObject, "u_color" );

   
   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   
   
   glClearStencil ( 0x1 );

   
   glClearDepthf( 0.75f );

   
   glEnable( GL_DEPTH_TEST );
   glEnable( GL_STENCIL_TEST );

   return TRUE;
}





void Draw ( ESContext *esContext )
{
   int  i;

   UserData *userData = esContext->userData;

   GLfloat vVertices[] = { 
       -0.75f,  0.25f,  0.50f, 
       -0.25f,  0.25f,  0.50f,
       -0.25f,  0.75f,  0.50f,
       -0.75f,  0.75f,  0.50f,
	    0.25f,  0.25f,  0.90f, 
		0.75f,  0.25f,  0.90f,
		0.75f,  0.75f,  0.90f,
		0.25f,  0.75f,  0.90f,
	   -0.75f, -0.75f,  0.50f, 
       -0.25f, -0.75f,  0.50f,
       -0.25f, -0.25f,  0.50f,
       -0.75f, -0.25f,  0.50f,
        0.25f, -0.75f,  0.50f, 
        0.75f, -0.75f,  0.50f,
        0.75f, -0.25f,  0.50f,
        0.25f, -0.25f,  0.50f,
       -1.00f, -1.00f,  0.00f, 
        1.00f, -1.00f,  0.00f,
        1.00f,  1.00f,  0.00f,
       -1.00f,  1.00f,  0.00f
   };

   GLubyte indices[][6] = { 
       {  0,  1,  2,  0,  2,  3 }, 
       {  4,  5,  6,  4,  6,  7 }, 
       {  8,  9, 10,  8, 10, 11 }, 
       { 12, 13, 14, 12, 14, 15 }, 
       { 16, 17, 18, 16, 18, 19 }  
   };
   
#define NumTests  4
   GLfloat  colors[NumTests][4] = { 
       { 1.0f, 0.0f, 0.0f, 1.0f },
       { 0.0f, 1.0f, 0.0f, 1.0f },
       { 0.0f, 0.0f, 1.0f, 1.0f },
       { 1.0f, 1.0f, 0.0f, 0.0f }
   };

   GLint   numStencilBits;
   GLuint  stencilValues[NumTests] = { 
      0x7, 
      0x0, 
      0x2, 
      0xff 
           
   };

   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 0, vVertices );
  
   glEnableVertexAttribArray ( userData->positionLoc );

   
   
   
   
   
   
   
   
   
   
   
   
   
   glStencilFunc( GL_LESS, 0x7, 0x3 );
   glStencilOp( GL_REPLACE, GL_DECR, GL_DECR );
   glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices[0] );
 
   
   
   
   
   
   
   
   
   
   
   
   
   glStencilFunc( GL_GREATER, 0x3, 0x3 );
   glStencilOp( GL_KEEP, GL_DECR, GL_KEEP );
   glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices[1] );

   
   
   
   
   
   
   
   
   
   
   
   
   glStencilFunc( GL_EQUAL, 0x1, 0x3 );
   glStencilOp( GL_KEEP, GL_INCR, GL_INCR );
   glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices[2] );

   
   
   
   
   
   
   
   
   
   
   
   
   
   glStencilFunc( GL_EQUAL, 0x2, 0x1 );
   glStencilOp( GL_INVERT, GL_KEEP, GL_KEEP );
   glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices[3] );
   
   
   
   
   
   glGetIntegerv( GL_STENCIL_BITS, &numStencilBits );
   
   stencilValues[3] = ~(((1 << numStencilBits) - 1) & 0x1) & 0xff;

   
   
   
   
   glStencilMask( 0x0 );
   
   for ( i = 0; i < NumTests; ++i )
   {
      glStencilFunc( GL_EQUAL, stencilValues[i], 0xff );
      glUniform4fv( userData->colorLoc, 1, colors[i] );
      glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices[4] );
   }

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}




void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   
   glDeleteProgram ( userData->programObject );
}


int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, TEXT("Stencil Test"), 320, 240,
                    ES_WINDOW_RGB | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
