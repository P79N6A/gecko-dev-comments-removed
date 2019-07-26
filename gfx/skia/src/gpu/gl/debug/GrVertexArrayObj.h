






#ifndef GrVertexArrayObj_DEFINED
#define GrVertexArrayObj_DEFINED

#include "GrFakeRefObj.h"

class GrVertexArrayObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrVertexArrayObj);

public:
    GrVertexArrayObj() : GrFakeRefObj() {}

    typedef GrFakeRefObj INHERITED;
};
#endif
