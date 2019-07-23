

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H


#if defined( FT_DEBUG_LEVEL_ERROR )

  

  FT_BASE_DEF( void )
  FT_Message( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }


  

  FT_BASE_DEF( void )
  FT_Panic( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );

    exit( EXIT_FAILURE );
  }

#endif 



#ifdef FT_DEBUG_LEVEL_TRACE

  
  int  ft_trace_levels[trace_count];


  
#define FT_TRACE_DEF( x )  #x ,

  static const char*  ft_trace_toggles[trace_count + 1] =
  {
#include FT_INTERNAL_TRACE_H
    NULL
  };

#undef FT_TRACE_DEF


  

  FT_BASE_DEF( FT_Int )
  FT_Trace_Get_Count( void )
  {
    return trace_count;
  }


  

  FT_BASE_DEF( const char * )
  FT_Trace_Get_Name( FT_Int  idx )
  {
    int  max = FT_Trace_Get_Count();


    if ( idx < max )
      return ft_trace_toggles[idx];
    else
      return NULL;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    const char*  ft2_debug = getenv( "FT2_DEBUG" );


    if ( ft2_debug )
    {
      const char*  p = ft2_debug;
      const char*  q;


      for ( ; *p; p++ )
      {
        
        if ( *p == ' ' || *p == '\t' || *p == ',' || *p == ';' || *p == '=' )
          continue;

        
        q = p;
        while ( *p && *p != ':' )
          p++;

        if ( *p == ':' && p > q )
        {
          FT_Int  n, i, len = (FT_Int)( p - q );
          FT_Int  level = -1, found = -1;


          for ( n = 0; n < trace_count; n++ )
          {
            const char*  toggle = ft_trace_toggles[n];


            for ( i = 0; i < len; i++ )
            {
              if ( toggle[i] != q[i] )
                break;
            }

            if ( i == len && toggle[i] == 0 )
            {
              found = n;
              break;
            }
          }

          
          p++;
          if ( *p )
          {
            level = *p++ - '0';
            if ( level < 0 || level > 7 )
              level = -1;
          }

          if ( found >= 0 && level >= 0 )
          {
            if ( found == trace_any )
            {
              
              for ( n = 0; n < trace_count; n++ )
                ft_trace_levels[n] = level;
            }
            else
              ft_trace_levels[found] = level;
          }
        }
      }
    }
  }


#else  


  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    
  }


  FT_BASE_DEF( FT_Int )
  FT_Trace_Get_Count( void )
  {
    return 0;
  }


  FT_BASE_DEF( const char * )
  FT_Trace_Get_Name( FT_Int  idx )
  {
    FT_UNUSED( idx );

    return NULL;
  }


#endif 



