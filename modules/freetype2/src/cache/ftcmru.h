

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTCMRU_H__
#define __FTCMRU_H__


#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif

#define  xxFT_DEBUG_ERROR
#define  FTC_INLINE

FT_BEGIN_HEADER

  typedef struct FTC_MruNodeRec_*  FTC_MruNode;

  typedef struct  FTC_MruNodeRec_
  {
    FTC_MruNode  next;
    FTC_MruNode  prev;

  } FTC_MruNodeRec;


  FT_LOCAL( void )
  FTC_MruNode_Prepend( FTC_MruNode  *plist,
                       FTC_MruNode   node );

  FT_LOCAL( void )
  FTC_MruNode_Up( FTC_MruNode  *plist,
                  FTC_MruNode   node );

  FT_LOCAL( void )
  FTC_MruNode_Remove( FTC_MruNode  *plist,
                      FTC_MruNode   node );


  typedef struct FTC_MruListRec_*              FTC_MruList;

  typedef struct FTC_MruListClassRec_ const *  FTC_MruListClass;


  typedef FT_Bool
  (*FTC_MruNode_CompareFunc)( FTC_MruNode  node,
                              FT_Pointer   key );

  typedef FT_Error
  (*FTC_MruNode_InitFunc)( FTC_MruNode  node,
                           FT_Pointer   key,
                           FT_Pointer   data );

  typedef FT_Error
  (*FTC_MruNode_ResetFunc)( FTC_MruNode  node,
                            FT_Pointer   key,
                            FT_Pointer   data );

  typedef void
  (*FTC_MruNode_DoneFunc)( FTC_MruNode  node,
                           FT_Pointer   data );


  typedef struct  FTC_MruListClassRec_
  {
    FT_UInt                  node_size;
    FTC_MruNode_CompareFunc  node_compare;
    FTC_MruNode_InitFunc     node_init;
    FTC_MruNode_ResetFunc    node_reset;
    FTC_MruNode_DoneFunc     node_done;

  } FTC_MruListClassRec;

  typedef struct  FTC_MruListRec_
  {
    FT_UInt              num_nodes;
    FT_UInt              max_nodes;
    FTC_MruNode          nodes;
    FT_Pointer           data;
    FTC_MruListClassRec  clazz;
    FT_Memory            memory;

  } FTC_MruListRec;


  FT_LOCAL( void )
  FTC_MruList_Init( FTC_MruList       list,
                    FTC_MruListClass  clazz,
                    FT_UInt           max_nodes,
                    FT_Pointer        data,
                    FT_Memory         memory );

  FT_LOCAL( void )
  FTC_MruList_Reset( FTC_MruList  list );


  FT_LOCAL( void )
  FTC_MruList_Done( FTC_MruList  list );


  FT_LOCAL( FT_Error )
  FTC_MruList_New( FTC_MruList   list,
                   FT_Pointer    key,
                   FTC_MruNode  *anode );

  FT_LOCAL( void )
  FTC_MruList_Remove( FTC_MruList  list,
                      FTC_MruNode  node );

  FT_LOCAL( void )
  FTC_MruList_RemoveSelection( FTC_MruList              list,
                               FTC_MruNode_CompareFunc  selection,
                               FT_Pointer               key );


#ifdef FTC_INLINE

#define FTC_MRULIST_LOOKUP_CMP( list, key, compare, node, error )           \
  FT_BEGIN_STMNT                                                            \
    FTC_MruNode*             _pfirst  = &(list)->nodes;                     \
    FTC_MruNode_CompareFunc  _compare = (FTC_MruNode_CompareFunc)(compare); \
    FTC_MruNode              _first, _node, *_pnode;                        \
                                                                            \
                                                                            \
    error  = 0;                                                             \
    _first = *(_pfirst);                                                    \
    _node  = NULL;                                                          \
                                                                            \
    if ( _first )                                                           \
    {                                                                       \
      _node = _first;                                                       \
      do                                                                    \
      {                                                                     \
        if ( _compare( _node, (key) ) )                                     \
        {                                                                   \
          if ( _node != _first )                                            \
            FTC_MruNode_Up( _pfirst, _node );                               \
                                                                            \
          _pnode = (FTC_MruNode*)(void*)&(node);                            \
          *_pnode = _node;                                                  \
          goto _MruOk;                                                      \
        }                                                                   \
        _node = _node->next;                                                \
                                                                            \
      } while ( _node != _first) ;                                          \
    }                                                                       \
                                                                            \
    error = FTC_MruList_New( (list), (key), (FTC_MruNode*)(void*)&(node) ); \
  _MruOk:                                                                   \
    ;                                                                       \
  FT_END_STMNT

#define FTC_MRULIST_LOOKUP( list, key, node, error ) \
  FTC_MRULIST_LOOKUP_CMP( list, key, (list)->clazz.node_compare, node, error )

#else  

  FT_LOCAL( FTC_MruNode )
  FTC_MruList_Find( FTC_MruList  list,
                    FT_Pointer   key );

  FT_LOCAL( FT_Error )
  FTC_MruList_Lookup( FTC_MruList   list,
                      FT_Pointer    key,
                      FTC_MruNode  *pnode );

#define FTC_MRULIST_LOOKUP( list, key, node, error ) \
  error = FTC_MruList_Lookup( (list), (key), (FTC_MruNode*)&(node) )

#endif 


#define FTC_MRULIST_LOOP( list, node )        \
  FT_BEGIN_STMNT                              \
    FTC_MruNode  _first = (list)->nodes;      \
                                              \
                                              \
    if ( _first )                             \
    {                                         \
      FTC_MruNode  _node = _first;            \
                                              \
                                              \
      do                                      \
      {                                       \
        *(FTC_MruNode*)&(node) = _node;


#define FTC_MRULIST_LOOP_END()               \
        _node = _node->next;                 \
                                             \
      } while ( _node != _first );           \
    }                                        \
  FT_END_STMNT

 

FT_END_HEADER


#endif 



