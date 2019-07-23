






















#ifndef __FTDEBUG_H__
#define __FTDEBUG_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  
  
  
#ifdef FT_DEBUG_LEVEL_TRACE
#undef  FT_DEBUG_LEVEL_ERROR
#define FT_DEBUG_LEVEL_ERROR
#endif


  
  
  
  
  
  

#ifdef FT_DEBUG_LEVEL_TRACE

#define FT_TRACE_DEF( x )  trace_ ## x ,

  
  typedef enum  FT_Trace_
  {
#include FT_INTERNAL_TRACE_H
    trace_count

  } FT_Trace;


  
  extern int  ft_trace_levels[trace_count];

#undef FT_TRACE_DEF

#endif 


  
  
  
  
  
  
  
  
  
  

#ifdef FT_DEBUG_LEVEL_TRACE

#define FT_TRACE( level, varformat )                      \
          do                                              \
          {                                               \
            if ( ft_trace_levels[FT_COMPONENT] >= level ) \
              FT_Message varformat;                       \
          } while ( 0 )

#else 

#define FT_TRACE( level, varformat )  do ; while ( 0 )      /* nothing */

#endif 


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Int )
  FT_Trace_Get_Count( void );


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( const char * )
  FT_Trace_Get_Name( FT_Int  idx );


  
  
  
  
  
  
  
  
  

#define FT_TRACE0( varformat )  FT_TRACE( 0, varformat )
#define FT_TRACE1( varformat )  FT_TRACE( 1, varformat )
#define FT_TRACE2( varformat )  FT_TRACE( 2, varformat )
#define FT_TRACE3( varformat )  FT_TRACE( 3, varformat )
#define FT_TRACE4( varformat )  FT_TRACE( 4, varformat )
#define FT_TRACE5( varformat )  FT_TRACE( 5, varformat )
#define FT_TRACE6( varformat )  FT_TRACE( 6, varformat )
#define FT_TRACE7( varformat )  FT_TRACE( 7, varformat )


  
  
  
  
  
  
  

#ifdef FT_DEBUG_LEVEL_ERROR

#define FT_ERROR( varformat )  FT_Message  varformat

#else  

#define FT_ERROR( varformat )  do ; while ( 0 )      /* nothing */

#endif 


  
  
  
  
  

#ifdef FT_DEBUG_LEVEL_ERROR

#define FT_ASSERT( condition )                                      \
          do                                                        \
          {                                                         \
            if ( !( condition ) )                                   \
              FT_Panic( "assertion failed on line %d of file %s\n", \
                        __LINE__, __FILE__ );                       \
          } while ( 0 )

#else 

#define FT_ASSERT( condition )  do ; while ( 0 )

#endif 


  
  
  
  
  

#ifdef FT_DEBUG_LEVEL_ERROR

#include "stdio.h"  

  
  FT_BASE( void )
  FT_Message( const char*  fmt,
              ... );

  
  FT_BASE( void )
  FT_Panic( const char*  fmt,
            ... );

#endif 


  FT_BASE( void )
  ft_debug_init( void );


#if defined( _MSC_VER )      

  
  
#pragma warning( disable : 4127 )

#endif 


FT_END_HEADER

#endif 



