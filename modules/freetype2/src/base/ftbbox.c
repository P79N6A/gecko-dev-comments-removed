

















  
  
  
  
  
  


#include <ft2build.h>
#include FT_BBOX_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_INTERNAL_CALC_H


  typedef struct  TBBox_Rec_
  {
    FT_Vector  last;
    FT_BBox    bbox;

  } TBBox_Rec;


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Move_To( FT_Vector*  to,
                TBBox_Rec*  user )
  {
    user->last = *to;

    return 0;
  }


#define CHECK_X( p, bbox )  \
          ( p->x < bbox.xMin || p->x > bbox.xMax )

#define CHECK_Y( p, bbox )  \
          ( p->y < bbox.yMin || p->y > bbox.yMax )


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  BBox_Conic_Check( FT_Pos   y1,
                    FT_Pos   y2,
                    FT_Pos   y3,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    if ( y1 <= y3 && y2 == y1 )     
      goto Suite;

    if ( y1 < y3 )
    {
      if ( y2 >= y1 && y2 <= y3 )   
        goto Suite;
    }
    else
    {
      if ( y2 >= y3 && y2 <= y1 )   
      {
        y2 = y1;
        y1 = y3;
        y3 = y2;
        goto Suite;
      }
    }

    y1 = y3 = y1 - FT_MulDiv( y2 - y1, y2 - y1, y1 - 2*y2 + y3 );

  Suite:
    if ( y1 < *min ) *min = y1;
    if ( y3 > *max ) *max = y3;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Conic_To( FT_Vector*  control,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    
    

    if ( CHECK_X( control, user->bbox ) )
      BBox_Conic_Check( user->last.x,
                        control->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control, user->bbox ) )
      BBox_Conic_Check( user->last.y,
                        control->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

#if 0

  static void
  BBox_Cubic_Check( FT_Pos   p1,
                    FT_Pos   p2,
                    FT_Pos   p3,
                    FT_Pos   p4,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    FT_Pos  stack[32*3 + 1], *arc;


    arc = stack;

    arc[0] = p1;
    arc[1] = p2;
    arc[2] = p3;
    arc[3] = p4;

    do
    {
      FT_Pos  y1 = arc[0];
      FT_Pos  y2 = arc[1];
      FT_Pos  y3 = arc[2];
      FT_Pos  y4 = arc[3];


      if ( y1 == y4 )
      {
        if ( y1 == y2 && y1 == y3 )                         
          goto Test;
      }
      else if ( y1 < y4 )
      {
        if ( y2 >= y1 && y2 <= y4 && y3 >= y1 && y3 <= y4 ) 
          goto Test;
      }
      else
      {
        if ( y2 >= y4 && y2 <= y1 && y3 >= y4 && y3 <= y1 ) 
        {
          y2 = y1;
          y1 = y4;
          y4 = y2;
          goto Test;
        }
      }

      
      arc[6] = y4;
      arc[1] = y1 = ( y1 + y2 ) / 2;
      arc[5] = y4 = ( y4 + y3 ) / 2;
      y2 = ( y2 + y3 ) / 2;
      arc[2] = y1 = ( y1 + y2 ) / 2;
      arc[4] = y4 = ( y4 + y2 ) / 2;
      arc[3] = ( y1 + y4 ) / 2;

      arc += 3;
      goto Suite;

   Test:
      if ( y1 < *min ) *min = y1;
      if ( y4 > *max ) *max = y4;
      arc -= 3;

    Suite:
      ;
    } while ( arc >= stack );
  }

#else

  static void
  test_cubic_extrema( FT_Pos    y1,
                      FT_Pos    y2,
                      FT_Pos    y3,
                      FT_Pos    y4,
                      FT_Fixed  u,
                      FT_Pos*   min,
                      FT_Pos*   max )
  {
 
    FT_Pos    b = y3 - 2*y2 + y1;
    FT_Pos    c = y2 - y1;
    FT_Pos    d = y1;
    FT_Pos    y;
    FT_Fixed  uu;

    FT_UNUSED ( y4 );


    
    
    
    
    
    
    
    
    
    
    
    
    

    if ( u > 0 && u < 0x10000L )
    {
      uu = FT_MulFix( u, u );
      y  = d + FT_MulFix( c, 2*u ) + FT_MulFix( b, uu );

      if ( y < *min ) *min = y;
      if ( y > *max ) *max = y;
    }
  }


  static void
  BBox_Cubic_Check( FT_Pos   y1,
                    FT_Pos   y2,
                    FT_Pos   y3,
                    FT_Pos   y4,
                    FT_Pos*  min,
                    FT_Pos*  max )
  {
    
    if      ( y1 < *min )  *min = y1;
    else if ( y1 > *max )  *max = y1;

    if      ( y4 < *min )  *min = y4;
    else if ( y4 > *max )  *max = y4;

    
    if ( y1 <= y4 )
    {
      
      if ( y1 <= y2 && y2 <= y4 && y1 <= y3 && y3 <= y4 )
        return;
    }
    else 
    {
      
      if ( y1 >= y2 && y2 >= y4 && y1 >= y3 && y3 >= y4 )
        return;
    }

    
    {
      FT_Pos    a = y4 - 3*y3 + 3*y2 - y1;
      FT_Pos    b = y3 - 2*y2 + y1;
      FT_Pos    c = y2 - y1;
      FT_Pos    d;
      FT_Fixed  t;


      
      
      
      
      
      
      
      
      

      {
        FT_ULong  t1, t2;
        int       shift = 0;


        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        t1  = (FT_ULong)( ( a >= 0 ) ? a : -a );
        t2  = (FT_ULong)( ( b >= 0 ) ? b : -b );
        t1 |= t2;
        t2  = (FT_ULong)( ( c >= 0 ) ? c : -c );
        t1 |= t2;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if ( t1 == 0 )  
          return;

        if ( t1 > 0x7FFFFFUL )
        {
          do
          {
            shift++;
            t1 >>= 1;

          } while ( t1 > 0x7FFFFFUL );

          
          
          a >>= shift;
          b >>= shift;
          c >>= shift;
        }
        else if ( t1 < 0x400000UL )
        {
          do
          {
            shift++;
            t1 <<= 1;

          } while ( t1 < 0x400000UL );

          a <<= shift;
          b <<= shift;
          c <<= shift;
        }
      }

      
      if ( a == 0 )
      {
        if ( b != 0 )
        {
          t = - FT_DivFix( c, b ) / 2;
          test_cubic_extrema( y1, y2, y3, y4, t, min, max );
        }
      }
      else
      {
        
        d = FT_MulFix( b, b ) - FT_MulFix( a, c );
        if ( d < 0 )
          return;

        if ( d == 0 )
        {
          
          t = - FT_DivFix( b, a );
          test_cubic_extrema( y1, y2, y3, y4, t, min, max );
        }
        else
        {
          
          d = FT_SqrtFixed( (FT_Int32)d );
          t = - FT_DivFix( b - d, a );
          test_cubic_extrema( y1, y2, y3, y4, t, min, max );

          t = - FT_DivFix( b + d, a );
          test_cubic_extrema( y1, y2, y3, y4, t, min, max );
        }
      }
    }
  }

#endif


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  BBox_Cubic_To( FT_Vector*  control1,
                 FT_Vector*  control2,
                 FT_Vector*  to,
                 TBBox_Rec*  user )
  {
    
    

    if ( CHECK_X( control1, user->bbox ) ||
         CHECK_X( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.x,
                        control1->x,
                        control2->x,
                        to->x,
                        &user->bbox.xMin,
                        &user->bbox.xMax );

    if ( CHECK_Y( control1, user->bbox ) ||
         CHECK_Y( control2, user->bbox ) )
      BBox_Cubic_Check( user->last.y,
                        control1->y,
                        control2->y,
                        to->y,
                        &user->bbox.yMin,
                        &user->bbox.yMax );

    user->last = *to;

    return 0;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Get_BBox( FT_Outline*  outline,
                       FT_BBox     *abbox )
  {
    FT_BBox     cbox;
    FT_BBox     bbox;
    FT_Vector*  vec;
    FT_UShort   n;


    if ( !abbox )
      return FT_Err_Invalid_Argument;

    if ( !outline )
      return FT_Err_Invalid_Outline;

    
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
    {
      abbox->xMin = abbox->xMax = 0;
      abbox->yMin = abbox->yMax = 0;
      return 0;
    }

    
    
    

    vec = outline->points;
    bbox.xMin = bbox.xMax = cbox.xMin = cbox.xMax = vec->x;
    bbox.yMin = bbox.yMax = cbox.yMin = cbox.yMax = vec->y;
    vec++;

    for ( n = 1; n < outline->n_points; n++ )
    {
      FT_Pos  x = vec->x;
      FT_Pos  y = vec->y;


      
      if ( x < cbox.xMin ) cbox.xMin = x;
      if ( x > cbox.xMax ) cbox.xMax = x;

      if ( y < cbox.yMin ) cbox.yMin = y;
      if ( y > cbox.yMax ) cbox.yMax = y;

      if ( FT_CURVE_TAG( outline->tags[n] ) == FT_CURVE_TAG_ON )
      {
        
        if ( x < bbox.xMin ) bbox.xMin = x;
        if ( x > bbox.xMax ) bbox.xMax = x;

        if ( y < bbox.yMin ) bbox.yMin = y;
        if ( y > bbox.yMax ) bbox.yMax = y;
      }

      vec++;
    }

    
    if ( cbox.xMin < bbox.xMin || cbox.xMax > bbox.xMax ||
         cbox.yMin < bbox.yMin || cbox.yMax > bbox.yMax )
    {
      
      

      static const FT_Outline_Funcs  bbox_interface =
      {
        (FT_Outline_MoveTo_Func) BBox_Move_To,
        (FT_Outline_LineTo_Func) BBox_Move_To,
        (FT_Outline_ConicTo_Func)BBox_Conic_To,
        (FT_Outline_CubicTo_Func)BBox_Cubic_To,
        0, 0
      };

      FT_Error   error;
      TBBox_Rec  user;


      user.bbox = bbox;

      error = FT_Outline_Decompose( outline, &bbox_interface, &user );
      if ( error )
        return error;

      *abbox = user.bbox;
    }
    else
      *abbox = bbox;

    return FT_Err_Ok;
  }



