








































#include "gluos.h"
#include <assert.h>
#include <stddef.h>
#include "mesh.h"
#include "tess.h"
#include "render.h"

#define TRUE 1
#define FALSE 0





struct FaceCount {
  long		size;		
  GLUhalfEdge	*eStart;	
  void		(*render)(GLUtesselator *, GLUhalfEdge *, long);
                                
};

static struct FaceCount MaximumFan( GLUhalfEdge *eOrig );
static struct FaceCount MaximumStrip( GLUhalfEdge *eOrig );

static void RenderFan( GLUtesselator *tess, GLUhalfEdge *eStart, long size );
static void RenderStrip( GLUtesselator *tess, GLUhalfEdge *eStart, long size );
static void RenderTriangle( GLUtesselator *tess, GLUhalfEdge *eStart,
			    long size );

static void RenderMaximumFaceGroup( GLUtesselator *tess, GLUface *fOrig );
static void RenderLonelyTriangles( GLUtesselator *tess, GLUface *head );












void __gl_renderMesh( GLUtesselator *tess, GLUmesh *mesh )
{
  GLUface *f;

  
  tess->lonelyTriList = NULL;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
    f->marked = FALSE;
  }
  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {

    



    if( f->inside && ! f->marked ) {
      RenderMaximumFaceGroup( tess, f );
      assert( f->marked );
    }
  }
  if( tess->lonelyTriList != NULL ) {
    RenderLonelyTriangles( tess, tess->lonelyTriList );
    tess->lonelyTriList = NULL;
  }
}


static void RenderMaximumFaceGroup( GLUtesselator *tess, GLUface *fOrig )
{
  






  GLUhalfEdge *e = fOrig->anEdge;
  struct FaceCount max, newFace;

  max.size = 1;
  max.eStart = e;
  max.render = &RenderTriangle;

  if( ! tess->flagBoundary ) {
    newFace = MaximumFan( e ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumFan( e->Lnext ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumFan( e->Lprev ); if( newFace.size > max.size ) { max = newFace; }

    newFace = MaximumStrip( e ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumStrip( e->Lnext ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumStrip( e->Lprev ); if( newFace.size > max.size ) { max = newFace; }
  }
  (*(max.render))( tess, max.eStart, max.size );
}









#define Marked(f)	(! (f)->inside || (f)->marked)

#define AddToTrail(f,t)	((f)->trail = (t), (t) = (f), (f)->marked = TRUE)

#define FreeTrail(t)	do { \
			  while( (t) != NULL ) { \
			    (t)->marked = FALSE; t = (t)->trail; \
			  } \
			} while(0) /* absorb trailing semicolon */



static struct FaceCount MaximumFan( GLUhalfEdge *eOrig )
{
  



  struct FaceCount newFace = { 0, NULL, &RenderFan };
  GLUface *trail = NULL;
  GLUhalfEdge *e;

  for( e = eOrig; ! Marked( e->Lface ); e = e->Onext ) {
    AddToTrail( e->Lface, trail );
    ++newFace.size;
  }
  for( e = eOrig; ! Marked( e->Rface ); e = e->Oprev ) {
    AddToTrail( e->Rface, trail );
    ++newFace.size;
  }
  newFace.eStart = e;
  
  FreeTrail( trail );
  return newFace;
}


#define IsEven(n)	(((n) & 1) == 0)

static struct FaceCount MaximumStrip( GLUhalfEdge *eOrig )
{
  









  struct FaceCount newFace = { 0, NULL, &RenderStrip };
  long headSize = 0, tailSize = 0;
  GLUface *trail = NULL;
  GLUhalfEdge *e, *eTail, *eHead;

  for( e = eOrig; ! Marked( e->Lface ); ++tailSize, e = e->Onext ) {
    AddToTrail( e->Lface, trail );
    ++tailSize;
    e = e->Dprev;
    if( Marked( e->Lface )) break;
    AddToTrail( e->Lface, trail );
  }
  eTail = e;

  for( e = eOrig; ! Marked( e->Rface ); ++headSize, e = e->Dnext ) {
    AddToTrail( e->Rface, trail );
    ++headSize;
    e = e->Oprev;
    if( Marked( e->Rface )) break;
    AddToTrail( e->Rface, trail );
  }
  eHead = e;

  newFace.size = tailSize + headSize;
  if( IsEven( tailSize )) {
    newFace.eStart = eTail->Sym;
  } else if( IsEven( headSize )) {
    newFace.eStart = eHead;
  } else {
    


    --newFace.size;
    newFace.eStart = eHead->Onext;
  }
  
  FreeTrail( trail );
  return newFace;
}


static void RenderTriangle( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  


  assert( size == 1 );
  AddToTrail( e->Lface, tess->lonelyTriList );
}


static void RenderLonelyTriangles( GLUtesselator *tess, GLUface *f )
{
  


  GLUhalfEdge *e;
  int newState;
  int edgeState = -1;	

  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLES );

  for( ; f != NULL; f = f->trail ) {
    

    e = f->anEdge;
    do {
      if( tess->flagBoundary ) {
	


	newState = ! e->Rface->inside;
	if( edgeState != newState ) {
	  edgeState = newState;
          CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA( edgeState );
	}
      }
      CALL_VERTEX_OR_VERTEX_DATA( e->Org->data );

      e = e->Lnext;
    } while( e != f->anEdge );
  }
  CALL_END_OR_END_DATA();
}


static void RenderFan( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  



  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLE_FAN ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 

  while( ! Marked( e->Lface )) {
    e->Lface->marked = TRUE;
    --size;
    e = e->Onext;
    CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 
  }

  assert( size == 0 );
  CALL_END_OR_END_DATA();
}


static void RenderStrip( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  



  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLE_STRIP );
  CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 

  while( ! Marked( e->Lface )) {
    e->Lface->marked = TRUE;
    --size;
    e = e->Dprev;
    CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
    if( Marked( e->Lface )) break;

    e->Lface->marked = TRUE;
    --size;
    e = e->Onext;
    CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 
  }

  assert( size == 0 );
  CALL_END_OR_END_DATA();
}








void __gl_renderBoundary( GLUtesselator *tess, GLUmesh *mesh )
{
  GLUface *f;
  GLUhalfEdge *e;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
    if( f->inside ) {
      CALL_BEGIN_OR_BEGIN_DATA( GL_LINE_LOOP );
      e = f->anEdge;
      do {
        CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
	e = e->Lnext;
      } while( e != f->anEdge );
      CALL_END_OR_END_DATA();
    }
  }
}




