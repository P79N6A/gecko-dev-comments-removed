

















#include <ft2build.h>
#include FT_CACHE_H
#include "ftcsbits.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_ERRORS_H

#include "ftccback.h"
#include "ftcerror.h"


  
  
  
  
  
  
  


  static FT_Error
  ftc_sbit_copy_bitmap( FTC_SBit    sbit,
                        FT_Bitmap*  bitmap,
                        FT_Memory   memory )
  {
    FT_Error  error;
    FT_Int    pitch = bitmap->pitch;
    FT_ULong  size;


    if ( pitch < 0 )
      pitch = -pitch;

    size = (FT_ULong)( pitch * bitmap->rows );

    if ( !FT_ALLOC( sbit->buffer, size ) )
      FT_MEM_COPY( sbit->buffer, bitmap->buffer, size );

    return error;
  }


  FT_LOCAL_DEF( void )
  ftc_snode_free( FTC_Node   ftcsnode,
                  FTC_Cache  cache )
  {
    FTC_SNode  snode  = (FTC_SNode)ftcsnode;
    FTC_SBit   sbit   = snode->sbits;
    FT_UInt    count  = snode->count;
    FT_Memory  memory = cache->memory;


    for ( ; count > 0; sbit++, count-- )
      FT_FREE( sbit->buffer );

    FTC_GNode_Done( FTC_GNODE( snode ), cache );

    FT_FREE( snode );
  }


  FT_LOCAL_DEF( void )
  FTC_SNode_Free( FTC_SNode  snode,
                  FTC_Cache  cache )
  {
    ftc_snode_free( FTC_NODE( snode ), cache );
  }


  









  static FT_Error
  ftc_snode_load( FTC_SNode    snode,
                  FTC_Manager  manager,
                  FT_UInt      gindex,
                  FT_ULong    *asize )
  {
    FT_Error          error;
    FTC_GNode         gnode  = FTC_GNODE( snode );
    FTC_Family        family = gnode->family;
    FT_Memory         memory = manager->memory;
    FT_Face           face;
    FTC_SBit          sbit;
    FTC_SFamilyClass  clazz;


    if ( (FT_UInt)(gindex - gnode->gindex) >= snode->count )
    {
      FT_ERROR(( "ftc_snode_load: invalid glyph index" ));
      return FTC_Err_Invalid_Argument;
    }

    sbit  = snode->sbits + ( gindex - gnode->gindex );
    clazz = (FTC_SFamilyClass)family->clazz;

    sbit->buffer = 0;

    error = clazz->family_load_glyph( family, gindex, manager, &face );
    if ( error )
      goto BadGlyph;

    {
      FT_Int        temp;
      FT_GlyphSlot  slot   = face->glyph;
      FT_Bitmap*    bitmap = &slot->bitmap;
      FT_Int        xadvance, yadvance;


      if ( slot->format != FT_GLYPH_FORMAT_BITMAP )
      {
        FT_ERROR(( "%s: glyph loaded didn't return a bitmap!\n",
                   "ftc_snode_load" ));
        goto BadGlyph;
      }

      
      
      

#define CHECK_CHAR( d )  ( temp = (FT_Char)d, temp == d )
#define CHECK_BYTE( d )  ( temp = (FT_Byte)d, temp == d )

      
      xadvance = ( slot->advance.x + 32 ) >> 6;
      yadvance = ( slot->advance.y + 32 ) >> 6;

      if ( !CHECK_BYTE( bitmap->rows  )     ||
           !CHECK_BYTE( bitmap->width )     ||
           !CHECK_CHAR( bitmap->pitch )     ||
           !CHECK_CHAR( slot->bitmap_left ) ||
           !CHECK_CHAR( slot->bitmap_top  ) ||
           !CHECK_CHAR( xadvance )          ||
           !CHECK_CHAR( yadvance )          )
        goto BadGlyph;

      sbit->width     = (FT_Byte)bitmap->width;
      sbit->height    = (FT_Byte)bitmap->rows;
      sbit->pitch     = (FT_Char)bitmap->pitch;
      sbit->left      = (FT_Char)slot->bitmap_left;
      sbit->top       = (FT_Char)slot->bitmap_top;
      sbit->xadvance  = (FT_Char)xadvance;
      sbit->yadvance  = (FT_Char)yadvance;
      sbit->format    = (FT_Byte)bitmap->pixel_mode;
      sbit->max_grays = (FT_Byte)(bitmap->num_grays - 1);

      
      error = ftc_sbit_copy_bitmap( sbit, bitmap, memory );

      
      if ( asize )
        *asize = FT_ABS( sbit->pitch ) * sbit->height;

    } 

    
    
    
    
    if ( error && error != FTC_Err_Out_Of_Memory )
    {
    BadGlyph:
      sbit->width  = 255;
      sbit->height = 0;
      sbit->buffer = NULL;
      error        = 0;
      if ( asize )
        *asize = 0;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  FTC_SNode_New( FTC_SNode  *psnode,
                 FTC_GQuery  gquery,
                 FTC_Cache   cache )
  {
    FT_Memory   memory = cache->memory;
    FT_Error    error;
    FTC_SNode   snode  = NULL;
    FT_UInt     gindex = gquery->gindex;
    FTC_Family  family = gquery->family;

    FTC_SFamilyClass  clazz = FTC_CACHE__SFAMILY_CLASS( cache );
    FT_UInt           total;


    total = clazz->family_get_count( family, cache->manager );
    if ( total == 0 || gindex >= total )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    if ( !FT_NEW( snode ) )
    {
      FT_UInt  count, start;


      start = gindex - ( gindex % FTC_SBIT_ITEMS_PER_NODE );
      count = total - start;
      if ( count > FTC_SBIT_ITEMS_PER_NODE )
        count = FTC_SBIT_ITEMS_PER_NODE;

      FTC_GNode_Init( FTC_GNODE( snode ), start, family );

      snode->count = count;

      error = ftc_snode_load( snode,
                              cache->manager,
                              gindex,
                              NULL );
      if ( error )
      {
        FTC_SNode_Free( snode, cache );
        snode = NULL;
      }
    }

  Exit:
    *psnode = snode;
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_snode_new( FTC_Node   *ftcpsnode,
                 FT_Pointer  ftcgquery,
                 FTC_Cache   cache )
  {
    FTC_SNode  *psnode = (FTC_SNode*)ftcpsnode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;


    return FTC_SNode_New( psnode, gquery, cache );
  }


  FT_LOCAL_DEF( FT_ULong )
  ftc_snode_weight( FTC_Node   ftcsnode,
                    FTC_Cache  cache )
  {
    FTC_SNode  snode = (FTC_SNode)ftcsnode;
    FT_UInt    count = snode->count;
    FTC_SBit   sbit  = snode->sbits;
    FT_Int     pitch;
    FT_ULong   size;

    FT_UNUSED( cache );


    FT_ASSERT( snode->count <= FTC_SBIT_ITEMS_PER_NODE );

    
    size = sizeof ( *snode );

    for ( ; count > 0; count--, sbit++ )
    {
      if ( sbit->buffer )
      {
        pitch = sbit->pitch;
        if ( pitch < 0 )
          pitch = -pitch;

        
        size += pitch * sbit->height;
      }
    }

    return size;
  }


#if 0

  FT_LOCAL_DEF( FT_ULong )
  FTC_SNode_Weight( FTC_SNode  snode )
  {
    return ftc_snode_weight( FTC_NODE( snode ), NULL );
  }

#endif 


  FT_LOCAL_DEF( FT_Bool )
  ftc_snode_compare( FTC_Node    ftcsnode,
                     FT_Pointer  ftcgquery,
                     FTC_Cache   cache )
  {
    FTC_SNode   snode  = (FTC_SNode)ftcsnode;
    FTC_GQuery  gquery = (FTC_GQuery)ftcgquery;
    FTC_GNode   gnode  = FTC_GNODE( snode );
    FT_UInt     gindex = gquery->gindex;
    FT_Bool     result;


    result = FT_BOOL( gnode->family == gquery->family                    &&
                      (FT_UInt)( gindex - gnode->gindex ) < snode->count );
    if ( result )
    {
      
      FTC_SBit  sbit = snode->sbits + ( gindex - gnode->gindex );


      































      if ( sbit->buffer == NULL && sbit->width != 255 )
      {
        FT_ULong  size;
        FT_Error  error;


        ftcsnode->ref_count++;  
                                

        FTC_CACHE_TRYLOOP( cache )
        {
          error = ftc_snode_load( snode, cache->manager, gindex, &size );
        }
        FTC_CACHE_TRYLOOP_END();

        ftcsnode->ref_count--;  

        if ( error )
          result = 0;
        else
          cache->manager->cur_weight += size;
      }
    }

    return result;
  }


  FT_LOCAL_DEF( FT_Bool )
  FTC_SNode_Compare( FTC_SNode   snode,
                     FTC_GQuery  gquery,
                     FTC_Cache   cache )
  {
    return ftc_snode_compare( FTC_NODE( snode ), gquery, cache );
  }



