







#include "GrAAHairLinePathRenderer.h"

#include "GrContext.h"
#include "GrDrawState.h"
#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrGpu.h"
#include "GrIndexBuffer.h"
#include "GrPathUtils.h"
#include "GrTBackendEffectFactory.h"
#include "SkGeometry.h"
#include "SkStroke.h"
#include "SkTemplates.h"

#include "gl/GrGLEffect.h"
#include "gl/GrGLSL.h"

namespace {



static const int kVertsPerQuad = 5;
static const int kIdxsPerQuad = 9;

static const int kVertsPerLineSeg = 4;
static const int kIdxsPerLineSeg = 6;

static const int kNumQuadsInIdxBuffer = 256;
static const size_t kQuadIdxSBufize = kIdxsPerQuad *
                                      sizeof(uint16_t) *
                                      kNumQuadsInIdxBuffer;

bool push_quad_index_data(GrIndexBuffer* qIdxBuffer) {
    uint16_t* data = (uint16_t*) qIdxBuffer->lock();
    bool tempData = NULL == data;
    if (tempData) {
        data = SkNEW_ARRAY(uint16_t, kNumQuadsInIdxBuffer * kIdxsPerQuad);
    }
    for (int i = 0; i < kNumQuadsInIdxBuffer; ++i) {

        
        
        
        
        
        
        
        
        
        
        
        
        int baseIdx = i * kIdxsPerQuad;
        uint16_t baseVert = (uint16_t)(i * kVertsPerQuad);
        data[0 + baseIdx] = baseVert + 0; 
        data[1 + baseIdx] = baseVert + 1; 
        data[2 + baseIdx] = baseVert + 2; 
        data[3 + baseIdx] = baseVert + 2; 
        data[4 + baseIdx] = baseVert + 4; 
        data[5 + baseIdx] = baseVert + 3; 
        data[6 + baseIdx] = baseVert + 1; 
        data[7 + baseIdx] = baseVert + 4; 
        data[8 + baseIdx] = baseVert + 2; 
    }
    if (tempData) {
        bool ret = qIdxBuffer->updateData(data, kQuadIdxSBufize);
        delete[] data;
        return ret;
    } else {
        qIdxBuffer->unlock();
        return true;
    }
}
}

GrPathRenderer* GrAAHairLinePathRenderer::Create(GrContext* context) {
    const GrIndexBuffer* lIdxBuffer = context->getQuadIndexBuffer();
    if (NULL == lIdxBuffer) {
        return NULL;
    }
    GrGpu* gpu = context->getGpu();
    GrIndexBuffer* qIdxBuf = gpu->createIndexBuffer(kQuadIdxSBufize, false);
    SkAutoTUnref<GrIndexBuffer> qIdxBuffer(qIdxBuf);
    if (NULL == qIdxBuf ||
        !push_quad_index_data(qIdxBuf)) {
        return NULL;
    }
    return SkNEW_ARGS(GrAAHairLinePathRenderer,
                      (context, lIdxBuffer, qIdxBuf));
}

GrAAHairLinePathRenderer::GrAAHairLinePathRenderer(
                                        const GrContext* context,
                                        const GrIndexBuffer* linesIndexBuffer,
                                        const GrIndexBuffer* quadsIndexBuffer) {
    fLinesIndexBuffer = linesIndexBuffer;
    linesIndexBuffer->ref();
    fQuadsIndexBuffer = quadsIndexBuffer;
    quadsIndexBuffer->ref();
}

GrAAHairLinePathRenderer::~GrAAHairLinePathRenderer() {
    fLinesIndexBuffer->unref();
    fQuadsIndexBuffer->unref();
}

