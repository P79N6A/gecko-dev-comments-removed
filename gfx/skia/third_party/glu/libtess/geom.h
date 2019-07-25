








































#ifndef __geom_h_
#define __geom_h_

#include "mesh.h"

#ifdef NO_BRANCH_CONDITIONS




#define VertEq(u,v)	(((u)->s == (v)->s) & ((u)->t == (v)->t))
#define VertLeq(u,v)	(((u)->s < (v)->s) | \
                         ((u)->s == (v)->s & (u)->t <= (v)->t))
#else
#define VertEq(u,v)	((u)->s == (v)->s && (u)->t == (v)->t)
#define VertLeq(u,v)	(((u)->s < (v)->s) || \
                         ((u)->s == (v)->s && (u)->t <= (v)->t))
#endif

#define EdgeEval(u,v,w)	__gl_edgeEval(u,v,w)
#define EdgeSign(u,v,w)	__gl_edgeSign(u,v,w)



#define TransLeq(u,v)	(((u)->t < (v)->t) || \
                         ((u)->t == (v)->t && (u)->s <= (v)->s))
#define TransEval(u,v,w)	__gl_transEval(u,v,w)
#define TransSign(u,v,w)	__gl_transSign(u,v,w)


#define EdgeGoesLeft(e)		VertLeq( (e)->Dst, (e)->Org )
#define EdgeGoesRight(e)	VertLeq( (e)->Org, (e)->Dst )

#define ABS(x)	((x) < 0 ? -(x) : (x))
#define VertL1dist(u,v)	(ABS(u->s - v->s) + ABS(u->t - v->t))

#define VertCCW(u,v,w)	__gl_vertCCW(u,v,w)

int		__gl_vertLeq( GLUvertex *u, GLUvertex *v );
GLdouble	__gl_edgeEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_edgeSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
int		__gl_vertCCW( GLUvertex *u, GLUvertex *v, GLUvertex *w );
void		__gl_edgeIntersect( GLUvertex *o1, GLUvertex *d1,
				    GLUvertex *o2, GLUvertex *d2,
				    GLUvertex *v );

#endif
