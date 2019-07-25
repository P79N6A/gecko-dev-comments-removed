








































#include "gluos.h"
#include <stddef.h>
#include <assert.h>
#include "mesh.h"
#include "memalloc.h"

#define TRUE 1
#define FALSE 0

static GLUvertex *allocVertex()
{
   return (GLUvertex *)memAlloc( sizeof( GLUvertex ));
}

static GLUface *allocFace()
{
   return (GLUface *)memAlloc( sizeof( GLUface ));
}






typedef struct { GLUhalfEdge e, eSym; } EdgePair;





static GLUhalfEdge *MakeEdge( GLUhalfEdge *eNext )
{
  GLUhalfEdge *e;
  GLUhalfEdge *eSym;
  GLUhalfEdge *ePrev;
  EdgePair *pair = (EdgePair *)memAlloc( sizeof( EdgePair ));
  if (pair == NULL) return NULL;

  e = &pair->e;
  eSym = &pair->eSym;

  
  if( eNext->Sym < eNext ) { eNext = eNext->Sym; }

  


  ePrev = eNext->Sym->next;
  eSym->next = ePrev;
  ePrev->Sym->next = e;
  e->next = eNext;
  eNext->Sym->next = eSym;

  e->Sym = eSym;
  e->Onext = e;
  e->Lnext = eSym;
  e->Org = NULL;
  e->Lface = NULL;
  e->winding = 0;
  e->activeRegion = NULL;

  eSym->Sym = e;
  eSym->Onext = eSym;
  eSym->Lnext = e;
  eSym->Org = NULL;
  eSym->Lface = NULL;
  eSym->winding = 0;
  eSym->activeRegion = NULL;

  return e;
}







static void Splice( GLUhalfEdge *a, GLUhalfEdge *b )
{
  GLUhalfEdge *aOnext = a->Onext;
  GLUhalfEdge *bOnext = b->Onext;

  aOnext->Sym->Lnext = b;
  bOnext->Sym->Lnext = a;
  a->Onext = bOnext;
  b->Onext = aOnext;
}







static void MakeVertex( GLUvertex *newVertex, 
			GLUhalfEdge *eOrig, GLUvertex *vNext )
{
  GLUhalfEdge *e;
  GLUvertex *vPrev;
  GLUvertex *vNew = newVertex;

  assert(vNew != NULL);

  
  vPrev = vNext->prev;
  vNew->prev = vPrev;
  vPrev->next = vNew;
  vNew->next = vNext;
  vNext->prev = vNew;

  vNew->anEdge = eOrig;
  vNew->data = NULL;
  

  
  e = eOrig;
  do {
    e->Org = vNew;
    e = e->Onext;
  } while( e != eOrig );
}







static void MakeFace( GLUface *newFace, GLUhalfEdge *eOrig, GLUface *fNext )
{
  GLUhalfEdge *e;
  GLUface *fPrev;
  GLUface *fNew = newFace;

  assert(fNew != NULL); 

  
  fPrev = fNext->prev;
  fNew->prev = fPrev;
  fPrev->next = fNew;
  fNew->next = fNext;
  fNext->prev = fNew;

  fNew->anEdge = eOrig;
  fNew->data = NULL;
  fNew->trail = NULL;
  fNew->marked = FALSE;

  


  fNew->inside = fNext->inside;

  
  e = eOrig;
  do {
    e->Lface = fNew;
    e = e->Lnext;
  } while( e != eOrig );
}




static void KillEdge( GLUhalfEdge *eDel )
{
  GLUhalfEdge *ePrev, *eNext;

  
  if( eDel->Sym < eDel ) { eDel = eDel->Sym; }

  
  eNext = eDel->next;
  ePrev = eDel->Sym->next;
  eNext->Sym->next = ePrev;
  ePrev->Sym->next = eNext;

  memFree( eDel );
}





static void KillVertex( GLUvertex *vDel, GLUvertex *newOrg )
{
  GLUhalfEdge *e, *eStart = vDel->anEdge;
  GLUvertex *vPrev, *vNext;

  
  e = eStart;
  do {
    e->Org = newOrg;
    e = e->Onext;
  } while( e != eStart );

  
  vPrev = vDel->prev;
  vNext = vDel->next;
  vNext->prev = vPrev;
  vPrev->next = vNext;

  memFree( vDel );
}