namespace {

typedef SkTArray<SkPoint, true> PtArray;
#define PREALLOC_PTARRAY(N) SkSTArray<(N),SkPoint, true>
typedef SkTArray<int, true> IntArray;


int get_float_exp(float x) {
    GR_STATIC_ASSERT(sizeof(int) == sizeof(float));
#if GR_DEBUG
    static bool tested;
    if (!tested) {
        tested = true;
        GrAssert(get_float_exp(0.25f) == -2);
        GrAssert(get_float_exp(0.3f) == -2);
        GrAssert(get_float_exp(0.5f) == -1);
        GrAssert(get_float_exp(1.f) == 0);
        GrAssert(get_float_exp(2.f) == 1);
        GrAssert(get_float_exp(2.5f) == 1);
        GrAssert(get_float_exp(8.f) == 3);
        GrAssert(get_float_exp(100.f) == 6);
        GrAssert(get_float_exp(1000.f) == 9);
        GrAssert(get_float_exp(1024.f) == 10);
        GrAssert(get_float_exp(3000000.f) == 21);
    }
#endif
    const int* iptr = (const int*)&x;
    return (((*iptr) & 0x7f800000) >> 23) - 127;
}



int num_quad_subdivs(const SkPoint p[3]) {
    static const SkScalar gDegenerateToLineTol = SK_Scalar1;
    static const SkScalar gDegenerateToLineTolSqd =
        SkScalarMul(gDegenerateToLineTol, gDegenerateToLineTol);

    if (p[0].distanceToSqd(p[1]) < gDegenerateToLineTolSqd ||
        p[1].distanceToSqd(p[2]) < gDegenerateToLineTolSqd) {
        return -1;
    }

    SkScalar dsqd = p[1].distanceToLineBetweenSqd(p[0], p[2]);
    if (dsqd < gDegenerateToLineTolSqd) {
        return -1;
    }

    if (p[2].distanceToLineBetweenSqd(p[1], p[0]) < gDegenerateToLineTolSqd) {
        return -1;
    }

    
    
    
    
    static const SkScalar gSubdivTol = 175 * SK_Scalar1;

    if (dsqd <= SkScalarMul(gSubdivTol, gSubdivTol)) {
        return 0;
    } else {
        static const int kMaxSub = 4;
        
        
        

#ifdef SK_SCALAR_IS_FLOAT
        
        int log = get_float_exp(dsqd/(gSubdivTol*gSubdivTol)) + 1;
        log = GrMin(GrMax(0, log), kMaxSub);
        return log;
#else
        SkScalar log = SkScalarLog(
                          SkScalarDiv(dsqd,
                                      SkScalarMul(gSubdivTol, gSubdivTol)));
        static const SkScalar conv = SkScalarInvert(SkScalarLog(2));
        log = SkScalarMul(log, conv);
        return  GrMin(GrMax(0, SkScalarCeilToInt(log)),kMaxSub);
#endif
    }
}










int generate_lines_and_quads(const SkPath& path,
                             const SkMatrix& m,
                             const GrIRect& devClipBounds,
                             PtArray* lines,
                             PtArray* quads,
                             IntArray* quadSubdivCnts) {
    SkPath::Iter iter(path, false);

    int totalQuadCount = 0;
    GrRect bounds;
    GrIRect ibounds;

    bool persp = m.hasPerspective();

    for (;;) {
        GrPoint pts[4];
        GrPoint devPts[4];
        GrPathCmd cmd = (GrPathCmd)iter.next(pts);
        switch (cmd) {
            case kMove_PathCmd:
                break;
            case kLine_PathCmd:
                m.mapPoints(devPts, pts, 2);
                bounds.setBounds(devPts, 2);
                bounds.outset(SK_Scalar1, SK_Scalar1);
                bounds.roundOut(&ibounds);
                if (SkIRect::Intersects(devClipBounds, ibounds)) {
                    SkPoint* pts = lines->push_back_n(2);
                    pts[0] = devPts[0];
                    pts[1] = devPts[1];
                }
                break;
            case kQuadratic_PathCmd:
                m.mapPoints(devPts, pts, 3);
                bounds.setBounds(devPts, 3);
                bounds.outset(SK_Scalar1, SK_Scalar1);
                bounds.roundOut(&ibounds);
                if (SkIRect::Intersects(devClipBounds, ibounds)) {
                    int subdiv = num_quad_subdivs(devPts);
                    GrAssert(subdiv >= -1);
                    if (-1 == subdiv) {
                        SkPoint* pts = lines->push_back_n(4);
                        pts[0] = devPts[0];
                        pts[1] = devPts[1];
                        pts[2] = devPts[1];
                        pts[3] = devPts[2];
                    } else {
                        
                        SkPoint* qPts = persp ? pts : devPts;
                        SkPoint* pts = quads->push_back_n(3);
                        pts[0] = qPts[0];
                        pts[1] = qPts[1];
                        pts[2] = qPts[2];
                        quadSubdivCnts->push_back() = subdiv;
                        totalQuadCount += 1 << subdiv;
                    }
                }
                break;
            case kCubic_PathCmd:
                m.mapPoints(devPts, pts, 4);
                bounds.setBounds(devPts, 4);
                bounds.outset(SK_Scalar1, SK_Scalar1);
                bounds.roundOut(&ibounds);
                if (SkIRect::Intersects(devClipBounds, ibounds)) {
                    PREALLOC_PTARRAY(32) q;
                    
                    static const SkPath::Direction kDummyDir = SkPath::kCCW_Direction;
                    
                    
                    if (persp) {
                        SkScalar tolScale =
                            GrPathUtils::scaleToleranceToSrc(SK_Scalar1, m,
                                                             path.getBounds());
                        GrPathUtils::convertCubicToQuads(pts, tolScale, false, kDummyDir, &q);
                    } else {
                        GrPathUtils::convertCubicToQuads(devPts, SK_Scalar1, false, kDummyDir, &q);
                    }
                    for (int i = 0; i < q.count(); i += 3) {
                        SkPoint* qInDevSpace;
                        
                        
                        if (persp) {
                            m.mapPoints(devPts, &q[i], 3);
                            bounds.setBounds(devPts, 3);
                            qInDevSpace = devPts;
                        } else {
                            bounds.setBounds(&q[i], 3);
                            qInDevSpace = &q[i];
                        }
                        bounds.outset(SK_Scalar1, SK_Scalar1);
                        bounds.roundOut(&ibounds);
                        if (SkIRect::Intersects(devClipBounds, ibounds)) {
                            int subdiv = num_quad_subdivs(qInDevSpace);
                            GrAssert(subdiv >= -1);
                            if (-1 == subdiv) {
                                SkPoint* pts = lines->push_back_n(4);
                                
                                pts[0] = qInDevSpace[0];
                                pts[1] = qInDevSpace[1];
                                pts[2] = qInDevSpace[1];
                                pts[3] = qInDevSpace[2];
                            } else {
                                SkPoint* pts = quads->push_back_n(3);
                                
                                
                                pts[0] = q[0 + i];
                                pts[1] = q[1 + i];
                                pts[2] = q[2 + i];
                                quadSubdivCnts->push_back() = subdiv;
                                totalQuadCount += 1 << subdiv;
                            }
                        }
                    }
                }
                break;
            case kClose_PathCmd:
                break;
            case kEnd_PathCmd:
                return totalQuadCount;
        }
    }
}

struct Vertex {
    GrPoint fPos;
    union {
        struct {
            SkScalar fA;
            SkScalar fB;
            SkScalar fC;
        } fLine;
        GrVec   fQuadCoord;
        struct {
            SkScalar fBogus[4];
        };
    };
};
GR_STATIC_ASSERT(sizeof(Vertex) == 3 * sizeof(GrPoint));

void intersect_lines(const SkPoint& ptA, const SkVector& normA,
                     const SkPoint& ptB, const SkVector& normB,
                     SkPoint* result) {

    SkScalar lineAW = -normA.dot(ptA);
    SkScalar lineBW = -normB.dot(ptB);

    SkScalar wInv = SkScalarMul(normA.fX, normB.fY) -
                    SkScalarMul(normA.fY, normB.fX);
    wInv = SkScalarInvert(wInv);

    result->fX = SkScalarMul(normA.fY, lineBW) - SkScalarMul(lineAW, normB.fY);
    result->fX = SkScalarMul(result->fX, wInv);

    result->fY = SkScalarMul(lineAW, normB.fX) - SkScalarMul(normA.fX, lineBW);
    result->fY = SkScalarMul(result->fY, wInv);
}

void bloat_quad(const SkPoint qpts[3], const SkMatrix* toDevice,
                const SkMatrix* toSrc, Vertex verts[kVertsPerQuad]) {
    GrAssert(!toDevice == !toSrc);
    
    SkPoint a = qpts[0];
    SkPoint b = qpts[1];
    SkPoint c = qpts[2];

    
    GrPathUtils::QuadUVMatrix DevToUV(qpts);

    if (toDevice) {
        toDevice->mapPoints(&a, 1);
        toDevice->mapPoints(&b, 1);
        toDevice->mapPoints(&c, 1);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    Vertex& a0 = verts[0];
    Vertex& a1 = verts[1];
    Vertex& b0 = verts[2];
    Vertex& c0 = verts[3];
    Vertex& c1 = verts[4];

    SkVector ab = b;
    ab -= a;
    SkVector ac = c;
    ac -= a;
    SkVector cb = b;
    cb -= c;

    
    GrAssert(ab.length() > 0 && cb.length() > 0);

    ab.normalize();
    SkVector abN;
    abN.setOrthog(ab, SkVector::kLeft_Side);
    if (abN.dot(ac) > 0) {
        abN.negate();
    }

    cb.normalize();
    SkVector cbN;
    cbN.setOrthog(cb, SkVector::kLeft_Side);
    if (cbN.dot(ac) < 0) {
        cbN.negate();
    }

    a0.fPos = a;
    a0.fPos += abN;
    a1.fPos = a;
    a1.fPos -= abN;

    c0.fPos = c;
    c0.fPos += cbN;
    c1.fPos = c;
    c1.fPos -= cbN;

    intersect_lines(a0.fPos, abN, c0.fPos, cbN, &b0.fPos);

    if (toSrc) {
        toSrc->mapPointsWithStride(&verts[0].fPos, sizeof(Vertex), kVertsPerQuad);
    }
    DevToUV.apply<kVertsPerQuad, sizeof(Vertex), sizeof(GrPoint)>(verts);
}

void add_quads(const SkPoint p[3],
               int subdiv,
               const SkMatrix* toDevice,
               const SkMatrix* toSrc,
               Vertex** vert) {
    GrAssert(subdiv >= 0);
    if (subdiv) {
        SkPoint newP[5];
        SkChopQuadAtHalf(p, newP);
        add_quads(newP + 0, subdiv-1, toDevice, toSrc, vert);
        add_quads(newP + 2, subdiv-1, toDevice, toSrc, vert);
    } else {
        bloat_quad(p, toDevice, toSrc, *vert);
        *vert += kVertsPerQuad;
    }
}

void add_line(const SkPoint p[2],
              int rtHeight,
              const SkMatrix* toSrc,
              Vertex** vert) {
    const SkPoint& a = p[0];
    const SkPoint& b = p[1];

    SkVector orthVec = b;
    orthVec -= a;

    if (orthVec.setLength(SK_Scalar1)) {
        orthVec.setOrthog(orthVec);

        SkScalar lineC = -(a.dot(orthVec));
        for (int i = 0; i < kVertsPerLineSeg; ++i) {
            (*vert)[i].fPos = (i < 2) ? a : b;
            if (0 == i || 3 == i) {
                (*vert)[i].fPos -= orthVec;
            } else {
                (*vert)[i].fPos += orthVec;
            }
            (*vert)[i].fLine.fA = orthVec.fX;
            (*vert)[i].fLine.fB = orthVec.fY;
            (*vert)[i].fLine.fC = lineC;
        }
        if (NULL != toSrc) {
            toSrc->mapPointsWithStride(&(*vert)->fPos,
                                       sizeof(Vertex),
                                       kVertsPerLineSeg);
        }
    } else {
        
        (*vert)[0].fPos.set(SK_ScalarMax, SK_ScalarMax);
        (*vert)[1].fPos.set(SK_ScalarMax, SK_ScalarMax);
        (*vert)[2].fPos.set(SK_ScalarMax, SK_ScalarMax);
        (*vert)[3].fPos.set(SK_ScalarMax, SK_ScalarMax);
    }

    *vert += kVertsPerLineSeg;
}

}










class HairQuadEdgeEffect : public GrEffect {
public:

