








































#include "gluos.h"
#include <assert.h>
#include <stddef.h>
#include <setjmp.h>		
#include <limits.h>		

#include "mesh.h"
#include "geom.h"
#include "tess.h"
#include "dict.h"
#include "priorityq.h"
#include "memalloc.h"
#include "sweep.h"

#define TRUE 1
#define FALSE 0

#ifdef FOR_TRITE_TEST_PROGRAM
extern void DebugEvent( GLUtesselator *tess );
#else
#define DebugEvent( tess )
#endif































#define MAX(x,y)	((x) >= (y) ? (x) : (y))
#define MIN(x,y)	((x) <= (y) ? (x) : (y))




#define AddWinding(eDst,eSrc)	(eDst->winding += eSrc->winding, \
				 eDst->Sym->winding += eSrc->Sym->winding)

static void SweepEvent( GLUtesselator *tess, GLUvertex *vEvent );
static void WalkDirtyRegions( GLUtesselator *tess, ActiveRegion *regUp );
static int CheckForRightSplice( GLUtesselator *tess, ActiveRegion *regUp );

static int EdgeLeq( GLUtesselator *tess, ActiveRegion *reg1,
		    ActiveRegion *reg2 )











{
  GLUvertex *event = tess->event;
  GLUhalfEdge *e1, *e2;
  GLdouble t1, t2;

  e1 = reg1->eUp;
  e2 = reg2->eUp;

  if( e1->Dst == event ) {
    if( e2->Dst == event ) {
      


      if( VertLeq( e1->Org, e2->Org )) {
	return EdgeSign( e2->Dst, e1->Org, e2->Org ) <= 0;
      }
      return EdgeSign( e1->Dst, e2->Org, e1->Org ) >= 0;
    }
    return EdgeSign( e2->Dst, event, e2->Org ) <= 0;
  }
  if( e2->Dst == event ) {
    return EdgeSign( e1->Dst, event, e1->Org ) >= 0;
  }

  
  t1 = EdgeEval( e1->Dst, event, e1->Org );
  t2 = EdgeEval( e2->Dst, event, e2->Org );
  return (t1 >= t2);
}


static void DeleteRegion( GLUtesselator *tess, ActiveRegion *reg )
{
  if( reg->fixUpperEdge ) {
    



    assert( reg->eUp->winding == 0 );
  }
  reg->eUp->activeRegion = NULL;
  dictDelete( tess->dict, reg->nodeUp ); 
  memFree( reg );
}


static int FixUpperEdge( ActiveRegion *reg, GLUhalfEdge *newEdge )



{
  assert( reg->fixUpperEdge );
  if ( !__gl_meshDelete( reg->eUp ) ) return 0;
  reg->fixUpperEdge = FALSE;
  reg->eUp = newEdge;
  newEdge->activeRegion = reg;

  return 1; 
}

static ActiveRegion *TopLeftRegion( ActiveRegion *reg )
{
  GLUvertex *org = reg->eUp->Org;
  GLUhalfEdge *e;

  
  do {
    reg = RegionAbove( reg );
  } while( reg->eUp->Org == org );

  


  if( reg->fixUpperEdge ) {
    e = __gl_meshConnect( RegionBelow(reg)->eUp->Sym, reg->eUp->Lnext );
    if (e == NULL) return NULL;
    if ( !FixUpperEdge( reg, e ) ) return NULL;
    reg = RegionAbove( reg );
  }
  return reg;
}

static ActiveRegion *TopRightRegion( ActiveRegion *reg )
{
  GLUvertex *dst = reg->eUp->Dst;

  
  do {
    reg = RegionAbove( reg );
  } while( reg->eUp->Dst == dst );
  return reg;
}

static ActiveRegion *AddRegionBelow( GLUtesselator *tess,
				     ActiveRegion *regAbove,
				     GLUhalfEdge *eNewUp )






{
  ActiveRegion *regNew = (ActiveRegion *)memAlloc( sizeof( ActiveRegion ));
  if (regNew == NULL) longjmp(tess->env,1);

  regNew->eUp = eNewUp;
   
  regNew->nodeUp = dictInsertBefore( tess->dict, regAbove->nodeUp, regNew );
  if (regNew->nodeUp == NULL) longjmp(tess->env,1);
  regNew->fixUpperEdge = FALSE;
  regNew->sentinel = FALSE;
  regNew->dirty = FALSE;

  eNewUp->activeRegion = regNew;
  return regNew;
}