static void KillFace( GLUface *fDel, GLUface *newLface )
{
  GLUhalfEdge *e, *eStart = fDel->anEdge;
  GLUface *fPrev, *fNext;

  
  e = eStart;
  do {
    e->Lface = newLface;
    e = e->Lnext;
  } while( e != eStart );

  
  fPrev = fDel->prev;
  fNext = fDel->next;
  fNext->prev = fPrev;
  fPrev->next = fNext;

  memFree( fDel );
}







GLUhalfEdge *__gl_meshMakeEdge( GLUmesh *mesh )
{
  GLUvertex *newVertex1= allocVertex();
  GLUvertex *newVertex2= allocVertex();
  GLUface *newFace= allocFace();
  GLUhalfEdge *e;

  
  if (newVertex1 == NULL || newVertex2 == NULL || newFace == NULL) {
     if (newVertex1 != NULL) memFree(newVertex1);
     if (newVertex2 != NULL) memFree(newVertex2);
     if (newFace != NULL) memFree(newFace);     
     return NULL;
  } 

  e = MakeEdge( &mesh->eHead );
  if (e == NULL) return NULL;

  MakeVertex( newVertex1, e, &mesh->vHead );
  MakeVertex( newVertex2, e->Sym, &mesh->vHead );
  MakeFace( newFace, e, &mesh->fHead );
  return e;
}
  
























int __gl_meshSplice( GLUhalfEdge *eOrg, GLUhalfEdge *eDst )
{
  int joiningLoops = FALSE;
  int joiningVertices = FALSE;

  if( eOrg == eDst ) return 1;

  if( eDst->Org != eOrg->Org ) {
    
    joiningVertices = TRUE;
    KillVertex( eDst->Org, eOrg->Org );
  }
  if( eDst->Lface != eOrg->Lface ) {
    
    joiningLoops = TRUE;
    KillFace( eDst->Lface, eOrg->Lface );
  }

  
  Splice( eDst, eOrg );

  if( ! joiningVertices ) {
    GLUvertex *newVertex= allocVertex();
    if (newVertex == NULL) return 0;

    


    MakeVertex( newVertex, eDst, eOrg->Org );
    eOrg->Org->anEdge = eOrg;
  }
  if( ! joiningLoops ) {
    GLUface *newFace= allocFace();  
    if (newFace == NULL) return 0;

    


    MakeFace( newFace, eDst, eOrg->Lface );
    eOrg->Lface->anEdge = eOrg;
  }

  return 1;
}












int __gl_meshDelete( GLUhalfEdge *eDel )
{
  GLUhalfEdge *eDelSym = eDel->Sym;
  int joiningLoops = FALSE;

  


  if( eDel->Lface != eDel->Rface ) {
    
    joiningLoops = TRUE;
    KillFace( eDel->Lface, eDel->Rface );
  }

  if( eDel->Onext == eDel ) {
    KillVertex( eDel->Org, NULL );
  } else {
    
    eDel->Rface->anEdge = eDel->Oprev;
    eDel->Org->anEdge = eDel->Onext;

    Splice( eDel, eDel->Oprev );
    if( ! joiningLoops ) {
      GLUface *newFace= allocFace();
      if (newFace == NULL) return 0; 

      
      MakeFace( newFace, eDel, eDel->Lface );
    }
  }

  


  if( eDelSym->Onext == eDelSym ) {
    KillVertex( eDelSym->Org, NULL );
    KillFace( eDelSym->Lface, NULL );
  } else {
    
    eDel->Lface->anEdge = eDelSym->Oprev;
    eDelSym->Org->anEdge = eDelSym->Onext;
    Splice( eDelSym, eDelSym->Oprev );
  }

  
  KillEdge( eDel );

  return 1;
}













GLUhalfEdge *__gl_meshAddEdgeVertex( GLUhalfEdge *eOrg )
{
  GLUhalfEdge *eNewSym;
  GLUhalfEdge *eNew = MakeEdge( eOrg );
  if (eNew == NULL) return NULL;

  eNewSym = eNew->Sym;

  
  Splice( eNew, eOrg->Lnext );

  
  eNew->Org = eOrg->Dst;
  {
    GLUvertex *newVertex= allocVertex();
    if (newVertex == NULL) return NULL;

    MakeVertex( newVertex, eNewSym, eNew->Org );
  }
  eNew->Lface = eNewSym->Lface = eOrg->Lface;

  return eNew;
}






