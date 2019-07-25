









#include "GrBatchedTextContext.h"
#include "GrContext.h"
#include "GrDrawTarget.h"
#include "GrIndexBuffer.h"
#include "GrTextContext.h"


GrBatchedTextContext::GrBatchedTextContext() {
}

GrBatchedTextContext::~GrBatchedTextContext() {
}

void GrBatchedTextContext::init(GrContext* context,
                                const GrPaint& grPaint,
                                const GrMatrix* extMatrix) {
    this->INHERITED::init(context, grPaint, extMatrix);
    fGrPaint = grPaint;
    fDrawTarget = fContext->getTextTarget(fGrPaint);

    fMaxVertices = 0;
    fCurrTexture = NULL;
    fCurrVertex = 0;
}

void GrBatchedTextContext::finish() {
    fDrawTarget = NULL;

    this->INHERITED::finish();
}

void GrBatchedTextContext::reset() {
    GrAssert(this->isValid());
    fDrawTarget->resetVertexSource();
    fMaxVertices = 0;
    fCurrVertex = 0;
    fCurrTexture->unref();
    fCurrTexture = NULL;
}

void GrBatchedTextContext::prepareForGlyph(GrTexture* texture) {
    GrAssert(this->isValid());
    GrAssert(texture);
    if (fCurrTexture != texture || fCurrVertex + 4 > fMaxVertices) {
        this->flush();
        fCurrTexture = texture;
        fCurrTexture->ref();
    }
}

void GrBatchedTextContext::setupVertexBuff(void** vertexBuff,
                                           GrVertexLayout vertexLayout) {
    GrAssert(this->isValid());
    if (NULL == *vertexBuff) {
        
        
        fMaxVertices = kMinRequestedVerts;
        bool flush = fDrawTarget->geometryHints(vertexLayout,
                                                &fMaxVertices,
                                                NULL);
        if (flush) {
            this->flush();
            fContext->flush();
            fDrawTarget = fContext->getTextTarget(fGrPaint);
            fMaxVertices = kDefaultRequestedVerts;
            
            fDrawTarget->geometryHints(vertexLayout,
                                       &fMaxVertices,
                                       NULL);
        }

        int maxQuadVertices = 4 * fContext->getQuadIndexBuffer()->maxQuads();
        if (fMaxVertices < kMinRequestedVerts) {
            fMaxVertices = kDefaultRequestedVerts;
        } else if (fMaxVertices > maxQuadVertices) {
            
            fMaxVertices = maxQuadVertices;
        }
        bool success = fDrawTarget->reserveVertexAndIndexSpace(
                                                   vertexLayout,
                                                   fMaxVertices,
                                                   0,
                                                   vertexBuff,
                                                   NULL);
        GrAlwaysAssert(success);
    }
}