static GLboolean IsWindingInside( GLUtesselator *tess, int n )
{
  switch( tess->windingRule ) {
  case GLU_TESS_WINDING_ODD:
    return (n & 1);
  case GLU_TESS_WINDING_NONZERO:
    return (n != 0);
  case GLU_TESS_WINDING_POSITIVE:
    return (n > 0);
  case GLU_TESS_WINDING_NEGATIVE:
    return (n < 0);
  case GLU_TESS_WINDING_ABS_GEQ_TWO:
    return (n >= 2) || (n <= -2);
  }
  
  assert( FALSE );
  
  return 0;
}


static void ComputeWinding( GLUtesselator *tess, ActiveRegion *reg )
{
  reg->windingNumber = RegionAbove(reg)->windingNumber + reg->eUp->winding;
  reg->inside = IsWindingInside( tess, reg->windingNumber );
}


static void FinishRegion( GLUtesselator *tess, ActiveRegion *reg )







{
  GLUhalfEdge *e = reg->eUp;
  GLUface *f = e->Lface;

  f->inside = reg->inside;
  f->anEdge = e;   
  DeleteRegion( tess, reg );
}


static GLUhalfEdge *FinishLeftRegions( GLUtesselator *tess,
	       ActiveRegion *regFirst, ActiveRegion *regLast )












{
  ActiveRegion *reg, *regPrev;
  GLUhalfEdge *e, *ePrev;

  regPrev = regFirst;
  ePrev = regFirst->eUp;
  while( regPrev != regLast ) {
    regPrev->fixUpperEdge = FALSE;	
    reg = RegionBelow( regPrev );
    e = reg->eUp;
    if( e->Org != ePrev->Org ) {
      if( ! reg->fixUpperEdge ) {
	





	FinishRegion( tess, regPrev );
	break;
      }
      


      e = __gl_meshConnect( ePrev->Lprev, e->Sym );
      if (e == NULL) longjmp(tess->env,1);
      if ( !FixUpperEdge( reg, e ) ) longjmp(tess->env,1);
    }

    
    if( ePrev->Onext != e ) {
      if ( !__gl_meshSplice( e->Oprev, e ) ) longjmp(tess->env,1);
      if ( !__gl_meshSplice( ePrev, e ) ) longjmp(tess->env,1);
    }
    FinishRegion( tess, regPrev );	
    ePrev = reg->eUp;
    regPrev = reg;
  }
  return ePrev;
}


static void AddRightEdges( GLUtesselator *tess, ActiveRegion *regUp,
       GLUhalfEdge *eFirst, GLUhalfEdge *eLast, GLUhalfEdge *eTopLeft,
       GLboolean cleanUp )










{
  ActiveRegion *reg, *regPrev;
  GLUhalfEdge *e, *ePrev;
  int firstTime = TRUE;

  
  e = eFirst;
  do {
    assert( VertLeq( e->Org, e->Dst ));
    AddRegionBelow( tess, regUp, e->Sym );
    e = e->Onext;
  } while ( e != eLast );

  



  if( eTopLeft == NULL ) {
    eTopLeft = RegionBelow( regUp )->eUp->Rprev;
  }
  regPrev = regUp;
  ePrev = eTopLeft;
  for( ;; ) {
    reg = RegionBelow( regPrev );
    e = reg->eUp->Sym;
    if( e->Org != ePrev->Org ) break;

    if( e->Onext != ePrev ) {
      
      if ( !__gl_meshSplice( e->Oprev, e ) ) longjmp(tess->env,1);
      if ( !__gl_meshSplice( ePrev->Oprev, e ) ) longjmp(tess->env,1);
    }
    
    reg->windingNumber = regPrev->windingNumber - e->winding;
    reg->inside = IsWindingInside( tess, reg->windingNumber );

    


    regPrev->dirty = TRUE;
    if( ! firstTime && CheckForRightSplice( tess, regPrev )) {
      AddWinding( e, ePrev );
      DeleteRegion( tess, regPrev );
      if ( !__gl_meshDelete( ePrev ) ) longjmp(tess->env,1);
    }
    firstTime = FALSE;
    regPrev = reg;
    ePrev = e;
  }
  regPrev->dirty = TRUE;
  assert( regPrev->windingNumber - e->winding == reg->windingNumber );

  if( cleanUp ) {
    
    WalkDirtyRegions( tess, regPrev );
  }
}