GLUhalfEdge *__gl_meshSplitEdge( GLUhalfEdge *eOrg )
{
  GLUhalfEdge *eNew;
  GLUhalfEdge *tempHalfEdge= __gl_meshAddEdgeVertex( eOrg );
  if (tempHalfEdge == NULL) return NULL;

  eNew = tempHalfEdge->Sym;

  
  Splice( eOrg->Sym, eOrg->Sym->Oprev );
  Splice( eOrg->Sym, eNew );

  
  eOrg->Dst = eNew->Org;
  eNew->Dst->anEdge = eNew->Sym;	
  eNew->Rface = eOrg->Rface;
  eNew->winding = eOrg->winding;	
  eNew->Sym->winding = eOrg->Sym->winding;

  return eNew;
}












GLUhalfEdge *__gl_meshConnect( GLUhalfEdge *eOrg, GLUhalfEdge *eDst )
{
  GLUhalfEdge *eNewSym;
  int joiningLoops = FALSE;  
  GLUhalfEdge *eNew = MakeEdge( eOrg );
  if (eNew == NULL) return NULL;

  eNewSym = eNew->Sym;

  if( eDst->Lface != eOrg->Lface ) {
    
    joiningLoops = TRUE;
    KillFace( eDst->Lface, eOrg->Lface );
  }

  
  Splice( eNew, eOrg->Lnext );
  Splice( eNewSym, eDst );

  
  eNew->Org = eOrg->Dst;
  eNewSym->Org = eDst->Org;
  eNew->Lface = eNewSym->Lface = eOrg->Lface;

  
  eOrg->Lface->anEdge = eNewSym;

  if( ! joiningLoops ) {
    GLUface *newFace= allocFace();
    if (newFace == NULL) return NULL;

    
    MakeFace( newFace, eNew, eOrg->Lface );
  }
  return eNew;
}











void __gl_meshZapFace( GLUface *fZap )
{
  GLUhalfEdge *eStart = fZap->anEdge;
  GLUhalfEdge *e, *eNext, *eSym;
  GLUface *fPrev, *fNext;

  
  eNext = eStart->Lnext;
  do {
    e = eNext;
    eNext = e->Lnext;

    e->Lface = NULL;
    if( e->Rface == NULL ) {
      

      if( e->Onext == e ) {
	KillVertex( e->Org, NULL );
      } else {
	
	e->Org->anEdge = e->Onext;
	Splice( e, e->Oprev );
      }
      eSym = e->Sym;
      if( eSym->Onext == eSym ) {
	KillVertex( eSym->Org, NULL );
      } else {
	
	eSym->Org->anEdge = eSym->Onext;
	Splice( eSym, eSym->Oprev );
      }
      KillEdge( e );
    }
  } while( e != eStart );

  
  fPrev = fZap->prev;
  fNext = fZap->next;
  fNext->prev = fPrev;
  fPrev->next = fNext;

  memFree( fZap );
}





GLUmesh *__gl_meshNewMesh( void )
{
  GLUvertex *v;
  GLUface *f;
  GLUhalfEdge *e;
  GLUhalfEdge *eSym;
  GLUmesh *mesh = (GLUmesh *)memAlloc( sizeof( GLUmesh ));
  if (mesh == NULL) {
     return NULL;
  }
  
  v = &mesh->vHead;
  f = &mesh->fHead;
  e = &mesh->eHead;
  eSym = &mesh->eHeadSym;

  v->next = v->prev = v;
  v->anEdge = NULL;
  v->data = NULL;

  f->next = f->prev = f;
  f->anEdge = NULL;
  f->data = NULL;
  f->trail = NULL;
  f->marked = FALSE;
  f->inside = FALSE;

  e->next = e;
  e->Sym = eSym;
  e->Onext = NULL;
  e->Lnext = NULL;
  e->Org = NULL;
  e->Lface = NULL;
  e->winding = 0;
  e->activeRegion = NULL;

  eSym->next = eSym;
  eSym->Sym = e;
  eSym->Onext = NULL;
  eSym->Lnext = NULL;
  eSym->Org = NULL;
  eSym->Lface = NULL;
  eSym->winding = 0;
  eSym->activeRegion = NULL;

  return mesh;
}





