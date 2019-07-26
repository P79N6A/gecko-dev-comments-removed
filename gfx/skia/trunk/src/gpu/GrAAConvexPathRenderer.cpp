







#include "GrAAConvexPathRenderer.h"

#include "GrContext.h"
#include "GrDrawState.h"
#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrPathUtils.h"
#include "GrTBackendEffectFactory.h"
#include "SkString.h"
#include "SkStrokeRec.h"
#include "SkTrace.h"

#include "gl/GrGLEffect.h"
#include "gl/GrGLSL.h"
#include "gl/GrGLVertexEffect.h"

#include "effects/GrVertexEffect.h"

GrAAConvexPathRenderer::GrAAConvexPathRenderer() {
}

struct Segment {
    enum {
        
        kLine = 0,
        kQuad = 1,
    } fType;

    
    GrPoint fPts[2];
    
    GrVec fNorms[2];
    
    
    GrVec fMid;

    int countPoints() {
        GR_STATIC_ASSERT(0 == kLine && 1 == kQuad);
        return fType + 1;
    }
    const SkPoint& endPt() const {
        GR_STATIC_ASSERT(0 == kLine && 1 == kQuad);
        return fPts[fType];
    };
    const SkPoint& endNorm() const {
        GR_STATIC_ASSERT(0 == kLine && 1 == kQuad);
        return fNorms[fType];
    };
};

typedef SkTArray<Segment, true> SegmentArray;

static void center_of_mass(const SegmentArray& segments, SkPoint* c) {
    SkScalar area = 0;
    SkPoint center = {0, 0};
    int count = segments.count();
    SkPoint p0 = {0, 0};
    if (count > 2) {
        
        
        
        p0 = segments[0].endPt();
        SkPoint pi;
        SkPoint pj;
        
        
        
        pj = segments[1].endPt() - p0;
        for (int i = 1; i < count - 1; ++i) {
            pi = pj;
            const SkPoint pj = segments[i + 1].endPt() - p0;

            SkScalar t = SkScalarMul(pi.fX, pj.fY) - SkScalarMul(pj.fX, pi.fY);
            area += t;
            center.fX += (pi.fX + pj.fX) * t;
            center.fY += (pi.fY + pj.fY) * t;

        }
    }
    
    
    if (SkScalarNearlyZero(area)) {
        SkPoint avg;
        avg.set(0, 0);
        for (int i = 0; i < count; ++i) {
            const SkPoint& pt = segments[i].endPt();
            avg.fX += pt.fX;
            avg.fY += pt.fY;
        }
        SkScalar denom = SK_Scalar1 / count;
        avg.scale(denom);
        *c = avg;
    } else {
        area *= 3;
        area = SkScalarDiv(SK_Scalar1, area);
        center.fX = SkScalarMul(center.fX, area);
        center.fY = SkScalarMul(center.fY, area);
        
        *c = center + p0;
    }
    SkASSERT(!SkScalarIsNaN(c->fX) && !SkScalarIsNaN(c->fY));
}

static void compute_vectors(SegmentArray* segments,
                            SkPoint* fanPt,
                            SkPath::Direction dir,
                            int* vCount,
                            int* iCount) {
    center_of_mass(*segments, fanPt);
    int count = segments->count();

    
    GrPoint::Side normSide;
    if (dir == SkPath::kCCW_Direction) {
        normSide = GrPoint::kRight_Side;
    } else {
        normSide = GrPoint::kLeft_Side;
    }

    *vCount = 0;
    *iCount = 0;
    
    for (int a = 0; a < count; ++a) {
        Segment& sega = (*segments)[a];
        int b = (a + 1) % count;
        Segment& segb = (*segments)[b];

        const GrPoint* prevPt = &sega.endPt();
        int n = segb.countPoints();
        for (int p = 0; p < n; ++p) {
            segb.fNorms[p] = segb.fPts[p] - *prevPt;
            segb.fNorms[p].normalize();
            segb.fNorms[p].setOrthog(segb.fNorms[p], normSide);
            prevPt = &segb.fPts[p];
        }
        if (Segment::kLine == segb.fType) {
            *vCount += 5;
            *iCount += 9;
        } else {
            *vCount += 6;
            *iCount += 12;
        }
    }

    
    
    for (int a = 0; a < count; ++a) {
        const Segment& sega = (*segments)[a];
        int b = (a + 1) % count;
        Segment& segb = (*segments)[b];
        segb.fMid = segb.fNorms[0] + sega.endNorm();
        segb.fMid.normalize();
        
        *vCount += 4;
        *iCount += 6;
    }
}

