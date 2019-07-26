





































#include "cf2ft.h"
#include "cf2error.h"


  FT_LOCAL_DEF( void )
  cf2_setError( FT_Error*  error,
                FT_Error   value )
  {
    if ( error && *error == 0 )
      *error = value;
  }



