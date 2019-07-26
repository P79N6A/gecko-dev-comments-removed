






#ifndef GrVertexEffect_DEFINED
#define GrVertexEffect_DEFINED

#include "GrEffect.h"






class GrVertexEffect : public GrEffect {
public:
    GrVertexEffect() { fHasVertexCode = true; }

protected:
    




    void addVertexAttrib(GrSLType type) {
        SkASSERT(fVertexAttribTypes.count() < kMaxVertexAttribs);
        fVertexAttribTypes.push_back(type);
    }

private:
    typedef GrEffect INHERITED;
};

#endif