struct DegenerateTestData {
    DegenerateTestData() { fStage = kInitial; }
    bool isDegenerate() const { return kNonDegenerate != fStage; }
    enum {
        kInitial,
        kPoint,
        kLine,
        kNonDegenerate
    }           fStage;
    GrPoint     fFirstPoint;
    GrVec       fLineNormal;
    SkScalar    fLineC;
};

static const SkScalar kClose = (SK_Scalar1 / 16);
static const SkScalar kCloseSqd = SkScalarMul(kClose, kClose);

static void update_degenerate_test(DegenerateTestData* data, const GrPoint& pt) {
    switch (data->fStage) {
        case DegenerateTestData::kInitial:
            data->fFirstPoint = pt;
            data->fStage = DegenerateTestData::kPoint;
            break;
        case DegenerateTestData::kPoint:
            if (pt.distanceToSqd(data->fFirstPoint) > kCloseSqd) {
                data->fLineNormal = pt - data->fFirstPoint;
                data->fLineNormal.normalize();
                data->fLineNormal.setOrthog(data->fLineNormal);
                data->fLineC = -data->fLineNormal.dot(data->fFirstPoint);
                data->fStage = DegenerateTestData::kLine;
            }
            break;
        case DegenerateTestData::kLine:
            if (SkScalarAbs(data->fLineNormal.dot(pt) + data->fLineC) > kClose) {
                data->fStage = DegenerateTestData::kNonDegenerate;
            }
        case DegenerateTestData::kNonDegenerate:
            break;
        default:
            GrCrash("Unexpected degenerate test stage.");
    }
}

static inline bool get_direction(const SkPath& path, const SkMatrix& m, SkPath::Direction* dir) {
    if (!path.cheapComputeDirection(dir)) {
        return false;
    }
    
    SkASSERT(!m.hasPerspective());
    SkScalar det2x2 = SkScalarMul(m.get(SkMatrix::kMScaleX), m.get(SkMatrix::kMScaleY)) -
                      SkScalarMul(m.get(SkMatrix::kMSkewX), m.get(SkMatrix::kMSkewY));
    if (det2x2 < 0) {
        *dir = SkPath::OppositeDirection(*dir);
    }
    return true;
}

static inline void add_line_to_segment(const SkPoint& pt,
                                       SegmentArray* segments,
                                       SkRect* devBounds) {
    segments->push_back();
    segments->back().fType = Segment::kLine;
    segments->back().fPts[0] = pt;
    devBounds->growToInclude(pt.fX, pt.fY);
}

#ifdef SK_DEBUG
static inline bool contains_inclusive(const SkRect& rect, const SkPoint& p) {
    return p.fX >= rect.fLeft && p.fX <= rect.fRight && p.fY >= rect.fTop && p.fY <= rect.fBottom;
}
#endif

static inline void add_quad_segment(const SkPoint pts[3],
                                    SegmentArray* segments,
                                    SkRect* devBounds) {
    if (pts[0].distanceToSqd(pts[1]) < kCloseSqd || pts[1].distanceToSqd(pts[2]) < kCloseSqd) {
        if (pts[0] != pts[2]) {
            add_line_to_segment(pts[2], segments, devBounds);
        }
    } else {
        segments->push_back();
        segments->back().fType = Segment::kQuad;
        segments->back().fPts[0] = pts[1];
        segments->back().fPts[1] = pts[2];
        SkASSERT(contains_inclusive(*devBounds, pts[0]));
        devBounds->growToInclude(pts + 1, 2);
    }
}