    static GrEffectRef* Create() {
        
        static GrEffectRef* gHairQuadEdgeEffectRef = 
            CreateEffectRef(AutoEffectUnref(SkNEW(HairQuadEdgeEffect)));
        static SkAutoTUnref<GrEffectRef> gUnref(gHairQuadEdgeEffectRef);

        gHairQuadEdgeEffectRef->ref();
        return gHairQuadEdgeEffectRef;
    }

    virtual ~HairQuadEdgeEffect() {}

    static const char* Name() { return "HairQuadEdge"; }

    virtual void getConstantColorComponents(GrColor* color, 
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<HairQuadEdgeEffect>::getInstance();
    }

    class GLEffect : public GrGLEffect {
    public:
        GLEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&)
            : INHERITED (factory) {}

        virtual void emitCode(GrGLShaderBuilder* builder,
                              const GrDrawEffect& drawEffect,
                              EffectKey key,
                              const char* outputColor,
                              const char* inputColor,
                              const TextureSamplerArray& samplers) SK_OVERRIDE {
            const char *vsName, *fsName;
            const SkString* attrName =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[0]);
            builder->fsCodeAppendf("\t\tfloat edgeAlpha;\n");

            SkAssertResult(builder->enableFeature(
                                              GrGLShaderBuilder::kStandardDerivatives_GLSLFeature));
            builder->addVarying(kVec4f_GrSLType, "HairQuadEdge", &vsName, &fsName);

