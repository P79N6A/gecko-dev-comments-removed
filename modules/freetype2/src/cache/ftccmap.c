

















#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include "ftcmanag.h"
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_DEBUG_H
#include FT_TRUETYPE_IDS_H

#include "ftccback.h"
#include "ftcerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  typedef enum  FTC_OldCMapType_
  {
    FTC_OLD_CMAP_BY_INDEX    = 0,
    FTC_OLD_CMAP_BY_ENCODING = 1,
    FTC_OLD_CMAP_BY_ID       = 2

  } FTC_OldCMapType;


  typedef struct  FTC_OldCMapIdRec_
  {
    FT_UInt  platform;
    FT_UInt  encoding;

  } FTC_OldCMapIdRec, *FTC_OldCMapId;


  typedef struct  FTC_OldCMapDescRec_
  {
    FTC_FaceID       face_id;
    FTC_OldCMapType  type;

    union
    {
      FT_UInt           index;
      FT_Encoding       encoding;
      FTC_OldCMapIdRec  id;

    } u;

  } FTC_OldCMapDescRec, *FTC_OldCMapDesc;

#endif 


  
  
  
  
  
  
  
  
  
  
  
  


  
#define FTC_CMAP_INDICES_MAX  128

  
#define FTC_CMAP_HASH( faceid, index, charcode )           \
          ( FTC_FACE_ID_HASH( faceid ) + 211 * ( index ) + \
            ( (char_code) / FTC_CMAP_INDICES_MAX )       )

  
  typedef struct  FTC_CMapQueryRec_
  {
    FTC_FaceID  face_id;
    FT_UInt     cmap_index;
    FT_UInt32   char_code;

  } FTC_CMapQueryRec, *FTC_CMapQuery;

#define FTC_CMAP_QUERY( x )  ((FTC_CMapQuery)(x))
#define FTC_CMAP_QUERY_HASH( x )                                         \
          FTC_CMAP_HASH( (x)->face_id, (x)->cmap_index, (x)->char_code )

  
  typedef struct  FTC_CMapNodeRec_
  {
    FTC_NodeRec  node;
    FTC_FaceID   face_id;
    FT_UInt      cmap_index;
    FT_UInt32    first;                         
    FT_UInt16    indices[FTC_CMAP_INDICES_MAX]; 

  } FTC_CMapNodeRec, *FTC_CMapNode;

#define FTC_CMAP_NODE( x ) ( (FTC_CMapNode)( x ) )
#define FTC_CMAP_NODE_HASH( x )                                      \
          FTC_CMAP_HASH( (x)->face_id, (x)->cmap_index, (x)->first )

  
  