static inline void add_cubic_segments(const SkPoint pts[4],
                                      SkPath::Direction dir,
                                      SegmentArray* segments,
                                      SkRect* devBounds) {
    SkSTArray<15, SkPoint, true> quads;
    GrPathUtils::convertCubicToQuads(pts, SK_Scalar1, true, dir, &quads);
    int count = quads.count();
    for (int q = 0; q < count; q += 3) {
        add_quad_segment(&quads[q], segments, devBounds);
    }
}

static bool get_segments(const SkPath& path,
                         const SkMatrix& m,
                         SegmentArray* segments,
                         SkPoint* fanPt,
                         int* vCount,
                         int* iCount,
                         SkRect* devBounds) {
    SkPath::Iter iter(path, true);
    
    
    
    
    
    
    
    DegenerateTestData degenerateData;
    SkPath::Direction dir;
    
    if (!get_direction(path, m, &dir)) {
        return false;
    }

    for (;;) {
        GrPoint pts[4];
        SkPath::Verb verb = iter.next(pts);
        switch (verb) {
            case SkPath::kMove_Verb:
                m.mapPoints(pts, 1);
                update_degenerate_test(&degenerateData, pts[0]);
                devBounds->set(pts->fX, pts->fY, pts->fX, pts->fY);
                break;
            case SkPath::kLine_Verb: {
                m.mapPoints(&pts[1], 1);
                update_degenerate_test(&degenerateData, pts[1]);
                add_line_to_segment(pts[1], segments, devBounds);
                break;
            }
            case SkPath::kQuad_Verb:
                m.mapPoints(pts, 3);
                update_degenerate_test(&degenerateData, pts[1]);
                update_degenerate_test(&degenerateData, pts[2]);
                add_quad_segment(pts, segments, devBounds);
                break;
            case SkPath::kCubic_Verb: {
                m.mapPoints(pts, 4);
                update_degenerate_test(&degenerateData, pts[1]);
                update_degenerate_test(&degenerateData, pts[2]);
                update_degenerate_test(&degenerateData, pts[3]);
                add_cubic_segments(pts, dir, segments, devBounds);
                break;
            };
            case SkPath::kDone_Verb:
                if (degenerateData.isDegenerate()) {
                    return false;
                } else {
                    compute_vectors(segments, fanPt, dir, vCount, iCount);
                    return true;
                }
            default:
                break;
        }
    }
}

struct QuadVertex {
    GrPoint  fPos;
    GrPoint  fUV;
    SkScalar fD0;
    SkScalar fD1;
};

struct Draw {
    Draw() : fVertexCnt(0), fIndexCnt(0) {}
    int fVertexCnt;
    int fIndexCnt;
};

typedef SkTArray<Draw, true> DrawArray;