static void CallCombine( GLUtesselator *tess, GLUvertex *isect,
			 void *data[4], GLfloat weights[4], int needed )
{
  GLdouble coords[3];

  
  coords[0] = isect->coords[0];
  coords[1] = isect->coords[1];
  coords[2] = isect->coords[2];

  isect->data = NULL;
  CALL_COMBINE_OR_COMBINE_DATA( coords, data, weights, &isect->data );
  if( isect->data == NULL ) {
    if( ! needed ) {
      isect->data = data[0];
    } else if( ! tess->fatalError ) {
      



      CALL_ERROR_OR_ERROR_DATA( GLU_TESS_NEED_COMBINE_CALLBACK );
      tess->fatalError = TRUE;
    }
  }
}

static void SpliceMergeVertices( GLUtesselator *tess, GLUhalfEdge *e1,
				 GLUhalfEdge *e2 )




{
  void *data[4] = { NULL, NULL, NULL, NULL };
  GLfloat weights[4] = { 0.5, 0.5, 0.0, 0.0 };

  data[0] = e1->Org->data;
  data[1] = e2->Org->data;
  CallCombine( tess, e1->Org, data, weights, FALSE );
  if ( !__gl_meshSplice( e1, e2 ) ) longjmp(tess->env,1); 
}

static void VertexWeights( GLUvertex *isect, GLUvertex *org, GLUvertex *dst,
                           GLfloat *weights )







{
  GLdouble t1 = VertL1dist( org, isect );
  GLdouble t2 = VertL1dist( dst, isect );

  weights[0] = 0.5 * t2 / (t1 + t2);
  weights[1] = 0.5 * t1 / (t1 + t2);
  isect->coords[0] += weights[0]*org->coords[0] + weights[1]*dst->coords[0];
  isect->coords[1] += weights[0]*org->coords[1] + weights[1]*dst->coords[1];
  isect->coords[2] += weights[0]*org->coords[2] + weights[1]*dst->coords[2];
}


static void GetIntersectData( GLUtesselator *tess, GLUvertex *isect,
       GLUvertex *orgUp, GLUvertex *dstUp,
       GLUvertex *orgLo, GLUvertex *dstLo )





{
  void *data[4];
  GLfloat weights[4];

  data[0] = orgUp->data;
  data[1] = dstUp->data;
  data[2] = orgLo->data;
  data[3] = dstLo->data;

  isect->coords[0] = isect->coords[1] = isect->coords[2] = 0;
  VertexWeights( isect, orgUp, dstUp, &weights[0] );
  VertexWeights( isect, orgLo, dstLo, &weights[2] );

  CallCombine( tess, isect, data, weights, TRUE );
}

static int CheckForRightSplice( GLUtesselator *tess, ActiveRegion *regUp )

























{
  ActiveRegion *regLo = RegionBelow(regUp);
  GLUhalfEdge *eUp = regUp->eUp;
  GLUhalfEdge *eLo = regLo->eUp;

  if( VertLeq( eUp->Org, eLo->Org )) {
    if( EdgeSign( eLo->Dst, eUp->Org, eLo->Org ) > 0 ) return FALSE;

    
    if( ! VertEq( eUp->Org, eLo->Org )) {
      
      if ( __gl_meshSplitEdge( eLo->Sym ) == NULL) longjmp(tess->env,1);
      if ( !__gl_meshSplice( eUp, eLo->Oprev ) ) longjmp(tess->env,1);
      regUp->dirty = regLo->dirty = TRUE;

    } else if( eUp->Org != eLo->Org ) {
      
      pqDelete( tess->pq, eUp->Org->pqHandle ); 
      SpliceMergeVertices( tess, eLo->Oprev, eUp );
    }
  } else {
    if( EdgeSign( eUp->Dst, eLo->Org, eUp->Org ) < 0 ) return FALSE;

    
    RegionAbove(regUp)->dirty = regUp->dirty = TRUE;
    if (__gl_meshSplitEdge( eUp->Sym ) == NULL) longjmp(tess->env,1);
    if ( !__gl_meshSplice( eLo->Oprev, eUp ) ) longjmp(tess->env,1);
  }
  return TRUE;
}

static int CheckForLeftSplice( GLUtesselator *tess, ActiveRegion *regUp )


















