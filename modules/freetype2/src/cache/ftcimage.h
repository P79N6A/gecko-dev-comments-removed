

















 








  
  
  
  
  


#ifndef __FTCIMAGE_H__
#define __FTCIMAGE_H__


#include <ft2build.h>
#include FT_CACHE_H
#include "ftcglyph.h"

FT_BEGIN_HEADER


  
  typedef struct  FTC_INodeRec_
  {
    FTC_GNodeRec  gnode;
    FT_Glyph      glyph;

  } FTC_INodeRec, *FTC_INode;

#define FTC_INODE( x )         ( (FTC_INode)( x ) )
#define FTC_INODE_GINDEX( x )  FTC_GNODE(x)->gindex
#define FTC_INODE_FAMILY( x )  FTC_GNODE(x)->family

  typedef FT_Error
  (*FTC_IFamily_LoadGlyphFunc)( FTC_Family  family,
                                FT_UInt     gindex,
                                FTC_Cache   cache,
                                FT_Glyph   *aglyph );

  typedef struct  FTC_IFamilyClassRec_
  {
    FTC_MruListClassRec        clazz;
    FTC_IFamily_LoadGlyphFunc  family_load_glyph;

  } FTC_IFamilyClassRec;

  typedef const FTC_IFamilyClassRec*  FTC_IFamilyClass;

#define FTC_IFAMILY_CLASS( x )  ((FTC_IFamilyClass)(x))

#define FTC_CACHE__IFAMILY_CLASS( x ) \
          FTC_IFAMILY_CLASS( FTC_CACHE__GCACHE_CLASS(x)->family_class )


  
  FT_LOCAL( void )
  FTC_INode_Free( FTC_INode  inode,
                  FTC_Cache  cache );

  



  FT_LOCAL( FT_Error )
  FTC_INode_New( FTC_INode   *pinode,
                 FTC_GQuery   gquery,
                 FTC_Cache    cache );

#if 0
  
  FT_LOCAL( FT_ULong )
  FTC_INode_Weight( FTC_INode  inode );
#endif


 

FT_END_HEADER

#endif 



