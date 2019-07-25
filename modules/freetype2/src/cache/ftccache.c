


















#include <ft2build.h>
#include "ftcmanag.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftccback.h"
#include "ftcerror.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache


#define FTC_HASH_MAX_LOAD  2
#define FTC_HASH_MIN_LOAD  1
#define FTC_HASH_SUB_LOAD  ( FTC_HASH_MAX_LOAD - FTC_HASH_MIN_LOAD )

  
#define FTC_HASH_INITIAL_SIZE  8


  
  
  
  
  
  
  

  
  static void
  ftc_node_mru_link( FTC_Node     node,
                     FTC_Manager  manager )
  {
    void  *nl = &manager->nodes_list;


    FTC_MruNode_Prepend( (FTC_MruNode*)nl,
                         (FTC_MruNode)node );
    manager->num_nodes++;
  }


  
  static void
  ftc_node_mru_unlink( FTC_Node     node,
                       FTC_Manager  manager )
  {
    void  *nl = &manager->nodes_list;


    FTC_MruNode_Remove( (FTC_MruNode*)nl,
                        (FTC_MruNode)node );
    manager->num_nodes--;
  }


#ifndef FTC_INLINE

  
  static void
  ftc_node_mru_up( FTC_Node     node,
                   FTC_Manager  manager )
  {
    FTC_MruNode_Up( (FTC_MruNode*)&manager->nodes_list,
                    (FTC_MruNode)node );
  }


  


  FT_LOCAL_DEF( FTC_Node* )
  ftc_get_top_node_for_hash( FTC_Cache   cache,
                             FT_PtrDist  hash )
  {
    FTC_Node*  pnode;
    FT_UInt    idx;


    idx = (FT_UInt)( hash & cache->mask );
    if ( idx < cache->p )
      idx = (FT_UInt)( hash & ( 2 * cache->mask + 1 ) );
    pnode = cache->buckets + idx;
    return pnode;
  }

#endif 


  



  static void
  ftc_cache_resize( FTC_Cache  cache )
  {
    for (;;)
    {
      FTC_Node  node, *pnode;
      FT_UFast  p     = cache->p;
      FT_UFast  mask  = cache->mask;
      FT_UFast  count = mask + p + 1;    


      
      if ( cache->slack < 0 )
      {
        FTC_Node  new_list = NULL;


        


        if ( p >= mask )
        {
          FT_Memory  memory = cache->memory;
          FT_Error   error;


          
          if ( FT_RENEW_ARRAY( cache->buckets,
                               ( mask + 1 ) * 2, ( mask + 1 ) * 4 ) )
            break;
        }

        
        pnode = cache->buckets + p;

        for (;;)
        {
          node = *pnode;
          if ( node == NULL )
            break;

          if ( node->hash & ( mask + 1 ) )
          {
            *pnode     = node->link;
            node->link = new_list;
            new_list   = node;
          }
          else
            pnode = &node->link;
        }

        cache->buckets[p + mask + 1] = new_list;

        cache->slack += FTC_HASH_MAX_LOAD;

        if ( p >= mask )
        {
          cache->mask = 2 * mask + 1;
          cache->p    = 0;
        }
        else
          cache->p = p + 1;
      }

      
      else if ( cache->slack > (FT_Long)count * FTC_HASH_SUB_LOAD )
      {
        FT_UFast   old_index = p + mask;
        FTC_Node*  pold;


        if ( old_index + 1 <= FTC_HASH_INITIAL_SIZE )
          break;

        if ( p == 0 )
        {
          FT_Memory  memory = cache->memory;
          FT_Error   error;


          
          if ( FT_RENEW_ARRAY( cache->buckets,
                               ( mask + 1 ) * 2, mask + 1 ) )
            break;

          cache->mask >>= 1;
          p             = cache->mask;
        }
        else
          p--;

        pnode = cache->buckets + p;
        while ( *pnode )
          pnode = &(*pnode)->link;

        pold   = cache->buckets + old_index;
        *pnode = *pold;
        *pold  = NULL;

        cache->slack -= FTC_HASH_MAX_LOAD;
        cache->p      = p;
      }

      
      else
        break;
    }
  }


  
  static void
  ftc_node_hash_unlink( FTC_Node   node0,
                        FTC_Cache  cache )
  {
    FTC_Node  *pnode = FTC_NODE__TOP_FOR_HASH( cache, node0->hash );


    for (;;)
    {
      FTC_Node  node = *pnode;


      if ( node == NULL )
      {
        FT_TRACE0(( "ftc_node_hash_unlink: unknown node\n" ));
        return;
      }

      if ( node == node0 )
        break;

      pnode = &(*pnode)->link;
    }

    *pnode      = node0->link;
    node0->link = NULL;

    cache->slack++;
    ftc_cache_resize( cache );
  }


  
  static void
  ftc_node_hash_link( FTC_Node   node,
                      FTC_Cache  cache )
  {
    FTC_Node  *pnode = FTC_NODE__TOP_FOR_HASH( cache, node->hash );


    node->link = *pnode;
    *pnode     = node;

    cache->slack--;
    ftc_cache_resize( cache );
  }


  
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
  FT_BASE_DEF( void )