{
  ActiveRegion *regLo = RegionBelow(regUp);
  GLUhalfEdge *eUp = regUp->eUp;
  GLUhalfEdge *eLo = regLo->eUp;
  GLUhalfEdge *e;

  assert( ! VertEq( eUp->Dst, eLo->Dst ));

  if( VertLeq( eUp->Dst, eLo->Dst )) {
    if( EdgeSign( eUp->Dst, eLo->Dst, eUp->Org ) < 0 ) return FALSE;

    
    RegionAbove(regUp)->dirty = regUp->dirty = TRUE;
    e = __gl_meshSplitEdge( eUp );
    if (e == NULL) longjmp(tess->env,1);
    if ( !__gl_meshSplice( eLo->Sym, e ) ) longjmp(tess->env,1);
    e->Lface->inside = regUp->inside;
  } else {
    if( EdgeSign( eLo->Dst, eUp->Dst, eLo->Org ) > 0 ) return FALSE;

    
    regUp->dirty = regLo->dirty = TRUE;
    e = __gl_meshSplitEdge( eLo );
    if (e == NULL) longjmp(tess->env,1);    
    if ( !__gl_meshSplice( eUp->Lnext, eLo->Sym ) ) longjmp(tess->env,1);
    e->Rface->inside = regUp->inside;
  }
  return TRUE;
}


static int CheckForIntersect( GLUtesselator *tess, ActiveRegion *regUp )









{
  ActiveRegion *regLo = RegionBelow(regUp);
  GLUhalfEdge *eUp = regUp->eUp;
  GLUhalfEdge *eLo = regLo->eUp;
  GLUvertex *orgUp = eUp->Org;
  GLUvertex *orgLo = eLo->Org;
  GLUvertex *dstUp = eUp->Dst;
  GLUvertex *dstLo = eLo->Dst;
  GLdouble tMinUp, tMaxLo;
  GLUvertex isect, *orgMin;
  GLUhalfEdge *e;

  assert( ! VertEq( dstLo, dstUp ));
  assert( EdgeSign( dstUp, tess->event, orgUp ) <= 0 );
  assert( EdgeSign( dstLo, tess->event, orgLo ) >= 0 );
  assert( orgUp != tess->event && orgLo != tess->event );
  assert( ! regUp->fixUpperEdge && ! regLo->fixUpperEdge );

  if( orgUp == orgLo ) return FALSE;	

  tMinUp = MIN( orgUp->t, dstUp->t );
  tMaxLo = MAX( orgLo->t, dstLo->t );
  if( tMinUp > tMaxLo ) return FALSE;	

  if( VertLeq( orgUp, orgLo )) {
    if( EdgeSign( dstLo, orgUp, orgLo ) > 0 ) return FALSE;
  } else {
    if( EdgeSign( dstUp, orgLo, orgUp ) < 0 ) return FALSE;
  }

  
  DebugEvent( tess );

  __gl_edgeIntersect( dstUp, orgUp, dstLo, orgLo, &isect );
  
  assert( MIN( orgUp->t, dstUp->t ) <= isect.t );
  assert( isect.t <= MAX( orgLo->t, dstLo->t ));
  assert( MIN( dstLo->s, dstUp->s ) <= isect.s );
  assert( isect.s <= MAX( orgLo->s, orgUp->s ));

  if( VertLeq( &isect, tess->event )) {
    





    isect.s = tess->event->s;
    isect.t = tess->event->t;
  }
  





  orgMin = VertLeq( orgUp, orgLo ) ? orgUp : orgLo;
  if( VertLeq( orgMin, &isect )) {
    isect.s = orgMin->s;
    isect.t = orgMin->t;
  }

  if( VertEq( &isect, orgUp ) || VertEq( &isect, orgLo )) {
    
    (void) CheckForRightSplice( tess, regUp );
    return FALSE;
  }

  if(    (! VertEq( dstUp, tess->event )
	  && EdgeSign( dstUp, tess->event, &isect ) >= 0)
      || (! VertEq( dstLo, tess->event )
	  && EdgeSign( dstLo, tess->event, &isect ) <= 0 ))
  {
    



    if( dstLo == tess->event ) {
      
      if (__gl_meshSplitEdge( eUp->Sym ) == NULL) longjmp(tess->env,1);
      if ( !__gl_meshSplice( eLo->Sym, eUp ) ) longjmp(tess->env,1);
      regUp = TopLeftRegion( regUp );
      if (regUp == NULL) longjmp(tess->env,1);
      eUp = RegionBelow(regUp)->eUp;
      FinishLeftRegions( tess, RegionBelow(regUp), regLo );
      AddRightEdges( tess, regUp, eUp->Oprev, eUp, eUp, TRUE );
      return TRUE;
    }
    if( dstUp == tess->event ) {
      
      if (__gl_meshSplitEdge( eLo->Sym ) == NULL) longjmp(tess->env,1);
      if ( !__gl_meshSplice( eUp->Lnext, eLo->Oprev ) ) longjmp(tess->env,1); 
      regLo = regUp;
      regUp = TopRightRegion( regUp );
      e = RegionBelow(regUp)->eUp->Rprev;
      regLo->eUp = eLo->Oprev;
      eLo = FinishLeftRegions( tess, regLo, NULL );
      AddRightEdges( tess, regUp, eLo->Onext, eUp->Rprev, e, TRUE );
      return TRUE;
    }
    



    if( EdgeSign( dstUp, tess->event, &isect ) >= 0 ) {
      RegionAbove(regUp)->dirty = regUp->dirty = TRUE;
      if (__gl_meshSplitEdge( eUp->Sym ) == NULL) longjmp(tess->env,1);
      eUp->Org->s = tess->event->s;
      eUp->Org->t = tess->event->t;
    }
    if( EdgeSign( dstLo, tess->event, &isect ) <= 0 ) {
      regUp->dirty = regLo->dirty = TRUE;
      if (__gl_meshSplitEdge( eLo->Sym ) == NULL) longjmp(tess->env,1);
      eLo->Org->s = tess->event->s;
      eLo->Org->t = tess->event->t;
    }
    
    return FALSE;
  }

  







  if (__gl_meshSplitEdge( eUp->Sym ) == NULL) longjmp(tess->env,1);
  if (__gl_meshSplitEdge( eLo->Sym ) == NULL) longjmp(tess->env,1);
  if ( !__gl_meshSplice( eLo->Oprev, eUp ) ) longjmp(tess->env,1);
  eUp->Org->s = isect.s;
  eUp->Org->t = isect.t;
  eUp->Org->pqHandle = pqInsert( tess->pq, eUp->Org ); 
  if (eUp->Org->pqHandle == LONG_MAX) {
     pqDeletePriorityQ(tess->pq);	
     tess->pq = NULL;
     longjmp(tess->env,1);
  }
  GetIntersectData( tess, eUp->Org, orgUp, dstUp, orgLo, dstLo );
  RegionAbove(regUp)->dirty = regUp->dirty = regLo->dirty = TRUE;
  return FALSE;
}

