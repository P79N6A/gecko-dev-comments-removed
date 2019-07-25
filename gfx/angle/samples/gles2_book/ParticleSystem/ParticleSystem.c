














#include <stdlib.h>
#include <math.h>
#include "esUtil.h"

#define NUM_PARTICLES	1000
#define PARTICLE_SIZE   7

typedef struct
{
   
   GLuint programObject;

   
   GLint  lifetimeLoc;
   GLint  startPositionLoc;
   GLint  endPositionLoc;
   
   
   GLint timeLoc;
   GLint colorLoc;
   GLint centerPositionLoc;
   GLint samplerLoc;

   
   GLuint textureId;

   
   float particleData[ NUM_PARTICLES * PARTICLE_SIZE ];

   
   float time;

} UserData;




GLuint LoadTexture ( char *fileName )
{
   int width,
       height;
   char *buffer = esLoadTGA ( fileName, &width, &height );
   GLuint texId;

   if ( buffer == NULL )
   {
      esLogMessage ( "Error loading (%s) image.\n", fileName );
      return 0;
   }

   glGenTextures ( 1, &texId );
   glBindTexture ( GL_TEXTURE_2D, texId );

   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
}





int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   int i;
   
   GLbyte vShaderStr[] =
      "uniform float u_time;		                           \n"
      "uniform vec3 u_centerPosition;                       \n"
      "attribute float a_lifetime;                          \n"
      "attribute vec3 a_startPosition;                      \n"
      "attribute vec3 a_endPosition;                        \n"
      "varying float v_lifetime;                            \n"
      "void main()                                          \n"
      "{                                                    \n"
      "  if ( u_time <= a_lifetime )                        \n"
      "  {                                                  \n"
      "    gl_Position.xyz = a_startPosition +              \n"
      "                      (u_time * a_endPosition);      \n"
      "    gl_Position.xyz += u_centerPosition;             \n"
      "    gl_Position.w = 1.0;                             \n"
      "  }                                                  \n"
      "  else                                               \n"
      "     gl_Position = vec4( -1000, -1000, 0, 0 );       \n"
      "  v_lifetime = 1.0 - ( u_time / a_lifetime );        \n"
      "  v_lifetime = clamp ( v_lifetime, 0.0, 1.0 );       \n"
      "  gl_PointSize = ( v_lifetime * v_lifetime ) * 40.0; \n"
      "}";
      
   GLbyte fShaderStr[] =  
      "precision mediump float;                             \n"
      "uniform vec4 u_color;		                           \n"
      "varying float v_lifetime;                            \n"
      "uniform sampler2D s_texture;                         \n"
      "void main()                                          \n"
      "{                                                    \n"
      "  vec4 texColor;                                     \n"
      "  texColor = texture2D( s_texture, gl_PointCoord );  \n"
      "  gl_FragColor = vec4( u_color ) * texColor;         \n"
      "  gl_FragColor.a *= v_lifetime;                      \n"
      "}                                                    \n";

   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   
   userData->lifetimeLoc = glGetAttribLocation ( userData->programObject, "a_lifetime" );
   userData->startPositionLoc = glGetAttribLocation ( userData->programObject, "a_startPosition" );
   userData->endPositionLoc = glGetAttribLocation ( userData->programObject, "a_endPosition" );
   
   
   userData->timeLoc = glGetUniformLocation ( userData->programObject, "u_time" );
   userData->centerPositionLoc = glGetUniformLocation ( userData->programObject, "u_centerPosition" );
   userData->colorLoc = glGetUniformLocation ( userData->programObject, "u_color" );
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

   
   srand ( 0 );
   for ( i = 0; i < NUM_PARTICLES; i++ )
   {
      float *particleData = &userData->particleData[i * PARTICLE_SIZE];
   
      
      (*particleData++) = ( (float)(rand() % 10000) / 10000.0f );

      
      (*particleData++) = ( (float)(rand() % 10000) / 5000.0f ) - 1.0f;
      (*particleData++) = ( (float)(rand() % 10000) / 5000.0f ) - 1.0f;
      (*particleData++) = ( (float)(rand() % 10000) / 5000.0f ) - 1.0f;

      
      (*particleData++) = ( (float)(rand() % 10000) / 40000.0f ) - 0.125f;
      (*particleData++) = ( (float)(rand() % 10000) / 40000.0f ) - 0.125f;
      (*particleData++) = ( (float)(rand() % 10000) / 40000.0f ) - 0.125f;

   }

   
   userData->time = 1.0f;

   userData->textureId = LoadTexture ( "smoke.tga" );
   if ( userData->textureId <= 0 )
   {
      return FALSE;
   }
   
   return TRUE;
}




void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;
  
   userData->time += deltaTime;

   if ( userData->time >= 1.0f )
   {
      float centerPos[3];
      float color[4];

      userData->time = 0.0f;

      
      centerPos[0] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;
      centerPos[1] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;
      centerPos[2] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;
      
      glUniform3fv ( userData->centerPositionLoc, 1, &centerPos[0] );

      
      color[0] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
      color[1] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
      color[2] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
      color[3] = 0.5;

      glUniform4fv ( userData->colorLoc, 1, &color[0] );
   }

   
   glUniform1f ( userData->timeLoc, userData->time );
}




void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
      
   
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   
   glClear ( GL_COLOR_BUFFER_BIT );

   
   glUseProgram ( userData->programObject );

   
   glVertexAttribPointer ( userData->lifetimeLoc, 1, GL_FLOAT, 
                           GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat), 
                           userData->particleData );
   
   glVertexAttribPointer ( userData->endPositionLoc, 3, GL_FLOAT,
                           GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
                           &userData->particleData[1] );

   glVertexAttribPointer ( userData->startPositionLoc, 3, GL_FLOAT,
                           GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
                           &userData->particleData[4] );

   
   glEnableVertexAttribArray ( userData->lifetimeLoc );
   glEnableVertexAttribArray ( userData->endPositionLoc );
   glEnableVertexAttribArray ( userData->startPositionLoc );
   
   glEnable ( GL_BLEND );
   glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

   
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );
   glEnable ( GL_TEXTURE_2D );

   
   glUniform1i ( userData->samplerLoc, 0 );

   glDrawArrays( GL_POINTS, 0, NUM_PARTICLES );
   
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

   esCreateWindow ( &esContext, TEXT("ParticleSystem"), 640, 480, ES_WINDOW_RGB );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   esRegisterUpdateFunc ( &esContext, Update );
   
   esMainLoop ( &esContext );

   ShutDown ( &esContext );
}
