








































#ifndef __tess_h_
#define __tess_h_

#include <sk_glu.h>
#include <setjmp.h>
#include "mesh.h"
#include "dict.h"
#include "priorityq.h"




enum TessState { T_DORMANT, T_IN_POLYGON, T_IN_CONTOUR };




#define TESS_MAX_CACHE	100

typedef struct CachedVertex {
  GLdouble	coords[3];
  void		*data;
} CachedVertex;

struct GLUtesselator {

  

  enum TessState state;		

  GLUhalfEdge	*lastEdge;	
  GLUmesh	*mesh;		


  void		(GLAPIENTRY *callError)( GLenum errnum );

  

  GLdouble	normal[3];	
  GLdouble	sUnit[3];	
  GLdouble	tUnit[3];	

  

  GLdouble	relTolerance;	
  GLenum	windingRule;	
  GLboolean	fatalError;	

  Dict		*dict;		
  PriorityQ	*pq;		
  GLUvertex	*event;		

  void		(GLAPIENTRY *callCombine)( GLdouble coords[3], void *data[4],
			        GLfloat weight[4], void **outData );

  

  GLboolean	flagBoundary;	
  GLboolean	boundaryOnly;	
  GLUface	*lonelyTriList;
    

  void		(GLAPIENTRY *callBegin)( GLenum type );
  void		(GLAPIENTRY *callEdgeFlag)( GLboolean boundaryEdge );
  void		(GLAPIENTRY *callVertex)( void *data );
  void		(GLAPIENTRY *callEnd)( void );
  void		(GLAPIENTRY *callMesh)( GLUmesh *mesh );


  

  GLboolean	emptyCache;		
  int		cacheCount;		
  CachedVertex	cache[TESS_MAX_CACHE];	

   
  void		(GLAPIENTRY *callBeginData)( GLenum type, void *polygonData );
  void		(GLAPIENTRY *callEdgeFlagData)( GLboolean boundaryEdge, 
				     void *polygonData );
  void		(GLAPIENTRY *callVertexData)( void *data, void *polygonData );
  void		(GLAPIENTRY *callEndData)( void *polygonData );
  void		(GLAPIENTRY *callErrorData)( GLenum errnum, void *polygonData );
  void		(GLAPIENTRY *callCombineData)( GLdouble coords[3], void *data[4],
				    GLfloat weight[4], void **outData,
				    void *polygonData );

  jmp_buf env;			

  void *polygonData;		
};

void GLAPIENTRY __gl_noBeginData( GLenum type, void *polygonData );
void GLAPIENTRY __gl_noEdgeFlagData( GLboolean boundaryEdge, void *polygonData );
void GLAPIENTRY __gl_noVertexData( void *data, void *polygonData );
void GLAPIENTRY __gl_noEndData( void *polygonData );
void GLAPIENTRY __gl_noErrorData( GLenum errnum, void *polygonData );
void GLAPIENTRY __gl_noCombineData( GLdouble coords[3], void *data[4],
			 GLfloat weight[4], void **outData,
			 void *polygonData );

#define CALL_BEGIN_OR_BEGIN_DATA(a) \
   if (tess->callBeginData != &__gl_noBeginData) \
      (*tess->callBeginData)((a),tess->polygonData); \
   else (*tess->callBegin)((a));

#define CALL_VERTEX_OR_VERTEX_DATA(a) \
   if (tess->callVertexData != &__gl_noVertexData) \
      (*tess->callVertexData)((a),tess->polygonData); \
   else (*tess->callVertex)((a));

#define CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA(a) \
   if (tess->callEdgeFlagData != &__gl_noEdgeFlagData) \
      (*tess->callEdgeFlagData)((a),tess->polygonData); \
   else (*tess->callEdgeFlag)((a));

#define CALL_END_OR_END_DATA() \
   if (tess->callEndData != &__gl_noEndData) \
      (*tess->callEndData)(tess->polygonData); \
   else (*tess->callEnd)();

#define CALL_COMBINE_OR_COMBINE_DATA(a,b,c,d) \
   if (tess->callCombineData != &__gl_noCombineData) \
      (*tess->callCombineData)((a),(b),(c),(d),tess->polygonData); \
   else (*tess->callCombine)((a),(b),(c),(d));

#define CALL_ERROR_OR_ERROR_DATA(a) \
   if (tess->callErrorData != &__gl_noErrorData) \
      (*tess->callErrorData)((a),tess->polygonData); \
   else (*tess->callError)((a));

#endif
