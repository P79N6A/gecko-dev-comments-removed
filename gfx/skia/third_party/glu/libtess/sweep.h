








































#ifndef __sweep_h_
#define __sweep_h_

#include "mesh.h"







int __gl_computeInterior( GLUtesselator *tess );




#include "dict.h"







struct ActiveRegion {
  GLUhalfEdge	*eUp;		
  DictNode	*nodeUp;	
  int		windingNumber;	

  GLboolean	inside;		
  GLboolean	sentinel;	
  GLboolean	dirty;		


  GLboolean	fixUpperEdge;	


};

#define RegionBelow(r)	((ActiveRegion *) dictKey(dictPred((r)->nodeUp)))
#define RegionAbove(r)	((ActiveRegion *) dictKey(dictSucc((r)->nodeUp)))

#endif
