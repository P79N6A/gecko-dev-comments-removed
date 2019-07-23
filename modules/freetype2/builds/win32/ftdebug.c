

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H


#ifdef FT_DEBUG_LEVEL_ERROR


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>


#ifdef _WIN32_WCE

  void
  OutputDebugStringEx( const char*  str )
  {
    static WCHAR  buf[8192];


    int sz = MultiByteToWideChar( CP_ACP, 0, str, -1, buf,
                                  sizeof ( buf ) / sizeof ( *buf ) );
    if ( !sz )
      lstrcpyW( buf, L"OutputDebugStringEx: MultiByteToWideChar failed" );

    OutputDebugStringW( buf );
  }

#else

#define OutputDebugStringEx  OutputDebugStringA

#endif


  FT_BASE_DEF( void )
  FT_Message( const char*  fmt, ... )
  {
    static char  buf[8192];
    va_list      ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    
    vsprintf( buf, fmt, ap );
    OutputDebugStringEx( buf );
    va_end( ap );
  }


  FT_BASE_DEF( void )
  FT_Panic( const char*  fmt, ... )
  {
    static char  buf[8192];
    va_list      ap;


    va_start( ap, fmt );
    vsprintf( buf, fmt, ap );
    OutputDebugStringEx( buf );
    va_end( ap );

    exit( EXIT_FAILURE );
  }


#ifdef FT_DEBUG_LEVEL_TRACE


  
  int  ft_trace_levels[trace_count];

  
#define FT_TRACE_DEF( x )  #x ,

  static const char*  ft_trace_toggles[trace_count + 1] =
  {
#include FT_INTERNAL_TRACE_H
    NULL
  };

#undef FT_TRACE_DEF


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
#ifdef _WIN32_WCE

    
    
    
    
    const char*  ft2_debug = 0;

#else

    const char*  ft2_debug = getenv( "FT2_DEBUG" );

#endif

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
          int  n, i, len = p - q;
          int  level = -1, found = -1;


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


#endif 

#endif 



