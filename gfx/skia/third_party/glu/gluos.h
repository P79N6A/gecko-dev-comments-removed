






























#ifndef GLUOS_H_
#define GLUOS_H_



#define GLAPIENTRY

typedef unsigned char GLboolean;
typedef double GLdouble;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef void GLvoid;

#define gluNewTess Sk_gluNewTess
#define gluDeleteTess Sk_gluDeleteTess
#define gluTessProperty Sk_gluTessProperty
#define gluGetTessProperty Sk_gluGetTessProperty
#define gluTessNormal Sk_gluTessNormal
#define gluTessCallback Sk_gluTessCallback
#define gluTessVertex Sk_gluTessVertex
#define gluTessBeginPolygon Sk_gluTessBeginPolygon
#define gluTessBeginContour Sk_gluTessBeginContour
#define gluTessEndContour Sk_gluTessEndContour
#define gluTessEndPolygon Sk_gluTessEndPolygon

#define __gl_noBeginData Sk__gl_noBeginData
#define __gl_noEdgeFlagData Sk__gl_noEdgeFlagData
#define __gl_noVertexData Sk__gl_noVertexData
#define __gl_noEndData Sk__gl_noEndData
#define __gl_noErrorData Sk__gl_noErrorData
#define __gl_noCombineData Sk__gl_noCombineData
#define __gl_computeInterior Sk__gl_computeInterior
#define __gl_dictListDelete Sk__gl_dictListDelete
#define __gl_dictListDeleteDict Sk__gl_dictListDeleteDict
#define __gl_dictListInsertBefore Sk__gl_dictListInsertBefore
#define __gl_dictListNewDict Sk__gl_dictListNewDict
#define __gl_dictListSearch Sk__gl_dictListSearch
#define __gl_edgeEval Sk__gl_edgeEval
#define __gl_edgeIntersect Sk__gl_edgeIntersect
#define __gl_edgeSign Sk__gl_edgeSign
#define __gl_memInit Sk__gl_memInit
#define __gl_meshAddEdgeVertex Sk__gl_meshAddEdgeVertex
#define __gl_meshCheckMesh Sk__gl_meshCheckMesh
#define __gl_meshConnect Sk__gl_meshConnect
#define __gl_meshDelete Sk__gl_meshDelete
#define __gl_meshDeleteMesh Sk__gl_meshDeleteMesh
#define __gl_meshDiscardExterior Sk__gl_meshDiscardExterior
#define __gl_meshMakeEdge Sk__gl_meshMakeEdge
#define __gl_meshNewMesh Sk__gl_meshNewMesh
#define __gl_meshSetWindingNumber Sk__gl_meshSetWindingNumber
#define __gl_meshSplice Sk__gl_meshSplice
#define __gl_meshSplitEdge Sk__gl_meshSplitEdge
#define __gl_meshTessellateInterior Sk__gl_meshTessellateInterior
#define __gl_meshTessellateMonoRegion Sk__gl_meshTessellateMonoRegion
#define __gl_meshUnion Sk__gl_meshUnion
#define __gl_meshZapFace Sk__gl_meshZapFace
#define __gl_pqHeapDelete Sk__gl_pqHeapDelete
#define __gl_pqHeapDeletePriorityQ Sk__gl_pqHeapDeletePriorityQ
#define __gl_pqHeapExtractMin Sk__gl_pqHeapExtractMin
#define __gl_pqHeapInit Sk__gl_pqHeapInit
#define __gl_pqHeapInsert Sk__gl_pqHeapInsert
#define __gl_pqHeapNewPriorityQ Sk__gl_pqHeapNewPriorityQ
#define __gl_pqSortDelete Sk__gl_pqSortDelete
#define __gl_pqSortDeletePriorityQ Sk__gl_pqSortDeletePriorityQ
#define __gl_pqSortExtractMin Sk__gl_pqSortExtractMin
#define __gl_pqSortInit Sk__gl_pqSortInit
#define __gl_pqSortInsert Sk__gl_pqSortInsert
#define __gl_pqSortIsEmpty Sk__gl_pqSortIsEmpty
#define __gl_pqSortMinimum Sk__gl_pqSortMinimum
#define __gl_pqSortNewPriorityQ Sk__gl_pqSortNewPriorityQ
#define __gl_projectPolygon Sk__gl_projectPolygon
#define __gl_renderBoundary Sk__gl_renderBoundary
#define __gl_renderCache Sk__gl_renderCache
#define __gl_renderMesh Sk__gl_renderMesh
#define __gl_transEval Sk__gl_transEval
#define __gl_transSign Sk__gl_transSign
#define __gl_vertCCW Sk__gl_vertCCW
#define __gl_vertLeq Sk__gl_vertLeq


#undef MIN
#undef MAX

#endif  