#else
  FT_LOCAL_DEF( void )
#endif
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager )
  {
    FTC_Cache  cache;


#ifdef FT_DEBUG_ERROR
    
    if ( node->cache_index >= manager->num_caches )
    {
      FT_TRACE0(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    cache = manager->caches[node->cache_index];

#ifdef FT_DEBUG_ERROR
    if ( cache == NULL )
    {
      FT_TRACE0(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    manager->cur_weight -= cache->clazz.node_weight( node, cache );

    
    ftc_node_mru_unlink( node, manager );

    
    ftc_node_hash_unlink( node, cache );

    
    cache->clazz.node_free( node, cache );

#if 0
    
    if ( manager->num_nodes == 0 )
      FT_TRACE0(( "ftc_node_destroy: invalid cache node count (%d)\n",
                  manager->num_nodes ));
#endif
  }


  
  
  
  
  
  
  


  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_Init( FTC_Cache  cache )
  {
    return ftc_cache_init( cache );
  }


  FT_LOCAL_DEF( FT_Error )
  ftc_cache_init( FTC_Cache  cache )
  {
    FT_Memory  memory = cache->memory;
    FT_Error   error;


    cache->p     = 0;
    cache->mask  = FTC_HASH_INITIAL_SIZE - 1;
    cache->slack = FTC_HASH_INITIAL_SIZE * FTC_HASH_MAX_LOAD;

    (void)FT_NEW_ARRAY( cache->buckets, FTC_HASH_INITIAL_SIZE * 2 );
    return error;
  }


  static void
  FTC_Cache_Clear( FTC_Cache  cache )
  {
    if ( cache && cache->buckets )
    {
      FTC_Manager  manager = cache->manager;
      FT_UFast     i;
      FT_UFast     count;


      count = cache->p + cache->mask + 1;

      for ( i = 0; i < count; i++ )
      {
        FTC_Node  *pnode = cache->buckets + i, next, node = *pnode;


        while ( node )
        {
          next        = node->link;
          node->link  = NULL;

          
          ftc_node_mru_unlink( node, manager );

          
          manager->cur_weight -= cache->clazz.node_weight( node, cache );

          cache->clazz.node_free( node, cache );
          node = next;
        }
        cache->buckets[i] = NULL;
      }
      ftc_cache_resize( cache );
    }
  }


  FT_LOCAL_DEF( void )
  ftc_cache_done( FTC_Cache  cache )
  {
    if ( cache->memory )
    {
      FT_Memory  memory = cache->memory;


      FTC_Cache_Clear( cache );

      FT_FREE( cache->buckets );
      cache->mask  = 0;
      cache->p     = 0;
      cache->slack = 0;

      cache->memory = NULL;
    }
  }


  FT_LOCAL_DEF( void )
  FTC_Cache_Done( FTC_Cache  cache )
  {
    ftc_cache_done( cache );
  }


  static void
  ftc_cache_add( FTC_Cache  cache,
                 FT_PtrDist hash,
                 FTC_Node   node )
  {
    node->hash        = hash;
    node->cache_index = (FT_UInt16)cache->index;
    node->ref_count   = 0;

    ftc_node_hash_link( node, cache );
    ftc_node_mru_link( node, cache->manager );

    {
      FTC_Manager  manager = cache->manager;


      manager->cur_weight += cache->clazz.node_weight( node, cache );

      if ( manager->cur_weight >= manager->max_weight )
      {
        node->ref_count++;
        FTC_Manager_Compress( manager );
        node->ref_count--;
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_NewNode( FTC_Cache   cache,
                     FT_PtrDist  hash,
                     FT_Pointer  query,
                     FTC_Node   *anode )
  {
    FT_Error  error;
    FTC_Node  node;


    





    FTC_CACHE_TRYLOOP( cache )
    {
      error = cache->clazz.node_new( &node, query, cache );
    }
    FTC_CACHE_TRYLOOP_END( NULL );

    if ( error )
      node = NULL;
    else
    {
     


      ftc_cache_add( cache, hash, node );
    }

    *anode = node;
    return error;
  }


#ifndef FTC_INLINE

  FT_LOCAL_DEF( FT_Error )
  FTC_Cache_Lookup( FTC_Cache   cache,
                    FT_PtrDist  hash,
                    FT_Pointer  query,
                    FTC_Node   *anode )
  {
    FTC_Node*  bucket;
    FTC_Node*  pnode;
    FTC_Node   node;
    FT_Error   error        = FTC_Err_Ok;
    FT_Bool    list_changed = FALSE;

    FTC_Node_CompareFunc  compare = cache->clazz.node_compare;


    if ( cache == NULL || anode == NULL )
      return FTC_Err_Invalid_Argument;

    
    bucket = pnode = FTC_NODE__TOP_FOR_HASH( cache, hash );

    
    
    for (;;)
    {
      node = *pnode;
      if ( node == NULL )
        goto NewNode;

      if ( node->hash == hash                           &&
           compare( node, query, cache, &list_changed ) )
        break;

      pnode = &node->link;
    }

    if ( list_changed )
    {
      
      bucket = pnode = FTC_NODE__TOP_FOR_HASH( cache, hash );

      
      while ( *pnode != node )
      {
        if ( *pnode == NULL )
        {
          FT_ERROR(( "FTC_Cache_Lookup: oops!!!  node missing\n" ));
          goto NewNode;
        }
        else
          pnode = &((*pnode)->link);
      }
    }

    
    if ( node != *bucket )
    {
      *pnode     = node->link;
      node->link = *bucket;
      *bucket    = node;
    }

    
    {
      FTC_Manager  manager = cache->manager;


      if ( node != manager->nodes_list )
        ftc_node_mru_up( node, manager );
    }
    *anode = node;

    return error;

  NewNode:
    return FTC_Cache_NewNode( cache, hash, query, anode );
  }

#endif 


  FT_LOCAL_DEF( void )
  FTC_Cache_RemoveFaceID( FTC_Cache   cache,
                          FTC_FaceID  face_id )
  {
    FT_UFast     i, count;
    FTC_Manager  manager = cache->manager;
    FTC_Node     frees   = NULL;


    count = cache->p + cache->mask + 1;
    for ( i = 0; i < count; i++ )
    {
      FTC_Node*  bucket = cache->buckets + i;
      FTC_Node*  pnode  = bucket;


      for ( ;; )
      {
        FTC_Node  node = *pnode;
        FT_Bool   list_changed = FALSE;


        if ( node == NULL )
          break;

        if ( cache->clazz.node_remove_faceid( node, face_id,
                                              cache, &list_changed ) )
        {
          *pnode     = node->link;
          node->link = frees;
          frees      = node;
        }
        else
          pnode = &node->link;
      }
    }

    
    while ( frees )
    {
      FTC_Node  node;


      node  = frees;
      frees = node->link;

      manager->cur_weight -= cache->clazz.node_weight( node, cache );
      ftc_node_mru_unlink( node, manager );

      cache->clazz.node_free( node, cache );

      cache->slack++;
    }

    ftc_cache_resize( cache );
  }



