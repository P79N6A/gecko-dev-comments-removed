

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H

#include "pshrec.h"
#include "pshalgo.h"

#include "pshnterr.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshrec

#ifdef DEBUG_HINTER
  PS_Hints  ps_debug_hints         = 0;
  int       ps_debug_no_horz_hints = 0;
  int       ps_debug_no_vert_hints = 0;
#endif


  
  
  
  
  
  
  

  
  static void
  ps_hint_table_done( PS_Hint_Table  table,
                      FT_Memory      memory )
  {
    FT_FREE( table->hints );
    table->num_hints = 0;
    table->max_hints = 0;
  }


  
  static FT_Error
  ps_hint_table_ensure( PS_Hint_Table  table,
                        FT_UInt        count,
                        FT_Memory      memory )
  {
    FT_UInt   old_max = table->max_hints;
    FT_UInt   new_max = count;
    FT_Error  error   = FT_Err_Ok;


    if ( new_max > old_max )
    {
      
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( table->hints, old_max, new_max ) )
        table->max_hints = new_max;
    }
    return error;
  }


  static FT_Error
  ps_hint_table_alloc( PS_Hint_Table  table,
                       FT_Memory      memory,
                       PS_Hint       *ahint )
  {
    FT_Error  error = FT_Err_Ok;
    FT_UInt   count;
    PS_Hint   hint = 0;


    count = table->num_hints;
    count++;

    if ( count >= table->max_hints )
    {
      error = ps_hint_table_ensure( table, count, memory );
      if ( error )
        goto Exit;
    }

    hint        = table->hints + count - 1;
    hint->pos   = 0;
    hint->len   = 0;
    hint->flags = 0;

    table->num_hints = count;

  Exit:
    *ahint = hint;
    return error;
  }


  
  
  
  
  
  
  

  
  static void
  ps_mask_done( PS_Mask    mask,
                FT_Memory  memory )
  {
    FT_FREE( mask->bytes );
    mask->num_bits  = 0;
    mask->max_bits  = 0;
    mask->end_point = 0;
  }


  
  static FT_Error
  ps_mask_ensure( PS_Mask    mask,
                  FT_UInt    count,
                  FT_Memory  memory )
  {
    FT_UInt   old_max = ( mask->max_bits + 7 ) >> 3;
    FT_UInt   new_max = ( count          + 7 ) >> 3;
    FT_Error  error   = FT_Err_Ok;


    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( mask->bytes, old_max, new_max ) )
        mask->max_bits = new_max * 8;
    }
    return error;
  }


  
  static FT_Int
  ps_mask_test_bit( PS_Mask  mask,
                    FT_Int   idx )
  {
    if ( (FT_UInt)idx >= mask->num_bits )
      return 0;

    return mask->bytes[idx >> 3] & ( 0x80 >> ( idx & 7 ) );
  }


  
  static void
  ps_mask_clear_bit( PS_Mask  mask,
                     FT_Int   idx )
  {
    FT_Byte*  p;


    if ( (FT_UInt)idx >= mask->num_bits )
      return;

    p    = mask->bytes + ( idx >> 3 );
    p[0] = (FT_Byte)( p[0] & ~( 0x80 >> ( idx & 7 ) ) );
  }


  
  static FT_Error
  ps_mask_set_bit( PS_Mask    mask,
                   FT_Int     idx,
                   FT_Memory  memory )
  {
    FT_Error  error = FT_Err_Ok;
    FT_Byte*  p;


    if ( idx < 0 )
      goto Exit;

    if ( (FT_UInt)idx >= mask->num_bits )
    {
      error = ps_mask_ensure( mask, idx + 1, memory );
      if ( error )
        goto Exit;

      mask->num_bits = idx + 1;
    }

    p    = mask->bytes + ( idx >> 3 );
    p[0] = (FT_Byte)( p[0] | ( 0x80 >> ( idx & 7 ) ) );

  Exit:
    return error;
  }


  
  static void
  ps_mask_table_done( PS_Mask_Table  table,
                      FT_Memory      memory )
  {
    FT_UInt  count = table->max_masks;
    PS_Mask  mask  = table->masks;


    for ( ; count > 0; count--, mask++ )
      ps_mask_done( mask, memory );

    FT_FREE( table->masks );
    table->num_masks = 0;
    table->max_masks = 0;
  }


  
  static FT_Error
  ps_mask_table_ensure( PS_Mask_Table  table,
                        FT_UInt        count,
                        FT_Memory      memory )
  {
    FT_UInt   old_max = table->max_masks;
    FT_UInt   new_max = count;
    FT_Error  error   = FT_Err_Ok;


    if ( new_max > old_max )
    {
      new_max = FT_PAD_CEIL( new_max, 8 );
      if ( !FT_RENEW_ARRAY( table->masks, old_max, new_max ) )
        table->max_masks = new_max;
    }
    return error;
  }


  
  static FT_Error
  ps_mask_table_alloc( PS_Mask_Table  table,
                       FT_Memory      memory,
                       PS_Mask       *amask )
  {
    FT_UInt   count;
    FT_Error  error = FT_Err_Ok;
    PS_Mask   mask  = 0;


    count = table->num_masks;
    count++;

    if ( count > table->max_masks )
    {
      error = ps_mask_table_ensure( table, count, memory );
      if ( error )
        goto Exit;
    }

    mask             = table->masks + count - 1;
    mask->num_bits   = 0;
    mask->end_point  = 0;
    table->num_masks = count;

  Exit:
    *amask = mask;
    return error;
  }


  
  static FT_Error
  ps_mask_table_last( PS_Mask_Table  table,
                      FT_Memory      memory,
                      PS_Mask       *amask )
  {
    FT_Error  error = FT_Err_Ok;
    FT_UInt   count;
    PS_Mask   mask;


    count = table->num_masks;
    if ( count == 0 )
    {
      error = ps_mask_table_alloc( table, memory, &mask );
      if ( error )
        goto Exit;
    }
    else
      mask = table->masks + count - 1;

  Exit:
    *amask = mask;
    return error;
  }


  
  static FT_Error
  ps_mask_table_set_bits( PS_Mask_Table   table,
                          const FT_Byte*  source,
                          FT_UInt         bit_pos,
                          FT_UInt         bit_count,
                          FT_Memory       memory )
  {
    FT_Error  error;
    PS_Mask   mask;


    error = ps_mask_table_last( table, memory, &mask );
    if ( error )
      goto Exit;

    error = ps_mask_ensure( mask, bit_count, memory );
    if ( error )
      goto Exit;

    mask->num_bits = bit_count;

    
    {
      FT_Byte*  read  = (FT_Byte*)source + ( bit_pos >> 3 );
      FT_Int    rmask = 0x80 >> ( bit_pos & 7 );
      FT_Byte*  write = mask->bytes;
      FT_Int    wmask = 0x80;
      FT_Int    val;


      for ( ; bit_count > 0; bit_count-- )
      {
        val = write[0] & ~wmask;

        if ( read[0] & rmask )
          val |= wmask;

        write[0] = (FT_Byte)val;

        rmask >>= 1;
        if ( rmask == 0 )
        {
          read++;
          rmask = 0x80;
        }

        wmask >>= 1;
        if ( wmask == 0 )
        {
          write++;
          wmask = 0x80;
        }
      }
    }

  Exit:
    return error;
  }


  
  static FT_Int
  ps_mask_table_test_intersect( PS_Mask_Table  table,
                                FT_Int         index1,
                                FT_Int         index2 )
  {
    PS_Mask   mask1  = table->masks + index1;
    PS_Mask   mask2  = table->masks + index2;
    FT_Byte*  p1     = mask1->bytes;
    FT_Byte*  p2     = mask2->bytes;
    FT_UInt   count1 = mask1->num_bits;
    FT_UInt   count2 = mask2->num_bits;
    FT_UInt   count;


    count = FT_MIN( count1, count2 );
    for ( ; count >= 8; count -= 8 )
    {
      if ( p1[0] & p2[0] )
        return 1;

      p1++;
      p2++;
    }

    if ( count == 0 )
      return 0;

    return ( p1[0] & p2[0] ) & ~( 0xFF >> count );
  }


  
  static FT_Error
  ps_mask_table_merge( PS_Mask_Table  table,
                       FT_Int         index1,
                       FT_Int         index2,
                       FT_Memory      memory )
  {
    FT_UInt   temp;
    FT_Error  error = FT_Err_Ok;


    
    if ( index1 > index2 )
    {
      temp   = index1;
      index1 = index2;
      index2 = temp;
    }

    if ( index1 < index2 && index1 >= 0 && index2 < (FT_Int)table->num_masks )
    {
      
      
      PS_Mask  mask1  = table->masks + index1;
      PS_Mask  mask2  = table->masks + index2;
      FT_UInt  count1 = mask1->num_bits;
      FT_UInt  count2 = mask2->num_bits;
      FT_Int   delta;


      if ( count2 > 0 )
      {
        FT_UInt   pos;
        FT_Byte*  read;
        FT_Byte*  write;


        
        
        if ( count2 > count1 )
        {
          error = ps_mask_ensure( mask1, count2, memory );
          if ( error )
            goto Exit;

          for ( pos = count1; pos < count2; pos++ )
            ps_mask_clear_bit( mask1, pos );
        }

        
        read  = mask2->bytes;
        write = mask1->bytes;
        pos   = (FT_UInt)( ( count2 + 7 ) >> 3 );

        for ( ; pos > 0; pos-- )
        {
          write[0] = (FT_Byte)( write[0] | read[0] );
          write++;
          read++;
        }
      }

      
      
      mask2->num_bits  = 0;
      mask2->end_point = 0;

      delta = table->num_masks - 1 - index2; 
      if ( delta > 0 )
      {
        
        PS_MaskRec  dummy = *mask2;


        ft_memmove( mask2, mask2 + 1, delta * sizeof ( PS_MaskRec ) );

        mask2[delta] = dummy;
      }

      table->num_masks--;
    }
    else
      FT_TRACE0(( "ps_mask_table_merge: ignoring invalid indices (%d,%d)\n",
                  index1, index2 ));

  Exit:
    return error;
  }


  
  
  
  static FT_Error
  ps_mask_table_merge_all( PS_Mask_Table  table,
                           FT_Memory      memory )
  {
    FT_Int    index1, index2;
    FT_Error  error = FT_Err_Ok;


    for ( index1 = table->num_masks - 1; index1 > 0; index1-- )
    {
      for ( index2 = index1 - 1; index2 >= 0; index2-- )
      {
        if ( ps_mask_table_test_intersect( table, index1, index2 ) )
        {
          error = ps_mask_table_merge( table, index2, index1, memory );
          if ( error )
            goto Exit;

          break;
        }
      }
    }

  Exit:
    return error;
  }


  
  
  
  
  
  
  


  
  static void
  ps_dimension_done( PS_Dimension  dimension,
                     FT_Memory     memory )
  {
    ps_mask_table_done( &dimension->counters, memory );
    ps_mask_table_done( &dimension->masks,    memory );
    ps_hint_table_done( &dimension->hints,    memory );
  }


  
  static void
  ps_dimension_init( PS_Dimension  dimension )
  {
    dimension->hints.num_hints    = 0;
    dimension->masks.num_masks    = 0;
    dimension->counters.num_masks = 0;
  }


