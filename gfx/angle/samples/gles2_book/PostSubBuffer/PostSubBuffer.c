
















#include <stdlib.h>
#include "esUtil.h"

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240

typedef struct
{
   
   GLuint programObject;

   
   GLint  positionLoc;

   
   GLint  mvpLoc;
   
   
   GLfloat  *vertices;
   GLushort   *indices;
   int       numIndices;

   
   GLfloat   angle;

   
   ESMatrix  mvpMatrix;
} UserData;




int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   GLbyte vShaderStr[] =  
      "uniform mat4 u_mvpMatrix;                   \n"
      "attribute vec4 a_position;                  \n"
      "void main()                                 \n"
      "{                                           \n"
      "   gl_Position = u_mvpMatrix * a_position;  \n"
      "}                                           \n";
   
   GLbyte fShaderStr[] =  
      "precision mediump float;                            \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 );        \n"
      "}                                                   \n";

   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );

   
   userData->mvpLoc = glGetUniformLocation( userData->programObject, "u_mvpMatrix" );
   
   
   userData->numIndices = esGenCube( 1.0, &userData->vertices,
                                     NULL, NULL, &userData->indices );
   
   
   userData->angle = 45.0f;

   
   glClearColor ( 0.0f, 0.0f, 1.0f, 0.0f );
   glClear ( GL_COLOR_BUFFER_BIT );
   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}





void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = (UserData*) esContext->userData;
   ESMatrix perspective;
   ESMatrix modelview;
   float    aspect;
   
   
   userData->angle += ( deltaTime * 40.0f );
   if( userData->angle >= 360.0f )
      userData->angle -= 360.0f;

   
   aspect = (GLfloat) esContext->width / (GLfloat) esContext->height;
   
   
   esMatrixLoadIdentity( &perspective );
   esPerspective( &perspective, 60.0f, aspect, 1.0f, 20.0f );

   
   esMatrixLoadIdentity( &modelview );

   
   esTranslate( &modelview, 0.0, 0.0, -2.0 );

   
   esRotate( &modelview, userData->angle, 1.0, 0.0, 1.0 );
   
   
   
   esMatrixMultiply( &userData->mvpMatrix, &modelview, &perspective );
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   
   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   
   glClear ( GL_COLOR_BUFFER_BIT );

   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->vertices );
   
   glEnableVertexAttribArray ( userData->positionLoc );
   
   
   
   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );
   
   
   glDrawElements ( GL_TRIANGLES, userData->numIndices, GL_UNSIGNED_SHORT, userData->indices );

   eglPostSubBufferNV ( esContext->eglDisplay, esContext->eglSurface, 60, 60, WINDOW_WIDTH - 120, WINDOW_HEIGHT - 120 );
}




void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   if ( userData->vertices != NULL )
   {
      free ( userData->vertices );
   }

   if ( userData->indices != NULL )
   {
      free ( userData->indices );
   }

   
   glDeleteProgram ( userData->programObject );
}


int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, TEXT("Simple Vertex Shader"), WINDOW_WIDTH, WINDOW_HEIGHT, ES_WINDOW_RGB | ES_WINDOW_POST_SUB_BUFFER_SUPPORTED );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   esRegisterUpdateFunc ( &esContext, Update );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
