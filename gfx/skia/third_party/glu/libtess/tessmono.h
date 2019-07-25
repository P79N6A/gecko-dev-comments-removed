








































#ifndef __tessmono_h_
#define __tessmono_h_




























int __gl_meshTessellateMonoRegion( GLUface *face );
int __gl_meshTessellateInterior( GLUmesh *mesh );
void __gl_meshDiscardExterior( GLUmesh *mesh );
int __gl_meshSetWindingNumber( GLUmesh *mesh, int value,
			        GLboolean keepOnlyBoundary );

#endif