#define SIGN_INCONSISTENT 2

static int ComputeNormal( GLUtesselator *tess, GLdouble norm[3], int check )








{
  CachedVertex *v0 = tess->cache;
  CachedVertex *vn = v0 + tess->cacheCount;
  CachedVertex *vc;
  GLdouble dot, xc, yc, zc, xp, yp, zp, n[3];
  int sign = 0;

  












  if( ! check ) {
    norm[0] = norm[1] = norm[2] = 0.0;
  }

  vc = v0 + 1;
  xc = vc->coords[0] - v0->coords[0];
  yc = vc->coords[1] - v0->coords[1];
  zc = vc->coords[2] - v0->coords[2];
  while( ++vc < vn ) {
    xp = xc; yp = yc; zp = zc;
    xc = vc->coords[0] - v0->coords[0];
    yc = vc->coords[1] - v0->coords[1];
    zc = vc->coords[2] - v0->coords[2];

    
    n[0] = yp*zc - zp*yc;
    n[1] = zp*xc - xp*zc;
    n[2] = xp*yc - yp*xc;

    dot = n[0]*norm[0] + n[1]*norm[1] + n[2]*norm[2];
    if( ! check ) {
      


      if( dot >= 0 ) {
	norm[0] += n[0]; norm[1] += n[1]; norm[2] += n[2];
      } else {
	norm[0] -= n[0]; norm[1] -= n[1]; norm[2] -= n[2];
      }
    } else if( dot != 0 ) {
      
      if( dot > 0 ) {
	if( sign < 0 ) return SIGN_INCONSISTENT;
	sign = 1;
      } else {
	if( sign > 0 ) return SIGN_INCONSISTENT;
	sign = -1;
      }
    }
  }
  return sign;
}








GLboolean __gl_renderCache( GLUtesselator *tess )
{
  CachedVertex *v0 = tess->cache;
  CachedVertex *vn = v0 + tess->cacheCount;
  CachedVertex *vc;
  GLdouble norm[3];
  int sign;

  if( tess->cacheCount < 3 ) {
    
    return TRUE;
  }

  norm[0] = tess->normal[0];
  norm[1] = tess->normal[1];
  norm[2] = tess->normal[2];
  if( norm[0] == 0 && norm[1] == 0 && norm[2] == 0 ) {
    ComputeNormal( tess, norm, FALSE );
  }

  sign = ComputeNormal( tess, norm, TRUE );
  if( sign == SIGN_INCONSISTENT ) {
    
    return FALSE;
  }
  if( sign == 0 ) {
    
    return TRUE;
  }

  
  switch( tess->windingRule ) {
  case GLU_TESS_WINDING_ODD:
  case GLU_TESS_WINDING_NONZERO:
    break;
  case GLU_TESS_WINDING_POSITIVE:
    if( sign < 0 ) return TRUE;
    break;
  case GLU_TESS_WINDING_NEGATIVE:
    if( sign > 0 ) return TRUE;
    break;
  case GLU_TESS_WINDING_ABS_GEQ_TWO:
    return TRUE;
  }

  CALL_BEGIN_OR_BEGIN_DATA( tess->boundaryOnly ? GL_LINE_LOOP
			  : (tess->cacheCount > 3) ? GL_TRIANGLE_FAN
			  : GL_TRIANGLES );

  CALL_VERTEX_OR_VERTEX_DATA( v0->data ); 
  if( sign > 0 ) {
    for( vc = v0+1; vc < vn; ++vc ) {
      CALL_VERTEX_OR_VERTEX_DATA( vc->data ); 
    }
  } else {
    for( vc = vn-1; vc > v0; --vc ) {
      CALL_VERTEX_OR_VERTEX_DATA( vc->data ); 
    }
  }
  CALL_END_OR_END_DATA();
  return TRUE;
}