static void WalkDirtyRegions( GLUtesselator *tess, ActiveRegion *regUp )








{
  ActiveRegion *regLo = RegionBelow(regUp);
  GLUhalfEdge *eUp, *eLo;

  for( ;; ) {
    
    while( regLo->dirty ) {
      regUp = regLo;
      regLo = RegionBelow(regLo);
    }
    if( ! regUp->dirty ) {
      regLo = regUp;
      regUp = RegionAbove( regUp );
      if( regUp == NULL || ! regUp->dirty ) {
	
	return;
      }
    }
    regUp->dirty = FALSE;
    eUp = regUp->eUp;
    eLo = regLo->eUp;

    if( eUp->Dst != eLo->Dst ) {
      
      if( CheckForLeftSplice( tess, regUp )) {

	



	if( regLo->fixUpperEdge ) {
	  DeleteRegion( tess, regLo );
	  if ( !__gl_meshDelete( eLo ) ) longjmp(tess->env,1);
	  regLo = RegionBelow( regUp );
	  eLo = regLo->eUp;
	} else if( regUp->fixUpperEdge ) {
	  DeleteRegion( tess, regUp );
	  if ( !__gl_meshDelete( eUp ) ) longjmp(tess->env,1);
	  regUp = RegionAbove( regLo );
	  eUp = regUp->eUp;
	}
      }
    }
    if( eUp->Org != eLo->Org ) {
      if(    eUp->Dst != eLo->Dst
	  && ! regUp->fixUpperEdge && ! regLo->fixUpperEdge
          && (eUp->Dst == tess->event || eLo->Dst == tess->event) )
      {
	







	if( CheckForIntersect( tess, regUp )) {
	  
	  return;
	}
      } else {
	


	(void) CheckForRightSplice( tess, regUp );
      }
    }
    if( eUp->Org == eLo->Org && eUp->Dst == eLo->Dst ) {
      
      AddWinding( eLo, eUp );
      DeleteRegion( tess, regUp );
      if ( !__gl_meshDelete( eUp ) ) longjmp(tess->env,1);
      regUp = RegionAbove( regLo );
    }
  }
}


