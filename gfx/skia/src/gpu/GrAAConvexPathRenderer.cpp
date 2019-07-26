







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

GrAAConvexPathRenderer::GrAAConvexPathRenderer() {
}

namespace {

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

void center_of_mass(const SegmentArray& segments, SkPoint* c) {
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
    GrAssert(!SkScalarIsNaN(c->fX) && !SkScalarIsNaN(c->fY));
}

void compute_vectors(SegmentArray* segments,
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
        const Segment& sega = (*segments)[a];
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

void update_degenerate_test(DegenerateTestData* data, const GrPoint& pt) {
    static const SkScalar TOL = (SK_Scalar1 / 16);
    static const SkScalar TOL_SQD = SkScalarMul(TOL, TOL);

    switch (data->fStage) {
        case DegenerateTestData::kInitial:
            data->fFirstPoint = pt;
            data->fStage = DegenerateTestData::kPoint;
            break;
        case DegenerateTestData::kPoint:
            if (pt.distanceToSqd(data->fFirstPoint) > TOL_SQD) {
                data->fLineNormal = pt - data->fFirstPoint;
                data->fLineNormal.normalize();
                data->fLineNormal.setOrthog(data->fLineNormal);
                data->fLineC = -data->fLineNormal.dot(data->fFirstPoint);
                data->fStage = DegenerateTestData::kLine;
            }
            break;
        case DegenerateTestData::kLine:
            if (SkScalarAbs(data->fLineNormal.dot(pt) + data->fLineC) > TOL) {
                data->fStage = DegenerateTestData::kNonDegenerate;
            }
        case DegenerateTestData::kNonDegenerate:
            break;
        default:
            GrCrash("Unexpected degenerate test stage.");
    }
}

inline bool get_direction(const SkPath& path, const SkMatrix& m, SkPath::Direction* dir) {
    if (!path.cheapComputeDirection(dir)) {
        return false;
    }
    
    GrAssert(!m.hasPerspective());
    SkScalar det2x2 = SkScalarMul(m.get(SkMatrix::kMScaleX), m.get(SkMatrix::kMScaleY)) -
                      SkScalarMul(m.get(SkMatrix::kMSkewX), m.get(SkMatrix::kMSkewY));
    if (det2x2 < 0) {
        *dir = SkPath::OppositeDirection(*dir);
    }
    return true;
}

bool get_segments(const SkPath& path,
                  const SkMatrix& m,
                  SegmentArray* segments,
                  SkPoint* fanPt,
                  int* vCount,
                  int* iCount) {
    SkPath::Iter iter(path, true);
    
    
    
    
    
    
    
    DegenerateTestData degenerateData;
    SkPath::Direction dir;
    
    if (!get_direction(path, m, &dir)) {
        return false;
    }

    for (;;) {
        GrPoint pts[4];
        GrPathCmd cmd = (GrPathCmd)iter.next(pts);
        switch (cmd) {
            case kMove_PathCmd:
                m.mapPoints(pts, 1);
                update_degenerate_test(&degenerateData, pts[0]);
                break;
            case kLine_PathCmd: {
                m.mapPoints(pts + 1, 1);
                update_degenerate_test(&degenerateData, pts[1]);
                segments->push_back();
                segments->back().fType = Segment::kLine;
                segments->back().fPts[0] = pts[1];
                break;
            }
            case kQuadratic_PathCmd:
                m.mapPoints(pts + 1, 2);
                update_degenerate_test(&degenerateData, pts[1]);
                update_degenerate_test(&degenerateData, pts[2]);
                segments->push_back();
                segments->back().fType = Segment::kQuad;
                segments->back().fPts[0] = pts[1];
                segments->back().fPts[1] = pts[2];
                break;
            case kCubic_PathCmd: {
                m.mapPoints(pts, 4);
                update_degenerate_test(&degenerateData, pts[1]);
                update_degenerate_test(&degenerateData, pts[2]);
                update_degenerate_test(&degenerateData, pts[3]);
                
                
                SkSTArray<15, SkPoint, true> quads;
                GrPathUtils::convertCubicToQuads(pts, SK_Scalar1, true, dir, &quads);
                int count = quads.count();
                for (int q = 0; q < count; q += 3) {
                    segments->push_back();
                    segments->back().fType = Segment::kQuad;
                    segments->back().fPts[0] = quads[q + 1];
                    segments->back().fPts[1] = quads[q + 2];
                }
                break;
            };
            case kEnd_PathCmd:
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

void create_vertices(const SegmentArray&  segments,
                     const SkPoint& fanPt,
                     QuadVertex*    verts,
                     uint16_t*      idxs) {
    int v = 0;
    int i = 0;

    int count = segments.count();
    for (int a = 0; a < count; ++a) {
        const Segment& sega = segments[a];
        int b = (a + 1) % count;
        const Segment& segb = segments[b];

        
        verts[v + 0].fPos = sega.endPt();
        verts[v + 1].fPos = verts[v + 0].fPos + sega.endNorm();
        verts[v + 2].fPos = verts[v + 0].fPos + segb.fMid;
        verts[v + 3].fPos = verts[v + 0].fPos + segb.fNorms[0];
        verts[v + 0].fUV.set(0,0);
        verts[v + 1].fUV.set(0,-SK_Scalar1);
        verts[v + 2].fUV.set(0,-SK_Scalar1);
        verts[v + 3].fUV.set(0,-SK_Scalar1);
        verts[v + 0].fD0 = verts[v + 0].fD1 = -SK_Scalar1;
        verts[v + 1].fD0 = verts[v + 1].fD1 = -SK_Scalar1;
        verts[v + 2].fD0 = verts[v + 2].fD1 = -SK_Scalar1;
        verts[v + 3].fD0 = verts[v + 3].fD1 = -SK_Scalar1;

        idxs[i + 0] = v + 0;
        idxs[i + 1] = v + 2;
        idxs[i + 2] = v + 1;
        idxs[i + 3] = v + 0;
        idxs[i + 4] = v + 3;
        idxs[i + 5] = v + 2;

        v += 4;
        i += 6;

        if (Segment::kLine == segb.fType) {
            verts[v + 0].fPos = fanPt;
            verts[v + 1].fPos = sega.endPt();
            verts[v + 2].fPos = segb.fPts[0];

            verts[v + 3].fPos = verts[v + 1].fPos + segb.fNorms[0];
            verts[v + 4].fPos = verts[v + 2].fPos + segb.fNorms[0];

            
            
            SkScalar dist = fanPt.distanceToLineBetween(verts[v + 1].fPos,
                                                        verts[v + 2].fPos);
            verts[v + 0].fUV.set(0, dist);
            verts[v + 1].fUV.set(0, 0);
            verts[v + 2].fUV.set(0, 0);
            verts[v + 3].fUV.set(0, -SK_Scalar1);
            verts[v + 4].fUV.set(0, -SK_Scalar1);

            verts[v + 0].fD0 = verts[v + 0].fD1 = -SK_Scalar1;
            verts[v + 1].fD0 = verts[v + 1].fD1 = -SK_Scalar1;
            verts[v + 2].fD0 = verts[v + 2].fD1 = -SK_Scalar1;
            verts[v + 3].fD0 = verts[v + 3].fD1 = -SK_Scalar1;
            verts[v + 4].fD0 = verts[v + 4].fD1 = -SK_Scalar1;

            idxs[i + 0] = v + 0;
            idxs[i + 1] = v + 2;
            idxs[i + 2] = v + 1;

            idxs[i + 3] = v + 3;
            idxs[i + 4] = v + 1;
            idxs[i + 5] = v + 2;

            idxs[i + 6] = v + 4;
            idxs[i + 7] = v + 3;
            idxs[i + 8] = v + 2;

            v += 5;
            i += 9;
        } else {
            GrPoint qpts[] = {sega.endPt(), segb.fPts[0], segb.fPts[1]};

            GrVec midVec = segb.fNorms[0] + segb.fNorms[1];
            midVec.normalize();

            verts[v + 0].fPos = fanPt;
            verts[v + 1].fPos = qpts[0];
            verts[v + 2].fPos = qpts[2];
            verts[v + 3].fPos = qpts[0] + segb.fNorms[0];
            verts[v + 4].fPos = qpts[2] + segb.fNorms[1];
            verts[v + 5].fPos = qpts[1] + midVec;

            SkScalar c = segb.fNorms[0].dot(qpts[0]);
            verts[v + 0].fD0 =  -segb.fNorms[0].dot(fanPt) + c;
            verts[v + 1].fD0 =  0.f;
            verts[v + 2].fD0 =  -segb.fNorms[0].dot(qpts[2]) + c;
            verts[v + 3].fD0 = -SK_ScalarMax/100;
            verts[v + 4].fD0 = -SK_ScalarMax/100;
            verts[v + 5].fD0 = -SK_ScalarMax/100;

            c = segb.fNorms[1].dot(qpts[2]);
            verts[v + 0].fD1 =  -segb.fNorms[1].dot(fanPt) + c;
            verts[v + 1].fD1 =  -segb.fNorms[1].dot(qpts[0]) + c;
            verts[v + 2].fD1 =  0.f;
            verts[v + 3].fD1 = -SK_ScalarMax/100;
            verts[v + 4].fD1 = -SK_ScalarMax/100;
            verts[v + 5].fD1 = -SK_ScalarMax/100;

            GrPathUtils::QuadUVMatrix toUV(qpts);
            toUV.apply<6, sizeof(QuadVertex), sizeof(GrPoint)>(verts + v);

            idxs[i + 0] = v + 3;
            idxs[i + 1] = v + 1;
            idxs[i + 2] = v + 2;
            idxs[i + 3] = v + 4;
            idxs[i + 4] = v + 3;
            idxs[i + 5] = v + 2;

            idxs[i + 6] = v + 5;
            idxs[i + 7] = v + 3;
            idxs[i + 8] = v + 4;

            idxs[i +  9] = v + 0;
            idxs[i + 10] = v + 2;
            idxs[i + 11] = v + 1;

            v += 6;
            i += 12;
        }
    }
}

}













class QuadEdgeEffect : public GrEffect {
public:

    static GrEffectRef* Create() {
        
        static GrEffectRef* gQuadEdgeEffectRef = 
            CreateEffectRef(AutoEffectUnref(SkNEW(QuadEdgeEffect)));
        static SkAutoTUnref<GrEffectRef> gUnref(gQuadEdgeEffectRef);

        gQuadEdgeEffectRef->ref();
        return gQuadEdgeEffectRef;
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
    QuadEdgeEffect() { 
        this->addVertexAttrib(kVec4f_GrSLType); 
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        return true;
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(QuadEdgeEffect);

GrEffectRef* QuadEdgeEffect::TestCreate(SkMWCRandom* random,
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

bool GrAAConvexPathRenderer::onDrawPath(const SkPath& origPath,
                                        const SkStrokeRec&,
                                        GrDrawTarget* target,
                                        bool antiAlias) {

    const SkPath* path = &origPath;
    if (path->isEmpty()) {
        return true;
    }

    GrDrawTarget::AutoStateRestore asr(target, GrDrawTarget::kPreserve_ASRInit);
    GrDrawState* drawState = target->drawState();

    GrDrawState::AutoDeviceCoordDraw adcd(drawState);
    if (!adcd.succeeded()) {
        return false;
    }
    const SkMatrix* vm = &adcd.getOriginalMatrix();

    
    
    
    SkPath tmpPath;
    if (vm->hasPerspective()) {
        origPath.transform(*vm, &tmpPath);
        path = &tmpPath;
        vm = &SkMatrix::I();
    }

    QuadVertex *verts;
    uint16_t* idxs;

    int vCount;
    int iCount;
    enum {
        kPreallocSegmentCnt = 512 / sizeof(Segment),
    };
    SkSTArray<kPreallocSegmentCnt, Segment, true> segments;
    SkPoint fanPt;

    if (!get_segments(*path, *vm, &segments, &fanPt, &vCount, &iCount)) {
        return false;
    }

    
    static const GrVertexAttrib kAttribs[] = {
        {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
        {kVec4f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding}
    };
    drawState->setVertexAttribs(kAttribs, SK_ARRAY_COUNT(kAttribs));

    enum {
        
        
        
        kEdgeEffectStage = GrPaint::kTotalStages,
    };
    static const int kEdgeAttrIndex = 1;
    GrEffectRef* quadEffect = QuadEdgeEffect::Create();
    drawState->setEffect(kEdgeEffectStage, quadEffect, kEdgeAttrIndex)->unref();

    GrDrawTarget::AutoReleaseGeometry arg(target, vCount, iCount);
    if (!arg.succeeded()) {
        return false;
    }
    GrAssert(sizeof(QuadVertex) == drawState->getVertexSize());
    verts = reinterpret_cast<QuadVertex*>(arg.vertices());
    idxs = reinterpret_cast<uint16_t*>(arg.indices());

    create_vertices(segments, fanPt, verts, idxs);

    target->drawIndexed(kTriangles_GrPrimitiveType,
                        0,        
                        0,        
                        vCount,
                        iCount);

    return true;
}
