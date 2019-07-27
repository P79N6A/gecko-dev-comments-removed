






#ifndef SkVertState_DEFINED
#define SkVertState_DEFINED

#include "SkCanvas.h"








struct VertState {
    int f0, f1, f2;

    



    VertState(int vCount, const uint16_t indices[], int indexCount)
            : fIndices(indices) {
        fCurrIndex = 0;
        if (indices) {
            fCount = indexCount;
        } else {
            fCount = vCount;
        }
    }

    typedef bool (*Proc)(VertState*);

    



    Proc chooseProc(SkCanvas::VertexMode mode);

private:
    int             fCount;
    int             fCurrIndex;
    const uint16_t* fIndices;

    static bool Triangles(VertState*);
    static bool TrianglesX(VertState*);
    static bool TriangleStrip(VertState*);
    static bool TriangleStripX(VertState*);
    static bool TriangleFan(VertState*);
    static bool TriangleFanX(VertState*);
};

#endif