#define FTC_CMAP_UNKNOWN  ( (FT_UInt16)-1 )


  
  
  
  
  
  
  


  FT_CALLBACK_DEF( void )
  ftc_cmap_node_free( FTC_Node   ftcnode,
                      FTC_Cache  cache )
  {
    FTC_CMapNode  node   = (FTC_CMapNode)ftcnode;
    FT_Memory     memory = cache->memory;


    FT_FREE( node );
  }


  
  FT_CALLBACK_DEF( FT_Error )
  ftc_cmap_node_new( FTC_Node   *ftcanode,
                     FT_Pointer  ftcquery,
                     FTC_Cache   cache )
  {
    FTC_CMapNode  *anode  = (FTC_CMapNode*)ftcanode;
    FTC_CMapQuery  query  = (FTC_CMapQuery)ftcquery;
    FT_Error       error;
    FT_Memory      memory = cache->memory;
    FTC_CMapNode   node;
    FT_UInt        nn;


    if ( !FT_NEW( node ) )
    {
      node->face_id    = query->face_id;
      node->cmap_index = query->cmap_index;
      node->first      = (query->char_code / FTC_CMAP_INDICES_MAX) *
                         FTC_CMAP_INDICES_MAX;

      for ( nn = 0; nn < FTC_CMAP_INDICES_MAX; nn++ )
        node->indices[nn] = FTC_CMAP_UNKNOWN;
    }

    *anode = node;
    return error;
  }


  
  FT_CALLBACK_DEF( FT_ULong )
  ftc_cmap_node_weight( FTC_Node   cnode,
                        FTC_Cache  cache )
  {
    FT_UNUSED( cnode );
    FT_UNUSED( cache );

    return sizeof ( *cnode );
  }


  
  FT_CALLBACK_DEF( FT_Bool )
  ftc_cmap_node_compare( FTC_Node    ftcnode,
                         FT_Pointer  ftcquery,
                         FTC_Cache   cache )
  {
    FTC_CMapNode   node  = (FTC_CMapNode)ftcnode;
    FTC_CMapQuery  query = (FTC_CMapQuery)ftcquery;
    FT_UNUSED( cache );


    if ( node->face_id    == query->face_id    &&
         node->cmap_index == query->cmap_index )
    {
      FT_UInt32  offset = (FT_UInt32)( query->char_code - node->first );


      return FT_BOOL( offset < FTC_CMAP_INDICES_MAX );
    }

    return 0;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_cmap_node_remove_faceid( FTC_Node    ftcnode,
                               FT_Pointer  ftcface_id,
                               FTC_Cache   cache )
  {
    FTC_CMapNode  node    = (FTC_CMapNode)ftcnode;
    FTC_FaceID    face_id = (FTC_FaceID)ftcface_id;
    FT_UNUSED( cache );

    return FT_BOOL( node->face_id == face_id );
  }


  
  
  
  
  
  
  


  FT_CALLBACK_TABLE_DEF
  const FTC_CacheClassRec  ftc_cmap_cache_class =
  {
    ftc_cmap_node_new,
    ftc_cmap_node_weight,
    ftc_cmap_node_compare,
    ftc_cmap_node_remove_faceid,
    ftc_cmap_node_free,

    sizeof ( FTC_CacheRec ),
    ftc_cache_init,
    ftc_cache_done,
  };


  

  FT_EXPORT_DEF( FT_Error )
  FTC_CMapCache_New( FTC_Manager     manager,
                     FTC_CMapCache  *acache )
  {
    return FTC_Manager_RegisterCache( manager,
                                      &ftc_cmap_cache_class,
                                      FTC_CACHE_P( acache ) );
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  









#endif


  

  FT_EXPORT_DEF( FT_UInt )
  FTC_CMapCache_Lookup( FTC_CMapCache  cmap_cache,
                        FTC_FaceID     face_id,
                        FT_Int         cmap_index,
                        FT_UInt32      char_code )
  {
    FTC_Cache         cache = FTC_CACHE( cmap_cache );
    FTC_CMapQueryRec  query;
    FTC_CMapNode      node;
    FT_Error          error;
    FT_UInt           gindex = 0;
    FT_UInt32         hash;


    if ( !cache )
    {
      FT_ERROR(( "FTC_CMapCache_Lookup: bad arguments, returning 0!\n" ));
      return 0;
    }

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    












    if ( cmap_index >= 16 )
    {
      FTC_OldCMapDesc  desc = (FTC_OldCMapDesc) face_id;


      char_code     = (FT_UInt32)cmap_index;
      query.face_id = desc->face_id;


      switch ( desc->type )
      {
      case FTC_OLD_CMAP_BY_INDEX:
        query.cmap_index = desc->u.index;
        query.char_code  = (FT_UInt32)cmap_index;
        break;

      case FTC_OLD_CMAP_BY_ENCODING:
        {
          FT_Face  face;


          error = FTC_Manager_LookupFace( cache->manager, desc->face_id,
                                          &face );
          if ( error )
            return 0;

          FT_Select_Charmap( face, desc->u.encoding );

          return FT_Get_Char_Index( face, char_code );
        }

      default:
        return 0;
      }
    }
    else

#endif 

    {
      query.face_id    = face_id;
      query.cmap_index = (FT_UInt)cmap_index;
      query.char_code  = char_code;
    }

    hash = FTC_CMAP_HASH( face_id, cmap_index, char_code );

#if 1
    FTC_CACHE_LOOKUP_CMP( cache, ftc_cmap_node_compare, hash, &query,
                          node, error );
#else
    error = FTC_Cache_Lookup( cache, hash, &query, (FTC_Node*) &node );
#endif
    if ( error )
      goto Exit;

    FT_ASSERT( (FT_UInt)( char_code - node->first ) < FTC_CMAP_INDICES_MAX );

    
    if ( (FT_UInt)( char_code - node->first >= FTC_CMAP_INDICES_MAX ) )
      return 0;

    gindex = node->indices[char_code - node->first];
    if ( gindex == FTC_CMAP_UNKNOWN )
    {
      FT_Face  face;


      gindex = 0;

      error = FTC_Manager_LookupFace( cache->manager, node->face_id, &face );
      if ( error )
        goto Exit;

      if ( (FT_UInt)cmap_index < (FT_UInt)face->num_charmaps )
      {
        FT_CharMap  old, cmap  = NULL;


        old  = face->charmap;
        cmap = face->charmaps[cmap_index];

        if ( old != cmap )
          FT_Set_Charmap( face, cmap );

        gindex = FT_Get_Char_Index( face, char_code );

        if ( old != cmap )
          FT_Set_Charmap( face, old );
      }

      node->indices[char_code - node->first] = (FT_UShort)gindex;
    }

  Exit:
    return gindex;
  }