            builder->fsCodeAppendf("\t\tvec2 duvdx = dFdx(%s.xy);\n", fsName);
            builder->fsCodeAppendf("\t\tvec2 duvdy = dFdy(%s.xy);\n", fsName);
            builder->fsCodeAppendf("\t\tvec2 gF = vec2(2.0*%s.x*duvdx.x - duvdx.y,\n"
                                   "\t\t               2.0*%s.x*duvdy.x - duvdy.y);\n",
                                   fsName, fsName);
            builder->fsCodeAppendf("\t\tedgeAlpha = (%s.x*%s.x - %s.y);\n", fsName, fsName,
                                   fsName);
            builder->fsCodeAppend("\t\tedgeAlpha = sqrt(edgeAlpha*edgeAlpha / dot(gF, gF));\n");
            builder->fsCodeAppend("\t\tedgeAlpha = max(1.0 - edgeAlpha, 0.0);\n");

            SkString modulate;
            GrGLSLModulate4f(&modulate, inputColor, "edgeAlpha");
            builder->fsCodeAppendf("\t%s = %s;\n", outputColor, modulate.c_str());

            builder->vsCodeAppendf("\t%s = %s;\n", vsName, attrName->c_str());
        }

        static inline EffectKey GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
            return 0x0;
        }

        virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE {}

    private:
        typedef GrGLEffect INHERITED;
    };