#if 0

  
  static FT_Error
  ps_dimension_set_mask_bit( PS_Dimension  dim,
                             FT_UInt       idx,
                             FT_Memory     memory )
  {
    PS_Mask   mask;
    FT_Error  error = FT_Err_Ok;


    
    error = ps_mask_table_last( &dim->masks, memory, &mask );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( mask, idx, memory );

  Exit:
    return error;
  }

#endif

  
  static void
  ps_dimension_end_mask( PS_Dimension  dim,
                         FT_UInt       end_point )
  {
    FT_UInt  count = dim->masks.num_masks;


    if ( count > 0 )
    {
      PS_Mask  mask = dim->masks.masks + count - 1;


      mask->end_point = end_point;
    }
  }


  
  
  static FT_Error
  ps_dimension_reset_mask( PS_Dimension  dim,
                           FT_UInt       end_point,
                           FT_Memory     memory )
  {
    PS_Mask  mask;


    
    ps_dimension_end_mask( dim, end_point );

    
    return ps_mask_table_alloc( &dim->masks, memory, &mask );
  }


  
  static FT_Error
  ps_dimension_set_mask_bits( PS_Dimension    dim,
                              const FT_Byte*  source,
                              FT_UInt         source_pos,
                              FT_UInt         source_bits,
                              FT_UInt         end_point,
                              FT_Memory       memory )
  {
    FT_Error  error;


    
    error = ps_dimension_reset_mask( dim, end_point, memory );
    if ( error )
      goto Exit;

    
    error = ps_mask_table_set_bits( &dim->masks, source,
                                    source_pos, source_bits, memory );

  Exit:
    return error;
  }


  
  static FT_Error
  ps_dimension_add_t1stem( PS_Dimension  dim,
                           FT_Int        pos,
                           FT_Int        len,
                           FT_Memory     memory,
                           FT_Int       *aindex )
  {
    FT_Error  error = FT_Err_Ok;
    FT_UInt   flags = 0;


    
    if ( len < 0 )
    {
      flags |= PS_HINT_FLAG_GHOST;
      if ( len == -21 )
      {
        flags |= PS_HINT_FLAG_BOTTOM;
        pos   += len;
      }
      len = 0;
    }

    if ( aindex )
      *aindex = -1;

    
    {
      PS_Mask  mask;
      FT_UInt  idx;
      FT_UInt  max   = dim->hints.num_hints;
      PS_Hint  hint  = dim->hints.hints;


      for ( idx = 0; idx < max; idx++, hint++ )
      {
        if ( hint->pos == pos && hint->len == len )
          break;
      }

      
      if ( idx >= max )
      {
        error = ps_hint_table_alloc( &dim->hints, memory, &hint );
        if ( error )
          goto Exit;

        hint->pos   = pos;
        hint->len   = len;
        hint->flags = flags;
      }

      
      error = ps_mask_table_last( &dim->masks, memory, &mask );
      if ( error )
        goto Exit;

      error = ps_mask_set_bit( mask, idx, memory );
      if ( error )
        goto Exit;

      if ( aindex )
        *aindex = (FT_Int)idx;
    }

  Exit:
    return error;
  }


  
  static FT_Error
  ps_dimension_add_counter( PS_Dimension  dim,
                            FT_Int        hint1,
                            FT_Int        hint2,
                            FT_Int        hint3,
                            FT_Memory     memory )
  {
    FT_Error  error   = FT_Err_Ok;
    FT_UInt   count   = dim->counters.num_masks;
    PS_Mask   counter = dim->counters.masks;


    
    
    for ( ; count > 0; count--, counter++ )
    {
      if ( ps_mask_test_bit( counter, hint1 ) ||
           ps_mask_test_bit( counter, hint2 ) ||
           ps_mask_test_bit( counter, hint3 ) )
        break;
    }

    
    if ( count == 0 )
    {
      error = ps_mask_table_alloc( &dim->counters, memory, &counter );
      if ( error )
        goto Exit;
    }

    
    error = ps_mask_set_bit( counter, hint1, memory );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( counter, hint2, memory );
    if ( error )
      goto Exit;

    error = ps_mask_set_bit( counter, hint3, memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  
  static FT_Error
  ps_dimension_end( PS_Dimension  dim,
                    FT_UInt       end_point,
                    FT_Memory     memory )
  {
    
    ps_dimension_end_mask( dim, end_point );

    
    return ps_mask_table_merge_all( &dim->counters, memory );
  }


  
  
  
  
  
  
  


  
  FT_LOCAL( void )
  ps_hints_done( PS_Hints  hints )
  {
    FT_Memory  memory = hints->memory;


    ps_dimension_done( &hints->dimension[0], memory );
    ps_dimension_done( &hints->dimension[1], memory );

    hints->error  = FT_Err_Ok;
    hints->memory = 0;
  }


  FT_LOCAL( FT_Error )
  ps_hints_init( PS_Hints   hints,
                 FT_Memory  memory )
  {
    FT_MEM_ZERO( hints, sizeof ( *hints ) );
    hints->memory = memory;
    return FT_Err_Ok;
  }


  
  static void
  ps_hints_open( PS_Hints      hints,
                 PS_Hint_Type  hint_type )
  {
    switch ( hint_type )
    {
    case PS_HINT_TYPE_1:
    case PS_HINT_TYPE_2:
      hints->error     = FT_Err_Ok;
      hints->hint_type = hint_type;

      ps_dimension_init( &hints->dimension[0] );
      ps_dimension_init( &hints->dimension[1] );
      break;

    default:
      hints->error     = FT_THROW( Invalid_Argument );
      hints->hint_type = hint_type;

      FT_TRACE0(( "ps_hints_open: invalid charstring type\n" ));
      break;
    }
  }


  
  static void
  ps_hints_stem( PS_Hints  hints,
                 FT_Int    dimension,
                 FT_UInt   count,
                 FT_Long*  stems )
  {
    if ( !hints->error )
    {
      
      if ( dimension < 0 || dimension > 1 )
      {
        FT_TRACE0(( "ps_hints_stem: invalid dimension (%d) used\n",
                    dimension ));
        dimension = ( dimension != 0 );
      }

      
      switch ( hints->hint_type )
      {
      case PS_HINT_TYPE_1:  
      case PS_HINT_TYPE_2:  
        {
          PS_Dimension  dim = &hints->dimension[dimension];


          for ( ; count > 0; count--, stems += 2 )
          {
            FT_Error   error;
            FT_Memory  memory = hints->memory;


            error = ps_dimension_add_t1stem(
                      dim, (FT_Int)stems[0], (FT_Int)stems[1],
                      memory, NULL );
            if ( error )
            {
              FT_ERROR(( "ps_hints_stem: could not add stem"
                         " (%d,%d) to hints table\n", stems[0], stems[1] ));

              hints->error = error;
              return;
            }
          }
          break;
        }

      default:
        FT_TRACE0(( "ps_hints_stem: called with invalid hint type (%d)\n",
                    hints->hint_type ));
        break;
      }
    }
  }


  
  static void
  ps_hints_t1stem3( PS_Hints   hints,
                    FT_Int     dimension,
                    FT_Fixed*  stems )
  {
    FT_Error  error = FT_Err_Ok;


    if ( !hints->error )
    {
      PS_Dimension  dim;
      FT_Memory     memory = hints->memory;
      FT_Int        count;
      FT_Int        idx[3];


      
      if ( dimension < 0 || dimension > 1 )
      {
        FT_TRACE0(( "ps_hints_t1stem3: invalid dimension (%d) used\n",
                    dimension ));
        dimension = ( dimension != 0 );
      }

      dim = &hints->dimension[dimension];

      
      if ( hints->hint_type == PS_HINT_TYPE_1 )
      {
        
        for ( count = 0; count < 3; count++, stems += 2 )
        {
          error = ps_dimension_add_t1stem( dim,
                                           (FT_Int)FIXED_TO_INT( stems[0] ),
                                           (FT_Int)FIXED_TO_INT( stems[1] ),
                                           memory, &idx[count] );
          if ( error )
            goto Fail;
        }

        
        error = ps_dimension_add_counter( dim, idx[0], idx[1], idx[2],
                                          memory );
        if ( error )
          goto Fail;
      }
      else
      {
        FT_ERROR(( "ps_hints_t1stem3: called with invalid hint type\n" ));
        error = FT_THROW( Invalid_Argument );
        goto Fail;
      }
    }

    return;

  Fail:
    FT_ERROR(( "ps_hints_t1stem3: could not add counter stems to table\n" ));
    hints->error = error;
  }


  
  static void
  ps_hints_t1reset( PS_Hints  hints,
                    FT_UInt   end_point )
  {
    FT_Error  error = FT_Err_Ok;


    if ( !hints->error )
    {
      FT_Memory  memory = hints->memory;


      if ( hints->hint_type == PS_HINT_TYPE_1 )
      {
        error = ps_dimension_reset_mask( &hints->dimension[0],
                                         end_point, memory );
        if ( error )
          goto Fail;

        error = ps_dimension_reset_mask( &hints->dimension[1],
                                         end_point, memory );
        if ( error )
          goto Fail;
      }
      else
      {
        
        error = FT_THROW( Invalid_Argument );
        goto Fail;
      }
    }
    return;

  Fail:
    hints->error = error;
  }


  
  static void
  ps_hints_t2mask( PS_Hints        hints,
                   FT_UInt         end_point,
                   FT_UInt         bit_count,
                   const FT_Byte*  bytes )
  {
    FT_Error  error;


    if ( !hints->error )
    {
      PS_Dimension  dim    = hints->dimension;
      FT_Memory     memory = hints->memory;
      FT_UInt       count1 = dim[0].hints.num_hints;
      FT_UInt       count2 = dim[1].hints.num_hints;


      
      if ( bit_count !=  count1 + count2 )
      {
        FT_TRACE0(( "ps_hints_t2mask:"
                    " called with invalid bitcount %d (instead of %d)\n",
                   bit_count, count1 + count2 ));

        
        return;
      }

      
      error = ps_dimension_set_mask_bits( &dim[0], bytes, count2, count1,
                                          end_point, memory );
      if ( error )
        goto Fail;

      error = ps_dimension_set_mask_bits( &dim[1], bytes, 0, count2,
                                          end_point, memory );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    hints->error = error;
  }


  static void
  ps_hints_t2counter( PS_Hints        hints,
                      FT_UInt         bit_count,
                      const FT_Byte*  bytes )
  {
    FT_Error  error;


    if ( !hints->error )
    {
      PS_Dimension  dim    = hints->dimension;
      FT_Memory     memory = hints->memory;
      FT_UInt       count1 = dim[0].hints.num_hints;
      FT_UInt       count2 = dim[1].hints.num_hints;


      
      if ( bit_count !=  count1 + count2 )
      {
        FT_TRACE0(( "ps_hints_t2counter:"
                    " called with invalid bitcount %d (instead of %d)\n",
                   bit_count, count1 + count2 ));

        
        return;
      }

      
      error = ps_dimension_set_mask_bits( &dim[0], bytes, 0, count1,
                                          0, memory );
      if ( error )
        goto Fail;

      error = ps_dimension_set_mask_bits( &dim[1], bytes, count1, count2,
                                          0, memory );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    hints->error = error;
  }


  
  static FT_Error
  ps_hints_close( PS_Hints  hints,
                  FT_UInt   end_point )
  {
    FT_Error  error;


    error = hints->error;
    if ( !error )
    {
      FT_Memory     memory = hints->memory;
      PS_Dimension  dim    = hints->dimension;


      error = ps_dimension_end( &dim[0], end_point, memory );
      if ( !error )
      {
        error = ps_dimension_end( &dim[1], end_point, memory );
      }
    }

#ifdef DEBUG_HINTER
    if ( !error )
      ps_debug_hints = hints;
#endif
    return error;
  }


  
  
  
  
  
  
  

  static void
  t1_hints_open( T1_Hints  hints )
  {
    ps_hints_open( (PS_Hints)hints, PS_HINT_TYPE_1 );
  }

  static void
  t1_hints_stem( T1_Hints   hints,
                 FT_Int     dimension,
                 FT_Fixed*  coords )
  {
    FT_Pos  stems[2];


    stems[0] = FIXED_TO_INT( coords[0] );
    stems[1] = FIXED_TO_INT( coords[1] );

    ps_hints_stem( (PS_Hints)hints, dimension, 1, stems );
  }


  FT_LOCAL_DEF( void )
  t1_hints_funcs_init( T1_Hints_FuncsRec*  funcs )
  {
    FT_MEM_ZERO( (char*)funcs, sizeof ( *funcs ) );

    funcs->open  = (T1_Hints_OpenFunc)    t1_hints_open;
    funcs->close = (T1_Hints_CloseFunc)   ps_hints_close;
    funcs->stem  = (T1_Hints_SetStemFunc) t1_hints_stem;
    funcs->stem3 = (T1_Hints_SetStem3Func)ps_hints_t1stem3;
    funcs->reset = (T1_Hints_ResetFunc)   ps_hints_t1reset;
    funcs->apply = (T1_Hints_ApplyFunc)   ps_hints_apply;
  }


  
  
  
  
  
  
  

  static void
  t2_hints_open( T2_Hints  hints )
  {
    ps_hints_open( (PS_Hints)hints, PS_HINT_TYPE_2 );
  }


  static void
  t2_hints_stems( T2_Hints   hints,
                  FT_Int     dimension,
                  FT_Int     count,
                  FT_Fixed*  coords )
  {
    FT_Pos  stems[32], y, n;
    FT_Int  total = count;


    y = 0;
    while ( total > 0 )
    {
      
      count = total;
      if ( count > 16 )
        count = 16;

      
      for ( n = 0; n < count * 2; n++ )
      {
        y       += coords[n];
        stems[n] = FIXED_TO_INT( y );
      }

      
      for ( n = 0; n < count * 2; n += 2 )
        stems[n + 1] = stems[n + 1] - stems[n];

      
      ps_hints_stem( (PS_Hints)hints, dimension, count, stems );

      total -= count;
    }
  }


  FT_LOCAL_DEF( void )
  t2_hints_funcs_init( T2_Hints_FuncsRec*  funcs )
  {
    FT_MEM_ZERO( funcs, sizeof ( *funcs ) );

    funcs->open    = (T2_Hints_OpenFunc)   t2_hints_open;
    funcs->close   = (T2_Hints_CloseFunc)  ps_hints_close;
    funcs->stems   = (T2_Hints_StemsFunc)  t2_hints_stems;
    funcs->hintmask= (T2_Hints_MaskFunc)   ps_hints_t2mask;
    funcs->counter = (T2_Hints_CounterFunc)ps_hints_t2counter;
    funcs->apply   = (T2_Hints_ApplyFunc)  ps_hints_apply;
  }



