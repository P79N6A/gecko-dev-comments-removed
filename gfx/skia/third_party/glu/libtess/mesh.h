








































#ifndef __mesh_h_
#define __mesh_h_

#include <sk_glu.h>

typedef struct GLUmesh GLUmesh; 

typedef struct GLUvertex GLUvertex;
typedef struct GLUface GLUface;
typedef struct GLUhalfEdge GLUhalfEdge;

typedef struct ActiveRegion ActiveRegion;	



































































struct GLUvertex {
  GLUvertex	*next;		
  GLUvertex	*prev;		
  GLUhalfEdge	*anEdge;	
  void		*data;		

  
  GLdouble	coords[3];	
  GLdouble	s, t;		
  long		pqHandle;	
};

struct GLUface {
  GLUface	*next;		
  GLUface	*prev;		
  GLUhalfEdge	*anEdge;	
  void		*data;		

  
  GLUface	*trail;		
  GLboolean	marked;		
  GLboolean	inside;		
};

struct GLUhalfEdge {
  GLUhalfEdge	*next;		
  GLUhalfEdge	*Sym;		
  GLUhalfEdge	*Onext;		
  GLUhalfEdge	*Lnext;		
  GLUvertex	*Org;		
  GLUface	*Lface;		

  
  ActiveRegion	*activeRegion;	
  int		winding;	

};

#define	Rface	Sym->Lface
#define Dst	Sym->Org

#define Oprev	Sym->Lnext
#define Lprev   Onext->Sym
#define Dprev	Lnext->Sym
#define Rprev	Sym->Onext
#define Dnext	Rprev->Sym	/* 3 pointers */
#define Rnext	Oprev->Sym	/* 3 pointers */


struct GLUmesh {
  GLUvertex	vHead;		
  GLUface	fHead;		
  GLUhalfEdge	eHead;		
  GLUhalfEdge	eHeadSym;	
};














































































GLUhalfEdge	*__gl_meshMakeEdge( GLUmesh *mesh );
int		__gl_meshSplice( GLUhalfEdge *eOrg, GLUhalfEdge *eDst );
int		__gl_meshDelete( GLUhalfEdge *eDel );

GLUhalfEdge	*__gl_meshAddEdgeVertex( GLUhalfEdge *eOrg );
GLUhalfEdge	*__gl_meshSplitEdge( GLUhalfEdge *eOrg );
GLUhalfEdge	*__gl_meshConnect( GLUhalfEdge *eOrg, GLUhalfEdge *eDst );

GLUmesh		*__gl_meshNewMesh( void );
GLUmesh		*__gl_meshUnion( GLUmesh *mesh1, GLUmesh *mesh2 );
void		__gl_meshDeleteMesh( GLUmesh *mesh );
void		__gl_meshZapFace( GLUface *fZap );

#ifdef NDEBUG
#define		__gl_meshCheckMesh( mesh )
#else
void		__gl_meshCheckMesh( GLUmesh *mesh );
#endif

#endif