private:
    HairQuadEdgeEffect() { 
        this->addVertexAttrib(kVec4f_GrSLType); 
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        return true;
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(HairQuadEdgeEffect);

GrEffectRef* HairQuadEdgeEffect::TestCreate(SkMWCRandom* random,
                                            GrContext*,
                                            const GrDrawTargetCaps& caps,
                                            GrTexture*[]) {
    
    return caps.shaderDerivativeSupport() ? HairQuadEdgeEffect::Create() : NULL;}







class HairLineEdgeEffect : public GrEffect {
public:

    static GrEffectRef* Create() {
        
        static GrEffectRef* gHairLineEdgeEffectRef = 
            CreateEffectRef(AutoEffectUnref(SkNEW(HairLineEdgeEffect)));
        static SkAutoTUnref<GrEffectRef> gUnref(gHairLineEdgeEffectRef);

        gHairLineEdgeEffectRef->ref();
        return gHairLineEdgeEffectRef;
    }

    virtual ~HairLineEdgeEffect() {}

    static const char* Name() { return "HairLineEdge"; }

    virtual void getConstantColorComponents(GrColor* color, 
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<HairLineEdgeEffect>::getInstance();
    }

    class GLEffect : public GrGLEffect {
    public:
        GLEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&)
            : INHERITED (factory) {}

        virtual void emitCode(GrGLShaderBuilder* builder,
                              const GrDrawEffect& drawEffect,
                              EffectKey key,
                              const char* outputColor,
                              const char* inputColor,
                              const TextureSamplerArray& samplers) SK_OVERRIDE {
            const char *vsName, *fsName;
            const SkString* attrName =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[0]);
            builder->fsCodeAppendf("\t\tfloat edgeAlpha;\n");

            builder->addVarying(kVec4f_GrSLType, "HairLineEdge", &vsName, &fsName);

            builder->fsCodeAppendf("\t\tedgeAlpha = abs(dot(vec3(%s.xy,1), %s.xyz));\n",
                                   builder->fragmentPosition(), fsName);
            builder->fsCodeAppendf("\t\tedgeAlpha = max(1.0 - edgeAlpha, 0.0);\n");

            SkString modulate;
            GrGLSLModulate4f(&modulate, inputColor, "edgeAlpha");
            builder->fsCodeAppendf("\t%s = %s;\n", outputColor, modulate.c_str());

            builder->vsCodeAppendf("\t%s = %s;\n", vsName, attrName->c_str());
        }

        static inline EffectKey GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
            return 0x0;
        }

        virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE {}

    private:
        typedef GrGLEffect INHERITED;
    };