static void ConnectRightVertex( GLUtesselator *tess, ActiveRegion *regUp,
			        GLUhalfEdge *eBottomLeft )































{
  GLUhalfEdge *eNew;
  GLUhalfEdge *eTopLeft = eBottomLeft->Onext;
  ActiveRegion *regLo = RegionBelow(regUp);
  GLUhalfEdge *eUp = regUp->eUp;
  GLUhalfEdge *eLo = regLo->eUp;
  int degenerate = FALSE;

  if( eUp->Dst != eLo->Dst ) {
    (void) CheckForIntersect( tess, regUp );
  }

  


  if( VertEq( eUp->Org, tess->event )) {
    if ( !__gl_meshSplice( eTopLeft->Oprev, eUp ) ) longjmp(tess->env,1);
    regUp = TopLeftRegion( regUp );
    if (regUp == NULL) longjmp(tess->env,1);
    eTopLeft = RegionBelow( regUp )->eUp;
    FinishLeftRegions( tess, RegionBelow(regUp), regLo );
    degenerate = TRUE;
  }
  if( VertEq( eLo->Org, tess->event )) {
    if ( !__gl_meshSplice( eBottomLeft, eLo->Oprev ) ) longjmp(tess->env,1);
    eBottomLeft = FinishLeftRegions( tess, regLo, NULL );
    degenerate = TRUE;
  }
  if( degenerate ) {
    AddRightEdges( tess, regUp, eBottomLeft->Onext, eTopLeft, eTopLeft, TRUE );
    return;
  }

  


  if( VertLeq( eLo->Org, eUp->Org )) {
    eNew = eLo->Oprev;
  } else {
    eNew = eUp;
  }
  eNew = __gl_meshConnect( eBottomLeft->Lprev, eNew );
  if (eNew == NULL) longjmp(tess->env,1);

  


  AddRightEdges( tess, regUp, eNew, eNew->Onext, eNew->Onext, FALSE );
  eNew->Sym->activeRegion->fixUpperEdge = TRUE;
  WalkDirtyRegions( tess, regUp );
}








#define TOLERANCE_NONZERO	FALSE

static void ConnectLeftDegenerate( GLUtesselator *tess,
				   ActiveRegion *regUp, GLUvertex *vEvent )





{
  GLUhalfEdge *e, *eTopLeft, *eTopRight, *eLast;
  ActiveRegion *reg;

  e = regUp->eUp;
  if( VertEq( e->Org, vEvent )) {
    


    assert( TOLERANCE_NONZERO );
    SpliceMergeVertices( tess, e, vEvent->anEdge );
    return;
  }
  
  if( ! VertEq( e->Dst, vEvent )) {
    
    if (__gl_meshSplitEdge( e->Sym ) == NULL) longjmp(tess->env,1);
    if( regUp->fixUpperEdge ) {
      
      if ( !__gl_meshDelete( e->Onext ) ) longjmp(tess->env,1);
      regUp->fixUpperEdge = FALSE;
    }
    if ( !__gl_meshSplice( vEvent->anEdge, e ) ) longjmp(tess->env,1);
    SweepEvent( tess, vEvent );	
    return;
  }

  


  assert( TOLERANCE_NONZERO );
  regUp = TopRightRegion( regUp );
  reg = RegionBelow( regUp );
  eTopRight = reg->eUp->Sym;
  eTopLeft = eLast = eTopRight->Onext;
  if( reg->fixUpperEdge ) {
    


    assert( eTopLeft != eTopRight );   
    DeleteRegion( tess, reg );
    if ( !__gl_meshDelete( eTopRight ) ) longjmp(tess->env,1);
    eTopRight = eTopLeft->Oprev;
  }
  if ( !__gl_meshSplice( vEvent->anEdge, eTopRight ) ) longjmp(tess->env,1);
  if( ! EdgeGoesLeft( eTopLeft )) {
    
    eTopLeft = NULL;
  }
  AddRightEdges( tess, regUp, eTopRight->Onext, eLast, eTopLeft, TRUE );
}