static void create_vertices(const SegmentArray&  segments,
                            const SkPoint& fanPt,
                            DrawArray*     draws,
                            QuadVertex*    verts,
                            uint16_t*      idxs) {
    Draw* draw = &draws->push_back();
    
    int* v = &draw->fVertexCnt;
    int* i = &draw->fIndexCnt;

    int count = segments.count();
    for (int a = 0; a < count; ++a) {
        const Segment& sega = segments[a];
        int b = (a + 1) % count;
        const Segment& segb = segments[b];

        
        
        int vCount = 4;
        if (Segment::kLine == segb.fType) {
            vCount += 5;
        } else {
            vCount += 6;
        }
        if (draw->fVertexCnt + vCount > (1 << 16)) {
            verts += *v;
            idxs += *i;
            draw = &draws->push_back();
            v = &draw->fVertexCnt;
            i = &draw->fIndexCnt;
        }

        
        verts[*v + 0].fPos = sega.endPt();
        verts[*v + 1].fPos = verts[*v + 0].fPos + sega.endNorm();
        verts[*v + 2].fPos = verts[*v + 0].fPos + segb.fMid;
        verts[*v + 3].fPos = verts[*v + 0].fPos + segb.fNorms[0];
        verts[*v + 0].fUV.set(0,0);
        verts[*v + 1].fUV.set(0,-SK_Scalar1);
        verts[*v + 2].fUV.set(0,-SK_Scalar1);
        verts[*v + 3].fUV.set(0,-SK_Scalar1);
        verts[*v + 0].fD0 = verts[*v + 0].fD1 = -SK_Scalar1;
        verts[*v + 1].fD0 = verts[*v + 1].fD1 = -SK_Scalar1;
        verts[*v + 2].fD0 = verts[*v + 2].fD1 = -SK_Scalar1;
        verts[*v + 3].fD0 = verts[*v + 3].fD1 = -SK_Scalar1;

        idxs[*i + 0] = *v + 0;
        idxs[*i + 1] = *v + 2;
        idxs[*i + 2] = *v + 1;
        idxs[*i + 3] = *v + 0;
        idxs[*i + 4] = *v + 3;
        idxs[*i + 5] = *v + 2;

        *v += 4;
        *i += 6;

        if (Segment::kLine == segb.fType) {
            verts[*v + 0].fPos = fanPt;
            verts[*v + 1].fPos = sega.endPt();
            verts[*v + 2].fPos = segb.fPts[0];

            verts[*v + 3].fPos = verts[*v + 1].fPos + segb.fNorms[0];
            verts[*v + 4].fPos = verts[*v + 2].fPos + segb.fNorms[0];

            
            
            SkScalar dist = fanPt.distanceToLineBetween(verts[*v + 1].fPos,
                                                        verts[*v + 2].fPos);
            verts[*v + 0].fUV.set(0, dist);
            verts[*v + 1].fUV.set(0, 0);
            verts[*v + 2].fUV.set(0, 0);
            verts[*v + 3].fUV.set(0, -SK_Scalar1);
            verts[*v + 4].fUV.set(0, -SK_Scalar1);

            verts[*v + 0].fD0 = verts[*v + 0].fD1 = -SK_Scalar1;
            verts[*v + 1].fD0 = verts[*v + 1].fD1 = -SK_Scalar1;
            verts[*v + 2].fD0 = verts[*v + 2].fD1 = -SK_Scalar1;
            verts[*v + 3].fD0 = verts[*v + 3].fD1 = -SK_Scalar1;
            verts[*v + 4].fD0 = verts[*v + 4].fD1 = -SK_Scalar1;

            idxs[*i + 0] = *v + 0;
            idxs[*i + 1] = *v + 2;
            idxs[*i + 2] = *v + 1;

            idxs[*i + 3] = *v + 3;
            idxs[*i + 4] = *v + 1;
            idxs[*i + 5] = *v + 2;

            idxs[*i + 6] = *v + 4;
            idxs[*i + 7] = *v + 3;
            idxs[*i + 8] = *v + 2;

            *v += 5;
            *i += 9;
        } else {
            GrPoint qpts[] = {sega.endPt(), segb.fPts[0], segb.fPts[1]};

            GrVec midVec = segb.fNorms[0] + segb.fNorms[1];
            midVec.normalize();

            verts[*v + 0].fPos = fanPt;
            verts[*v + 1].fPos = qpts[0];
            verts[*v + 2].fPos = qpts[2];
            verts[*v + 3].fPos = qpts[0] + segb.fNorms[0];
            verts[*v + 4].fPos = qpts[2] + segb.fNorms[1];
            verts[*v + 5].fPos = qpts[1] + midVec;

            SkScalar c = segb.fNorms[0].dot(qpts[0]);
            verts[*v + 0].fD0 =  -segb.fNorms[0].dot(fanPt) + c;
            verts[*v + 1].fD0 =  0.f;
            verts[*v + 2].fD0 =  -segb.fNorms[0].dot(qpts[2]) + c;
            verts[*v + 3].fD0 = -SK_ScalarMax/100;
            verts[*v + 4].fD0 = -SK_ScalarMax/100;
            verts[*v + 5].fD0 = -SK_ScalarMax/100;

            c = segb.fNorms[1].dot(qpts[2]);
            verts[*v + 0].fD1 =  -segb.fNorms[1].dot(fanPt) + c;
            verts[*v + 1].fD1 =  -segb.fNorms[1].dot(qpts[0]) + c;
            verts[*v + 2].fD1 =  0.f;
            verts[*v + 3].fD1 = -SK_ScalarMax/100;
            verts[*v + 4].fD1 = -SK_ScalarMax/100;
            verts[*v + 5].fD1 = -SK_ScalarMax/100;

            GrPathUtils::QuadUVMatrix toUV(qpts);
            toUV.apply<6, sizeof(QuadVertex), sizeof(GrPoint)>(verts + *v);

            idxs[*i + 0] = *v + 3;
            idxs[*i + 1] = *v + 1;
            idxs[*i + 2] = *v + 2;
            idxs[*i + 3] = *v + 4;
            idxs[*i + 4] = *v + 3;
            idxs[*i + 5] = *v + 2;

            idxs[*i + 6] = *v + 5;
            idxs[*i + 7] = *v + 3;
            idxs[*i + 8] = *v + 4;

            idxs[*i +  9] = *v + 0;
            idxs[*i + 10] = *v + 2;
            idxs[*i + 11] = *v + 1;

            *v += 6;
            *i += 12;
        }
    }
}