private:
    HairLineEdgeEffect() { 
        this->addVertexAttrib(kVec4f_GrSLType); 
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        return true;
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(HairLineEdgeEffect);

GrEffectRef* HairLineEdgeEffect::TestCreate(SkMWCRandom* random,
                                            GrContext*,
                                            const GrDrawTargetCaps& caps,
                                            GrTexture*[]) {
    return HairLineEdgeEffect::Create();
}



bool GrAAHairLinePathRenderer::createGeom(
            const SkPath& path,
            GrDrawTarget* target,
            int* lineCnt,
            int* quadCnt,
            GrDrawTarget::AutoReleaseGeometry* arg) {
    GrDrawState* drawState = target->drawState();
    int rtHeight = drawState->getRenderTarget()->height();

    GrIRect devClipBounds;
    target->getClip()->getConservativeBounds(drawState->getRenderTarget(),
                                             &devClipBounds);

    
    static const GrVertexAttrib kAttribs[] = {
        {kVec2f_GrVertexAttribType, 0,                  kPosition_GrVertexAttribBinding},
        {kVec4f_GrVertexAttribType, sizeof(GrPoint),    kEffect_GrVertexAttribBinding}
    };
    SkMatrix viewM = drawState->getViewMatrix();

    PREALLOC_PTARRAY(128) lines;
    PREALLOC_PTARRAY(128) quads;
    IntArray qSubdivs;
    *quadCnt = generate_lines_and_quads(path, viewM, devClipBounds,
                                        &lines, &quads, &qSubdivs);

    *lineCnt = lines.count() / 2;
    int vertCnt = kVertsPerLineSeg * *lineCnt + kVertsPerQuad * *quadCnt;

    target->drawState()->setVertexAttribs(kAttribs, SK_ARRAY_COUNT(kAttribs));
    GrAssert(sizeof(Vertex) == target->getDrawState().getVertexSize());

    if (!arg->set(target, vertCnt, 0)) {
        return false;
    }

    Vertex* verts = reinterpret_cast<Vertex*>(arg->vertices());

    const SkMatrix* toDevice = NULL;
    const SkMatrix* toSrc = NULL;
    SkMatrix ivm;

    if (viewM.hasPerspective()) {
        if (viewM.invert(&ivm)) {
            toDevice = &viewM;
            toSrc = &ivm;
        }
    }

    for (int i = 0; i < *lineCnt; ++i) {
        add_line(&lines[2*i], rtHeight, toSrc, &verts);
    }

    int unsubdivQuadCnt = quads.count() / 3;
    for (int i = 0; i < unsubdivQuadCnt; ++i) {
        GrAssert(qSubdivs[i] >= 0);
        add_quads(&quads[3*i], qSubdivs[i], toDevice, toSrc, &verts);
    }

    return true;
}

bool GrAAHairLinePathRenderer::canDrawPath(const SkPath& path,
                                           const SkStrokeRec& stroke,
                                           const GrDrawTarget* target,
                                           bool antiAlias) const {
    if (!stroke.isHairlineStyle() || !antiAlias) {
        return false;
    }

    static const uint32_t gReqDerivMask = SkPath::kCubic_SegmentMask |
                                          SkPath::kQuad_SegmentMask;
    if (!target->caps()->shaderDerivativeSupport() &&
        (gReqDerivMask & path.getSegmentMasks())) {
        return false;
    }
    return true;
}

bool GrAAHairLinePathRenderer::onDrawPath(const SkPath& path,
                                          const SkStrokeRec&,
                                          GrDrawTarget* target,
                                          bool antiAlias) {

    int lineCnt;
    int quadCnt;
    GrDrawTarget::AutoReleaseGeometry arg;
    if (!this->createGeom(path,
                          target,
                          &lineCnt,
                          &quadCnt,
                          &arg)) {
        return false;
    }

    GrDrawTarget::AutoStateRestore asr(target, GrDrawTarget::kPreserve_ASRInit);
    GrDrawState* drawState = target->drawState();

    GrDrawState::AutoDeviceCoordDraw adcd;
    
    
    if (!drawState->getViewMatrix().hasPerspective()) {
        adcd.set(drawState);
        if (!adcd.succeeded()) {
            return false;
        }
    }

    
    

    enum {
        
        
        
        kEdgeEffectStage = GrPaint::kTotalStages,
    };
    static const int kEdgeAttrIndex = 1;

    GrEffectRef* hairLineEffect = HairLineEdgeEffect::Create();
    GrEffectRef* hairQuadEffect = HairQuadEdgeEffect::Create();

    target->setIndexSourceToBuffer(fLinesIndexBuffer);
    int lines = 0;
    int nBufLines = fLinesIndexBuffer->maxQuads();
    drawState->setEffect(kEdgeEffectStage, hairLineEffect, kEdgeAttrIndex)->unref();
    while (lines < lineCnt) {
        int n = GrMin(lineCnt - lines, nBufLines);
        target->drawIndexed(kTriangles_GrPrimitiveType,
                            kVertsPerLineSeg*lines,    
                            0,                         
                            kVertsPerLineSeg*n,        
                            kIdxsPerLineSeg*n);        
        lines += n;
    }

    target->setIndexSourceToBuffer(fQuadsIndexBuffer);
    int quads = 0;
    drawState->setEffect(kEdgeEffectStage, hairQuadEffect, kEdgeAttrIndex)->unref();
    while (quads < quadCnt) {
        int n = GrMin(quadCnt - quads, kNumQuadsInIdxBuffer);
        target->drawIndexed(kTriangles_GrPrimitiveType,
                            4 * lineCnt + kVertsPerQuad*quads, 
                            0,                                 
                            kVertsPerQuad*n,                   
                            kIdxsPerQuad*n);                   
        quads += n;
    }

    return true;
}