static void ConnectLeftVertex( GLUtesselator *tess, GLUvertex *vEvent )















{
  ActiveRegion *regUp, *regLo, *reg;
  GLUhalfEdge *eUp, *eLo, *eNew;
  ActiveRegion tmp;

  

  
  tmp.eUp = vEvent->anEdge->Sym;
   
  regUp = (ActiveRegion *)dictKey( dictSearch( tess->dict, &tmp ));
  regLo = RegionBelow( regUp );
  eUp = regUp->eUp;
  eLo = regLo->eUp;

  
  if( EdgeSign( eUp->Dst, vEvent, eUp->Org ) == 0 ) {
    ConnectLeftDegenerate( tess, regUp, vEvent );
    return;
  }

  


  reg = VertLeq( eLo->Dst, eUp->Dst ) ? regUp : regLo;

  if( regUp->inside || reg->fixUpperEdge) {
    if( reg == regUp ) {
      eNew = __gl_meshConnect( vEvent->anEdge->Sym, eUp->Lnext );
      if (eNew == NULL) longjmp(tess->env,1);
    } else {
      GLUhalfEdge *tempHalfEdge= __gl_meshConnect( eLo->Dnext, vEvent->anEdge);
      if (tempHalfEdge == NULL) longjmp(tess->env,1);

      eNew = tempHalfEdge->Sym;
    }
    if( reg->fixUpperEdge ) {
      if ( !FixUpperEdge( reg, eNew ) ) longjmp(tess->env,1);
    } else {
      ComputeWinding( tess, AddRegionBelow( tess, regUp, eNew ));
    }
    SweepEvent( tess, vEvent );
  } else {
    


    AddRightEdges( tess, regUp, vEvent->anEdge, vEvent->anEdge, NULL, TRUE );
  }
}


static void SweepEvent( GLUtesselator *tess, GLUvertex *vEvent )




{
  ActiveRegion *regUp, *reg;
  GLUhalfEdge *e, *eTopLeft, *eBottomLeft;

  tess->event = vEvent;		
  DebugEvent( tess );
  
  



  e = vEvent->anEdge;
  while( e->activeRegion == NULL ) {
    e = e->Onext;
    if( e == vEvent->anEdge ) {
      
      ConnectLeftVertex( tess, vEvent );
      return;
    }
  }

  






  regUp = TopLeftRegion( e->activeRegion );
  if (regUp == NULL) longjmp(tess->env,1);
  reg = RegionBelow( regUp );
  eTopLeft = reg->eUp;
  eBottomLeft = FinishLeftRegions( tess, reg, NULL );

  




  if( eBottomLeft->Onext == eTopLeft ) {
    
    ConnectRightVertex( tess, regUp, eBottomLeft );
  } else {
    AddRightEdges( tess, regUp, eBottomLeft->Onext, eTopLeft, eTopLeft, TRUE );
  }
}







#define SENTINEL_COORD	(4 * GLU_TESS_MAX_COORD)

static void AddSentinel( GLUtesselator *tess, GLdouble t )




{
  GLUhalfEdge *e;
  ActiveRegion *reg = (ActiveRegion *)memAlloc( sizeof( ActiveRegion ));
  if (reg == NULL) longjmp(tess->env,1);

  e = __gl_meshMakeEdge( tess->mesh );
  if (e == NULL) longjmp(tess->env,1);

  e->Org->s = SENTINEL_COORD;
  e->Org->t = t;
  e->Dst->s = -SENTINEL_COORD;
  e->Dst->t = t;
  tess->event = e->Dst;		

  reg->eUp = e;
  reg->windingNumber = 0;
  reg->inside = FALSE;
  reg->fixUpperEdge = FALSE;
  reg->sentinel = TRUE;
  reg->dirty = FALSE;
  reg->nodeUp = dictInsert( tess->dict, reg ); 
  if (reg->nodeUp == NULL) longjmp(tess->env,1);
}


static void InitEdgeDict( GLUtesselator *tess )




{
  
  tess->dict = dictNewDict( tess, (int (*)(void *, DictKey, DictKey)) EdgeLeq );
  if (tess->dict == NULL) longjmp(tess->env,1);

  AddSentinel( tess, -SENTINEL_COORD );
  AddSentinel( tess, SENTINEL_COORD );
}