class QuadEdgeEffect : public GrVertexEffect {
public:

    static GrEffectRef* Create() {
        GR_CREATE_STATIC_EFFECT(gQuadEdgeEffect, QuadEdgeEffect, ());
        gQuadEdgeEffect->ref();
        return gQuadEdgeEffect;
    }

    virtual ~QuadEdgeEffect() {}

    static const char* Name() { return "QuadEdge"; }

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<QuadEdgeEffect>::getInstance();
    }

    class GLEffect : public GrGLVertexEffect {
    public:
        GLEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&)
            : INHERITED (factory) {}

        virtual void emitCode(GrGLFullShaderBuilder* builder,
                              const GrDrawEffect& drawEffect,
                              EffectKey key,
                              const char* outputColor,
                              const char* inputColor,
                              const TransformedCoordsArray&,
                              const TextureSamplerArray& samplers) SK_OVERRIDE {
            const char *vsName, *fsName;
            const SkString* attrName =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[0]);
            builder->fsCodeAppendf("\t\tfloat edgeAlpha;\n");

            SkAssertResult(builder->enableFeature(
                                              GrGLShaderBuilder::kStandardDerivatives_GLSLFeature));
            builder->addVarying(kVec4f_GrSLType, "QuadEdge", &vsName, &fsName);

            
            builder->fsCodeAppendf("\t\tvec2 duvdx = dFdx(%s.xy);\n", fsName);
            builder->fsCodeAppendf("\t\tvec2 duvdy = dFdy(%s.xy);\n", fsName);
            builder->fsCodeAppendf("\t\tif (%s.z > 0.0 && %s.w > 0.0) {\n", fsName, fsName);
            
            builder->fsCodeAppendf("\t\t\tedgeAlpha = min(min(%s.z, %s.w) + 0.5, 1.0);\n", fsName,
                                    fsName);
            builder->fsCodeAppendf ("\t\t} else {\n");
            builder->fsCodeAppendf("\t\t\tvec2 gF = vec2(2.0*%s.x*duvdx.x - duvdx.y,\n"
                                   "\t\t\t               2.0*%s.x*duvdy.x - duvdy.y);\n",
                                   fsName, fsName);
            builder->fsCodeAppendf("\t\t\tedgeAlpha = (%s.x*%s.x - %s.y);\n", fsName, fsName,
                                    fsName);
            builder->fsCodeAppendf("\t\t\tedgeAlpha = "
                                   "clamp(0.5 - edgeAlpha / length(gF), 0.0, 1.0);\n\t\t}\n");


            builder->fsCodeAppendf("\t%s = %s;\n", outputColor,
                                   (GrGLSLExpr4(inputColor) * GrGLSLExpr1("edgeAlpha")).c_str());

            builder->vsCodeAppendf("\t%s = %s;\n", vsName, attrName->c_str());
        }

        static inline EffectKey GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
            return 0x0;
        }

        virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE {}

    private:
        typedef GrGLVertexEffect INHERITED;
    };