GLUmesh *__gl_meshUnion( GLUmesh *mesh1, GLUmesh *mesh2 )
{
  GLUface *f1 = &mesh1->fHead;
  GLUvertex *v1 = &mesh1->vHead;
  GLUhalfEdge *e1 = &mesh1->eHead;
  GLUface *f2 = &mesh2->fHead;
  GLUvertex *v2 = &mesh2->vHead;
  GLUhalfEdge *e2 = &mesh2->eHead;

  
  if( f2->next != f2 ) {
    f1->prev->next = f2->next;
    f2->next->prev = f1->prev;
    f2->prev->next = f1;
    f1->prev = f2->prev;
  }

  if( v2->next != v2 ) {
    v1->prev->next = v2->next;
    v2->next->prev = v1->prev;
    v2->prev->next = v1;
    v1->prev = v2->prev;
  }

  if( e2->next != e2 ) {
    e1->Sym->next->Sym->next = e2->next;
    e2->next->Sym->next = e1->Sym->next;
    e2->Sym->next->Sym->next = e1;
    e1->Sym->next = e2->Sym->next;
  }

  memFree( mesh2 );
  return mesh1;
}


#ifdef DELETE_BY_ZAPPING



void __gl_meshDeleteMesh( GLUmesh *mesh )
{
  GLUface *fHead = &mesh->fHead;

  while( fHead->next != fHead ) {
    __gl_meshZapFace( fHead->next );
  }
  assert( mesh->vHead.next == &mesh->vHead );

  memFree( mesh );
}

#else



void __gl_meshDeleteMesh( GLUmesh *mesh )
{
  GLUface *f, *fNext;
  GLUvertex *v, *vNext;
  GLUhalfEdge *e, *eNext;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = fNext ) {
    fNext = f->next;
    memFree( f );
  }

  for( v = mesh->vHead.next; v != &mesh->vHead; v = vNext ) {
    vNext = v->next;
    memFree( v );
  }

  for( e = mesh->eHead.next; e != &mesh->eHead; e = eNext ) {
    
    eNext = e->next;
    memFree( e );
  }

  memFree( mesh );
}

#endif

#ifndef NDEBUG



void __gl_meshCheckMesh( GLUmesh *mesh )
{
  GLUface *fHead = &mesh->fHead;
  GLUvertex *vHead = &mesh->vHead;
  GLUhalfEdge *eHead = &mesh->eHead;
  GLUface *f, *fPrev;
  GLUvertex *v, *vPrev;
  GLUhalfEdge *e, *ePrev;

  fPrev = fHead;
  for( fPrev = fHead ; (f = fPrev->next) != fHead; fPrev = f) {
    assert( f->prev == fPrev );
    e = f->anEdge;
    do {
      assert( e->Sym != e );
      assert( e->Sym->Sym == e );
      assert( e->Lnext->Onext->Sym == e );
      assert( e->Onext->Sym->Lnext == e );
      assert( e->Lface == f );
      e = e->Lnext;
    } while( e != f->anEdge );
  }
  assert( f->prev == fPrev && f->anEdge == NULL && f->data == NULL );

  vPrev = vHead;
  for( vPrev = vHead ; (v = vPrev->next) != vHead; vPrev = v) {
    assert( v->prev == vPrev );
    e = v->anEdge;
    do {
      assert( e->Sym != e );
      assert( e->Sym->Sym == e );
      assert( e->Lnext->Onext->Sym == e );
      assert( e->Onext->Sym->Lnext == e );
      assert( e->Org == v );
      e = e->Onext;
    } while( e != v->anEdge );
  }
  assert( v->prev == vPrev && v->anEdge == NULL && v->data == NULL );

  ePrev = eHead;
  for( ePrev = eHead ; (e = ePrev->next) != eHead; ePrev = e) {
    assert( e->Sym->next == ePrev->Sym );
    assert( e->Sym != e );
    assert( e->Sym->Sym == e );
    assert( e->Org != NULL );
    assert( e->Dst != NULL );
    assert( e->Lnext->Onext->Sym == e );
    assert( e->Onext->Sym->Lnext == e );
  }
  assert( e->Sym->next == ePrev->Sym
       && e->Sym == &mesh->eHeadSym
       && e->Sym->Sym == e
       && e->Org == NULL && e->Dst == NULL
       && e->Lface == NULL && e->Rface == NULL );
}

#endif
