








































#ifndef __render_h_
#define __render_h_

#include "mesh.h"








void __gl_renderMesh( GLUtesselator *tess, GLUmesh *mesh );
void __gl_renderBoundary( GLUtesselator *tess, GLUmesh *mesh );

GLboolean __gl_renderCache( GLUtesselator *tess );

#endif
