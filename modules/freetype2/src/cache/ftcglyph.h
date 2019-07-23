

















  








































































  
  
  
  
  
  
  


  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTCGLYPH_H__
#define __FTCGLYPH_H__


#include <ft2build.h>
#include "ftcmanag.h"


FT_BEGIN_HEADER


 







  typedef struct  FTC_FamilyRec_
  {
    FTC_MruNodeRec    mrunode;
    FT_UInt           num_nodes; 
    FTC_Cache         cache;
    FTC_MruListClass  clazz;

  } FTC_FamilyRec, *FTC_Family;

#define  FTC_FAMILY(x)    ( (FTC_Family)(x) )
#define  FTC_FAMILY_P(x)  ( (FTC_Family*)(x) )


  typedef struct  FTC_GNodeRec_
  {
    FTC_NodeRec      node;
    FTC_Family       family;
    FT_UInt          gindex;

  } FTC_GNodeRec, *FTC_GNode;

#define FTC_GNODE( x )    ( (FTC_GNode)(x) )
#define FTC_GNODE_P( x )  ( (FTC_GNode*)(x) )


  typedef struct  FTC_GQueryRec_
  {
    FT_UInt      gindex;
    FTC_Family   family;

  } FTC_GQueryRec, *FTC_GQuery;

#define FTC_GQUERY( x )  ( (FTC_GQuery)(x) )


  
  
  
  
  
  

  
  FT_LOCAL( void )
  FTC_GNode_Init( FTC_GNode   node,
                  FT_UInt     gindex,  
                  FTC_Family  family );

  
  
  
  FT_LOCAL( FT_Bool )
  FTC_GNode_Compare( FTC_GNode   gnode,
                     FTC_GQuery  gquery );

  
  
  FT_LOCAL( void )
  FTC_GNode_UnselectFamily( FTC_GNode  gnode,
                            FTC_Cache  cache );

  
  FT_LOCAL( void )
  FTC_GNode_Done( FTC_GNode  node,
                  FTC_Cache  cache );


  FT_LOCAL( void )
  FTC_Family_Init( FTC_Family  family,
                   FTC_Cache   cache );

  typedef struct FTC_GCacheRec_
  {
    FTC_CacheRec    cache;
    FTC_MruListRec  families;

  } FTC_GCacheRec, *FTC_GCache;

#define FTC_GCACHE( x )  ((FTC_GCache)(x))


#if 0
  
  FT_LOCAL( FT_Error )
  FTC_GCache_Init( FTC_GCache  cache );
#endif


#if 0
  
  FT_LOCAL( void )
  FTC_GCache_Done( FTC_GCache  cache );
#endif


  
  typedef struct  FTC_GCacheClassRec_
  {
    FTC_CacheClassRec  clazz;
    FTC_MruListClass   family_class;

  } FTC_GCacheClassRec;

  typedef const FTC_GCacheClassRec*   FTC_GCacheClass;

#define FTC_GCACHE_CLASS( x )  ((FTC_GCacheClass)(x))

#define FTC_CACHE__GCACHE_CLASS( x ) \
          FTC_GCACHE_CLASS( FTC_CACHE(x)->org_class )
#define FTC_CACHE__FAMILY_CLASS( x ) \
          ( (FTC_MruListClass)FTC_CACHE__GCACHE_CLASS( x )->family_class )


  
  FT_LOCAL( FT_Error )
  FTC_GCache_New( FTC_Manager       manager,
                  FTC_GCacheClass   clazz,
                  FTC_GCache       *acache );

#ifndef FTC_INLINE
  FT_LOCAL( FT_Error )
  FTC_GCache_Lookup( FTC_GCache   cache,
                     FT_UInt32    hash,
                     FT_UInt      gindex,
                     FTC_GQuery   query,
                     FTC_Node    *anode );
#endif


  


#define FTC_FAMILY_FREE( family, cache )                      \
          FTC_MruList_Remove( &FTC_GCACHE((cache))->families, \
                              (FTC_MruNode)(family) )


#ifdef FTC_INLINE

#define FTC_GCACHE_LOOKUP_CMP( cache, famcmp, nodecmp, hash,                \
                               gindex, query, node, error )                 \
  FT_BEGIN_STMNT                                                            \
    FTC_GCache               _gcache   = FTC_GCACHE( cache );               \
    FTC_GQuery               _gquery   = (FTC_GQuery)( query );             \
    FTC_MruNode_CompareFunc  _fcompare = (FTC_MruNode_CompareFunc)(famcmp); \
                                                                            \
                                                                            \
    _gquery->gindex = (gindex);                                             \
                                                                            \
    FTC_MRULIST_LOOKUP_CMP( &_gcache->families, _gquery, _fcompare,         \
                            _gquery->family, error );                       \
    if ( !error )                                                           \
    {                                                                       \
      FTC_Family  _gqfamily = _gquery->family;                              \
                                                                            \
                                                                            \
      _gqfamily->num_nodes++;                                               \
                                                                            \
      FTC_CACHE_LOOKUP_CMP( cache, nodecmp, hash, query, node, error );     \
                                                                            \
      if ( --_gqfamily->num_nodes == 0 )                                    \
        FTC_FAMILY_FREE( _gqfamily, _gcache );                              \
    }                                                                       \
  FT_END_STMNT
  

#else 

#define FTC_GCACHE_LOOKUP_CMP( cache, famcmp, nodecmp, hash,          \
                               gindex, query, node, error )           \
   FT_BEGIN_STMNT                                                     \
     void*  _n = &(node);                                             \
                                                                      \
                                                                      \
     error = FTC_GCache_Lookup( FTC_GCACHE( cache ), hash, gindex,    \
                                FTC_GQUERY( query ), (FTC_Node*)_n ); \
   FT_END_STMNT

#endif 


FT_END_HEADER


#endif 



