

















#ifndef __FTCCACHE_H__
#define __FTCCACHE_H__


#include "ftcmru.h"

FT_BEGIN_HEADER

  
  typedef struct FTC_CacheRec_*  FTC_Cache;

  
  typedef const struct FTC_CacheClassRec_*  FTC_CacheClass;


  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  typedef struct  FTC_NodeRec_
  {
    FTC_MruNodeRec  mru;          
    FTC_Node        link;         
    FT_UInt32       hash;         
    FT_UShort       cache_index;  
    FT_Short        ref_count;    

  } FTC_NodeRec;


#define FTC_NODE( x )    ( (FTC_Node)(x) )
#define FTC_NODE_P( x )  ( (FTC_Node*)(x) )

#define FTC_NODE__NEXT( x )  FTC_NODE( (x)->mru.next )
#define FTC_NODE__PREV( x )  FTC_NODE( (x)->mru.prev )


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
  FT_BASE( void )
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager );
#endif


  
  
  
  
  
  
  

  
  typedef FT_Error
  (*FTC_Node_NewFunc)( FTC_Node    *pnode,
                       FT_Pointer   query,
                       FTC_Cache    cache );

  typedef FT_ULong
  (*FTC_Node_WeightFunc)( FTC_Node   node,
                          FTC_Cache  cache );

  
  typedef FT_Bool
  (*FTC_Node_CompareFunc)( FTC_Node    node,
                           FT_Pointer  key,
                           FTC_Cache   cache );


  typedef void
  (*FTC_Node_FreeFunc)( FTC_Node   node,
                        FTC_Cache  cache );

  typedef FT_Error
  (*FTC_Cache_InitFunc)( FTC_Cache  cache );

  typedef void
  (*FTC_Cache_DoneFunc)( FTC_Cache  cache );


  typedef struct  FTC_CacheClassRec_
  {
    FTC_Node_NewFunc      node_new;
    FTC_Node_WeightFunc   node_weight;
    FTC_Node_CompareFunc  node_compare;
    FTC_Node_CompareFunc  node_remove_faceid;
    FTC_Node_FreeFunc     node_free;

    FT_UInt               cache_size;
    FTC_Cache_InitFunc    cache_init;
    FTC_Cache_DoneFunc    cache_done;

  } FTC_CacheClassRec;


  
  typedef struct  FTC_CacheRec_
  {
    FT_UFast           p;
    FT_UFast           mask;
    FT_Long            slack;
    FTC_Node*          buckets;

    FTC_CacheClassRec  clazz;       

    FTC_Manager        manager;
    FT_Memory          memory;
    FT_UInt            index;       

    FTC_CacheClass     org_class;   

  } FTC_CacheRec;


#define FTC_CACHE( x )    ( (FTC_Cache)(x) )
#define FTC_CACHE_P( x )  ( (FTC_Cache*)(x) )


  
  FT_LOCAL( FT_Error )
  FTC_Cache_Init( FTC_Cache  cache );

  
  FT_LOCAL( void )
  FTC_Cache_Done( FTC_Cache  cache );

  





#ifndef FTC_INLINE
  FT_LOCAL( FT_Error )
  FTC_Cache_Lookup( FTC_Cache   cache,
                    FT_UInt32   hash,
                    FT_Pointer  query,
                    FTC_Node   *anode );
#endif

  FT_LOCAL( FT_Error )
  FTC_Cache_NewNode( FTC_Cache   cache,
                     FT_UInt32   hash,
                     FT_Pointer  query,
                     FTC_Node   *anode );

  









  FT_LOCAL( void )
  FTC_Cache_RemoveFaceID( FTC_Cache   cache,
                          FTC_FaceID  face_id );


#ifdef FTC_INLINE

#define FTC_CACHE_LOOKUP_CMP( cache, nodecmp, hash, query, node, error ) \
  FT_BEGIN_STMNT                                                         \
    FTC_Node             *_bucket, *_pnode, _node;                       \
    FTC_Cache             _cache   = FTC_CACHE(cache);                   \
    FT_UInt32             _hash    = (FT_UInt32)(hash);                  \
    FTC_Node_CompareFunc  _nodcomp = (FTC_Node_CompareFunc)(nodecmp);    \
    FT_UInt               _idx;                                          \
                                                                         \
                                                                         \
    error = 0;                                                           \
    node  = NULL;                                                        \
    _idx  = _hash & _cache->mask;                                        \
    if ( _idx < _cache->p )                                              \
      _idx = _hash & ( _cache->mask*2 + 1 );                             \
                                                                         \
    _bucket = _pnode = _cache->buckets + _idx;                           \
    for (;;)                                                             \
    {                                                                    \
      _node = *_pnode;                                                   \
      if ( _node == NULL )                                               \
        goto _NewNode;                                                   \
                                                                         \
      if ( _node->hash == _hash && _nodcomp( _node, query, _cache ) )    \
        break;                                                           \
                                                                         \
      _pnode = &_node->link;                                             \
    }                                                                    \
                                                                         \
    if ( _node != *_bucket )                                             \
    {                                                                    \
      *_pnode     = _node->link;                                         \
      _node->link = *_bucket;                                            \
      *_bucket    = _node;                                               \
    }                                                                    \
                                                                         \
    {                                                                    \
      FTC_Manager  _manager = _cache->manager;                           \
      void*        _nl      = &_manager->nodes_list;                     \
                                                                         \
                                                                         \
      if ( _node != _manager->nodes_list )                               \
        FTC_MruNode_Up( (FTC_MruNode*)_nl,                               \
                        (FTC_MruNode)_node );                            \
    }                                                                    \
    goto _Ok;                                                            \
                                                                         \
  _NewNode:                                                              \
    error = FTC_Cache_NewNode( _cache, _hash, query, &_node );           \
                                                                         \
  _Ok:                                                                   \
    _pnode = (FTC_Node*)(void*)&(node);                                  \
    *_pnode = _node;                                                     \
  FT_END_STMNT

#else 

#define FTC_CACHE_LOOKUP_CMP( cache, nodecmp, hash, query, node, error ) \
  FT_BEGIN_STMNT                                                         \
    error = FTC_Cache_Lookup( FTC_CACHE( cache ), hash, query,           \
                              (FTC_Node*)&(node) );                      \
  FT_END_STMNT

#endif 


  















#define FTC_CACHE_TRYLOOP( cache )                           \
  {                                                          \
    FTC_Manager  _try_manager = FTC_CACHE( cache )->manager; \
    FT_UInt      _try_count   = 4;                           \
                                                             \
                                                             \
    for (;;)                                                 \
    {                                                        \
      FT_UInt  _try_done;


#define FTC_CACHE_TRYLOOP_END()                                   \
      if ( !error || error != FT_Err_Out_Of_Memory )              \
        break;                                                    \
                                                                  \
      _try_done = FTC_Manager_FlushN( _try_manager, _try_count ); \
      if ( _try_done == 0 )                                       \
        break;                                                    \
                                                                  \
      if ( _try_done == _try_count )                              \
      {                                                           \
        _try_count *= 2;                                          \
        if ( _try_count < _try_done              ||               \
            _try_count > _try_manager->num_nodes )                \
          _try_count = _try_manager->num_nodes;                   \
      }                                                           \
    }                                                             \
  }

 

FT_END_HEADER


#endif 