private:
    QuadEdgeEffect() {
        this->addVertexAttrib(kVec4f_GrSLType);
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        return true;
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(QuadEdgeEffect);

GrEffectRef* QuadEdgeEffect::TestCreate(SkRandom* random,
                                        GrContext*,
                                        const GrDrawTargetCaps& caps,
                                        GrTexture*[]) {
    
    return caps.shaderDerivativeSupport() ? QuadEdgeEffect::Create() : NULL;
}



bool GrAAConvexPathRenderer::canDrawPath(const SkPath& path,
                                         const SkStrokeRec& stroke,
                                         const GrDrawTarget* target,
                                         bool antiAlias) const {
    return (target->caps()->shaderDerivativeSupport() && antiAlias &&
            stroke.isFillStyle() && !path.isInverseFillType() && path.isConvex());
}

namespace {


extern const GrVertexAttrib gPathAttribs[] = {
    {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
    {kVec4f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding}
};

};

bool GrAAConvexPathRenderer::onDrawPath(const SkPath& origPath,
                                        const SkStrokeRec&,
                                        GrDrawTarget* target,
                                        bool antiAlias) {

    const SkPath* path = &origPath;
    if (path->isEmpty()) {
        return true;
    }

    SkMatrix viewMatrix = target->getDrawState().getViewMatrix();
    GrDrawTarget::AutoStateRestore asr;
    if (!asr.setIdentity(target, GrDrawTarget::kPreserve_ASRInit)) {
        return false;
    }
    GrDrawState* drawState = target->drawState();

    
    
    
    SkPath tmpPath;
    if (viewMatrix.hasPerspective()) {
        origPath.transform(viewMatrix, &tmpPath);
        path = &tmpPath;
        viewMatrix = SkMatrix::I();
    }

    QuadVertex *verts;
    uint16_t* idxs;

    int vCount;
    int iCount;
    enum {
        kPreallocSegmentCnt = 512 / sizeof(Segment),
        kPreallocDrawCnt = 4,
    };
    SkSTArray<kPreallocSegmentCnt, Segment, true> segments;
    SkPoint fanPt;

    
    
    SkRect devBounds;
    if (!get_segments(*path, viewMatrix, &segments, &fanPt, &vCount, &iCount, &devBounds)) {
        return false;
    }

    
    devBounds.outset(SK_Scalar1, SK_Scalar1);

    drawState->setVertexAttribs<gPathAttribs>(SK_ARRAY_COUNT(gPathAttribs));

    static const int kEdgeAttrIndex = 1;
    GrEffectRef* quadEffect = QuadEdgeEffect::Create();
    drawState->addCoverageEffect(quadEffect, kEdgeAttrIndex)->unref();

    GrDrawTarget::AutoReleaseGeometry arg(target, vCount, iCount);
    if (!arg.succeeded()) {
        return false;
    }
    SkASSERT(sizeof(QuadVertex) == drawState->getVertexSize());
    verts = reinterpret_cast<QuadVertex*>(arg.vertices());
    idxs = reinterpret_cast<uint16_t*>(arg.indices());

    SkSTArray<kPreallocDrawCnt, Draw, true> draws;
    create_vertices(segments, fanPt, &draws, verts, idxs);

    
#ifdef SK_DEBUG
    SkRect tolDevBounds = devBounds;
    tolDevBounds.outset(SK_Scalar1 / 10000, SK_Scalar1 / 10000);
    SkRect actualBounds;
    actualBounds.set(verts[0].fPos, verts[1].fPos);
    for (int i = 2; i < vCount; ++i) {
        actualBounds.growToInclude(verts[i].fPos.fX, verts[i].fPos.fY);
    }
    SkASSERT(tolDevBounds.contains(actualBounds));
#endif

    int vOffset = 0;
    for (int i = 0; i < draws.count(); ++i) {
        const Draw& draw = draws[i];
        target->drawIndexed(kTriangles_GrPrimitiveType,
                            vOffset,  
                            0,        
                            draw.fVertexCnt,
                            draw.fIndexCnt,
                            &devBounds);
        vOffset += draw.fVertexCnt;
    }

    return true;
}