static void DoneEdgeDict( GLUtesselator *tess )
{
  ActiveRegion *reg;
#ifndef NDEBUG
  int fixedEdges = 0;
#endif

   
  while( (reg = (ActiveRegion *)dictKey( dictMin( tess->dict ))) != NULL ) {
    




    if( ! reg->sentinel ) {
      assert( reg->fixUpperEdge );
      assert( ++fixedEdges == 1 );
    }
    assert( reg->windingNumber == 0 );
    DeleteRegion( tess, reg );

  }
  dictDeleteDict( tess->dict );	
}


static void RemoveDegenerateEdges( GLUtesselator *tess )



{
  GLUhalfEdge *e, *eNext, *eLnext;
  GLUhalfEdge *eHead = &tess->mesh->eHead;

  
  for( e = eHead->next; e != eHead; e = eNext ) {
    eNext = e->next;
    eLnext = e->Lnext;
    
    if( VertEq( e->Org, e->Dst ) && e->Lnext->Lnext != e ) {
      
      
      SpliceMergeVertices( tess, eLnext, e );	
      if ( !__gl_meshDelete( e ) ) longjmp(tess->env,1); 
      e = eLnext;
      eLnext = e->Lnext;
    }
    if( eLnext->Lnext == e ) {
      
      
      if( eLnext != e ) {
	if( eLnext == eNext || eLnext == eNext->Sym ) { eNext = eNext->next; }
	if ( !__gl_meshDelete( eLnext ) ) longjmp(tess->env,1);
      }
      if( e == eNext || e == eNext->Sym ) { eNext = eNext->next; }
      if ( !__gl_meshDelete( e ) ) longjmp(tess->env,1);
    }
  }
}

static int InitPriorityQ( GLUtesselator *tess )




{
  PriorityQ *pq;
  GLUvertex *v, *vHead;

  
  pq = tess->pq = pqNewPriorityQ( (int (*)(PQkey, PQkey)) __gl_vertLeq );
  if (pq == NULL) return 0;

  vHead = &tess->mesh->vHead;
  for( v = vHead->next; v != vHead; v = v->next ) {
    v->pqHandle = pqInsert( pq, v ); 
    if (v->pqHandle == LONG_MAX) break;
  }
  if (v != vHead || !pqInit( pq ) ) { 
    pqDeletePriorityQ(tess->pq);	
    tess->pq = NULL;
    return 0;
  }

  return 1;
}


static void DonePriorityQ( GLUtesselator *tess )
{
  pqDeletePriorityQ( tess->pq ); 
}


static int RemoveDegenerateFaces( GLUmesh *mesh )














{
  GLUface *f, *fNext;
  GLUhalfEdge *e;

  
  for( f = mesh->fHead.next; f != &mesh->fHead; f = fNext ) {
    fNext = f->next;
    e = f->anEdge;
    assert( e->Lnext != e );

    if( e->Lnext->Lnext == e ) {
      
      AddWinding( e->Onext, e );
      if ( !__gl_meshDelete( e ) ) return 0;
    }
  }
  return 1;
}

int __gl_computeInterior( GLUtesselator *tess )







{
  GLUvertex *v, *vNext;

  tess->fatalError = FALSE;

  





  RemoveDegenerateEdges( tess );
  if ( !InitPriorityQ( tess ) ) return 0; 
  InitEdgeDict( tess );

  
  while( (v = (GLUvertex *)pqExtractMin( tess->pq )) != NULL ) {
    for( ;; ) {
      vNext = (GLUvertex *)pqMinimum( tess->pq ); 
      if( vNext == NULL || ! VertEq( vNext, v )) break;
      
      













      vNext = (GLUvertex *)pqExtractMin( tess->pq ); 
      SpliceMergeVertices( tess, v->anEdge, vNext->anEdge );
    }
    SweepEvent( tess, v );
  }

  
    
  tess->event = ((ActiveRegion *) dictKey( dictMin( tess->dict )))->eUp->Org;
  DebugEvent( tess );
  DoneEdgeDict( tess );
  DonePriorityQ( tess );

  if ( !RemoveDegenerateFaces( tess->mesh ) ) return 0;
  __gl_meshCheckMesh( tess->mesh );

  return 1;
}
