






#include "GrOvalRenderer.h"

#include "GrEffect.h"
#include "gl/GrGLEffect.h"
#include "gl/GrGLSL.h"
#include "GrTBackendEffectFactory.h"

#include "GrDrawState.h"
#include "GrDrawTarget.h"
#include "SkStrokeRec.h"

SK_DEFINE_INST_COUNT(GrOvalRenderer)

namespace {

struct CircleVertex {
    GrPoint fPos;
    GrPoint fCenter;
    SkScalar fOuterRadius;
    SkScalar fInnerRadius;
};

struct EllipseVertex {
    GrPoint fPos;
    GrPoint fCenter;
    SkScalar fOuterXRadius;
    SkScalar fOuterXYRatio;
    SkScalar fInnerXRadius;
    SkScalar fInnerXYRatio;
};

inline bool circle_stays_circle(const SkMatrix& m) {
    return m.isSimilarity();
}

}









class CircleEdgeEffect : public GrEffect {
public:
    static GrEffectRef* Create(bool stroke) {
        
        static SkAutoTUnref<GrEffectRef> gCircleStrokeEdgeEffectRef(
                        CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(CircleEdgeEffect, (true)))));
        static SkAutoTUnref<GrEffectRef> gCircleFillEdgeEffectRef(
                        CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(CircleEdgeEffect, (false)))));

        if (stroke) {
            gCircleStrokeEdgeEffectRef.get()->ref();
            return gCircleStrokeEdgeEffectRef;
        } else {
            gCircleFillEdgeEffectRef.get()->ref();
            return gCircleFillEdgeEffectRef;
        }
    }

    virtual void getConstantColorComponents(GrColor* color, 
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<CircleEdgeEffect>::getInstance();
    }

    virtual ~CircleEdgeEffect() {}

    static const char* Name() { return "CircleEdge"; }

    inline bool isStroked() const { return fStroke; }

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
            const CircleEdgeEffect& circleEffect = drawEffect.castEffect<CircleEdgeEffect>();
            const char *vsName, *fsName;
            builder->addVarying(kVec4f_GrSLType, "CircleEdge", &vsName, &fsName);

            const SkString* attrName =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[0]);
            builder->vsCodeAppendf("\t%s = %s;\n", vsName, attrName->c_str());

            builder->fsCodeAppendf("\tfloat d = distance(%s.xy, %s.xy);\n",
                                   builder->fragmentPosition(), fsName);
            builder->fsCodeAppendf("\tfloat edgeAlpha = clamp(%s.z - d, 0.0, 1.0);\n", fsName);
            if (circleEffect.isStroked()) {
                builder->fsCodeAppendf("\tfloat innerAlpha = clamp(d - %s.w, 0.0, 1.0);\n", fsName);
                builder->fsCodeAppend("\tedgeAlpha *= innerAlpha;\n");
            }
            SkString modulate;
            GrGLSLModulate4f(&modulate, inputColor, "edgeAlpha");
            builder->fsCodeAppendf("\t%s = %s;\n", outputColor, modulate.c_str());
        }

        static inline EffectKey GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
            const CircleEdgeEffect& circleEffect = drawEffect.castEffect<CircleEdgeEffect>();

            return circleEffect.isStroked() ? 0x1 : 0x0;
        }

        virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE {}

    private:
        typedef GrGLEffect INHERITED;
    };


private:
    CircleEdgeEffect(bool stroke) : GrEffect() {
        this->addVertexAttrib(kVec4f_GrSLType);
        fStroke = stroke;
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        const CircleEdgeEffect& cee = CastEffect<CircleEdgeEffect>(other);
        return cee.fStroke == fStroke;
    }

    bool fStroke;

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(CircleEdgeEffect);

GrEffectRef* CircleEdgeEffect::TestCreate(SkMWCRandom* random,
                                          GrContext* context,
                                          const GrDrawTargetCaps&,
                                          GrTexture* textures[]) {
    return CircleEdgeEffect::Create(random->nextBool());
}








class EllipseEdgeEffect : public GrEffect {
public:
    static GrEffectRef* Create(bool stroke) {
        
        static SkAutoTUnref<GrEffectRef> gEllipseStrokeEdgeEffectRef(
                        CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(EllipseEdgeEffect, (true)))));
        static SkAutoTUnref<GrEffectRef> gEllipseFillEdgeEffectRef(
                        CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(EllipseEdgeEffect, (false)))));

        if (stroke) {
            gEllipseStrokeEdgeEffectRef.get()->ref();
            return gEllipseStrokeEdgeEffectRef;
        } else {
            gEllipseFillEdgeEffectRef.get()->ref();
            return gEllipseFillEdgeEffectRef;
        }
    }

    virtual void getConstantColorComponents(GrColor* color, 
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<EllipseEdgeEffect>::getInstance();
    }

    virtual ~EllipseEdgeEffect() {}

    static const char* Name() { return "EllipseEdge"; }

    inline bool isStroked() const { return fStroke; }

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
            const EllipseEdgeEffect& ellipseEffect = drawEffect.castEffect<EllipseEdgeEffect>();

            const char *vsCenterName, *fsCenterName;
            const char *vsEdgeName, *fsEdgeName;

            builder->addVarying(kVec2f_GrSLType, "EllipseCenter", &vsCenterName, &fsCenterName);
            const SkString* attr0Name =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[0]);
            builder->vsCodeAppendf("\t%s = %s;\n", vsCenterName, attr0Name->c_str());

            builder->addVarying(kVec4f_GrSLType, "EllipseEdge", &vsEdgeName, &fsEdgeName);
            const SkString* attr1Name =
                builder->getEffectAttributeName(drawEffect.getVertexAttribIndices()[1]);
            builder->vsCodeAppendf("\t%s = %s;\n", vsEdgeName, attr1Name->c_str());

            
            builder->fsCodeAppendf("\tvec2 outerOffset = (%s.xy - %s.xy);\n",
                                   builder->fragmentPosition(), fsCenterName);
            builder->fsCodeAppend("\tvec2 innerOffset = outerOffset;\n");
            
            builder->fsCodeAppendf("\touterOffset.y *= %s.y;\n", fsEdgeName);
            builder->fsCodeAppend("\tfloat dOuter = length(outerOffset);\n");
            
            builder->fsCodeAppendf("\tfloat edgeAlpha = clamp(%s.x-dOuter, 0.0, 1.0);\n", 
                                   fsEdgeName);

            if (ellipseEffect.isStroked()) {
                builder->fsCodeAppendf("\tinnerOffset.y *= %s.w;\n", fsEdgeName);
                builder->fsCodeAppend("\tfloat dInner = length(innerOffset);\n");

                
                builder->fsCodeAppendf("\tfloat innerAlpha = clamp(dInner-%s.z, 0.0, 1.0);\n", 
                                       fsEdgeName);
                builder->fsCodeAppend("\tedgeAlpha *= innerAlpha;\n");
            }

            SkString modulate;
            GrGLSLModulate4f(&modulate, inputColor, "edgeAlpha");
            builder->fsCodeAppendf("\t%s = %s;\n", outputColor, modulate.c_str());      
        }

        static inline EffectKey GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
            const EllipseEdgeEffect& ellipseEffect = drawEffect.castEffect<EllipseEdgeEffect>();

            return ellipseEffect.isStroked() ? 0x1 : 0x0;
        }

        virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE {
        }

    private:
        typedef GrGLEffect INHERITED;
    };

private:
    EllipseEdgeEffect(bool stroke) : GrEffect() {
        this->addVertexAttrib(kVec2f_GrSLType);
        this->addVertexAttrib(kVec4f_GrSLType);
        fStroke = stroke;
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        const EllipseEdgeEffect& eee = CastEffect<EllipseEdgeEffect>(other);
        return eee.fStroke == fStroke;
    }

    bool fStroke;

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(EllipseEdgeEffect);

GrEffectRef* EllipseEdgeEffect::TestCreate(SkMWCRandom* random,
                                           GrContext* context,
                                           const GrDrawTargetCaps&,
                                           GrTexture* textures[]) {
    return EllipseEdgeEffect::Create(random->nextBool());
}



bool GrOvalRenderer::drawOval(GrDrawTarget* target, const GrContext* context, const GrPaint& paint,
                    const GrRect& oval, const SkStrokeRec& stroke)
{
    if (!paint.isAntiAlias()) {
        return false;
    }

    const SkMatrix& vm = context->getMatrix();

    
    if (SkScalarNearlyEqual(oval.width(), oval.height())
        && circle_stays_circle(vm)) {
        drawCircle(target, paint, oval, stroke);

    
    } else if (vm.rectStaysRect()) {
        drawEllipse(target, paint, oval, stroke);

    } else {
        return false;
    }

    return true;
}

void GrOvalRenderer::drawCircle(GrDrawTarget* target,
                                const GrPaint& paint,
                                const GrRect& circle,
                                const SkStrokeRec& stroke)
{
    GrDrawState* drawState = target->drawState();

    const SkMatrix& vm = drawState->getViewMatrix();
    GrPoint center = GrPoint::Make(circle.centerX(), circle.centerY());
    vm.mapPoints(&center, 1);
    SkScalar radius = vm.mapRadius(SkScalarHalf(circle.width()));
    SkScalar strokeWidth = vm.mapRadius(stroke.getWidth());

    GrDrawState::AutoDeviceCoordDraw adcd(drawState);
    if (!adcd.succeeded()) {
        return;
    }

    
    static const GrVertexAttrib kVertexAttribs[] = {
        {kVec2f_GrVertexAttribType, 0, kPosition_GrVertexAttribBinding},
        {kVec4f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding}
    };
    drawState->setVertexAttribs(kVertexAttribs, SK_ARRAY_COUNT(kVertexAttribs));
    GrAssert(sizeof(CircleVertex) == drawState->getVertexSize());

    GrDrawTarget::AutoReleaseGeometry geo(target, 4, 0);
    if (!geo.succeeded()) {
        GrPrintf("Failed to get space for vertices!\n");
        return;
    }

    CircleVertex* verts = reinterpret_cast<CircleVertex*>(geo.vertices());

    SkStrokeRec::Style style = stroke.getStyle();
    bool isStroked = (SkStrokeRec::kStroke_Style == style || SkStrokeRec::kHairline_Style == style);
    enum {
        
        
        
        kEdgeEffectStage = GrPaint::kTotalStages,
    };

    GrEffectRef* effect = CircleEdgeEffect::Create(isStroked);
    static const int kCircleEdgeAttrIndex = 1;
    drawState->setEffect(kEdgeEffectStage, effect, kCircleEdgeAttrIndex)->unref();

    SkScalar innerRadius = 0.0f;
    SkScalar outerRadius = radius;
    SkScalar halfWidth = 0;
    if (style != SkStrokeRec::kFill_Style) {
        if (SkScalarNearlyZero(strokeWidth)) {
            halfWidth = SK_ScalarHalf;
        } else {
            halfWidth = SkScalarHalf(strokeWidth);
        }

        outerRadius += halfWidth;
        if (isStroked) {
            innerRadius = SkMaxScalar(0, radius - halfWidth);
        }
    }

    
    
    
    
    outerRadius += SK_ScalarHalf;
    innerRadius -= SK_ScalarHalf;

    for (int i = 0; i < 4; ++i) {
        verts[i].fCenter = center;
        verts[i].fOuterRadius = outerRadius;
        verts[i].fInnerRadius = innerRadius;
    }

    SkRect bounds = SkRect::MakeLTRB(
        center.fX - outerRadius,
        center.fY - outerRadius,
        center.fX + outerRadius,
        center.fY + outerRadius
    );

    verts[0].fPos = SkPoint::Make(bounds.fLeft,  bounds.fTop);
    verts[1].fPos = SkPoint::Make(bounds.fRight, bounds.fTop);
    verts[2].fPos = SkPoint::Make(bounds.fLeft,  bounds.fBottom);
    verts[3].fPos = SkPoint::Make(bounds.fRight, bounds.fBottom);

    target->drawNonIndexed(kTriangleStrip_GrPrimitiveType, 0, 4, &bounds);
}

void GrOvalRenderer::drawEllipse(GrDrawTarget* target,
                                 const GrPaint& paint,
                                 const GrRect& ellipse,
                                 const SkStrokeRec& stroke)
{
    GrDrawState* drawState = target->drawState();
#ifdef SK_DEBUG
    {
        
        bool isAxisAlignedEllipse = drawState->getViewMatrix().rectStaysRect();
        SkASSERT(paint.isAntiAlias() && isAxisAlignedEllipse);
    }
#endif

    const SkMatrix& vm = drawState->getViewMatrix();
    GrPoint center = GrPoint::Make(ellipse.centerX(), ellipse.centerY());
    vm.mapPoints(&center, 1);
    SkRect xformedRect;
    vm.mapRect(&xformedRect, ellipse);

    GrDrawState::AutoDeviceCoordDraw adcd(drawState);
    if (!adcd.succeeded()) {
        return;
    }

    
    static const GrVertexAttrib kVertexAttribs[] = {
        {kVec2f_GrVertexAttribType, 0, kPosition_GrVertexAttribBinding},
        {kVec2f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding},
        {kVec4f_GrVertexAttribType, 2*sizeof(GrPoint), kEffect_GrVertexAttribBinding}
    };
    drawState->setVertexAttribs(kVertexAttribs, SK_ARRAY_COUNT(kVertexAttribs));
    GrAssert(sizeof(EllipseVertex) == drawState->getVertexSize());

    GrDrawTarget::AutoReleaseGeometry geo(target, 4, 0);
    if (!geo.succeeded()) {
        GrPrintf("Failed to get space for vertices!\n");
        return;
    }

    EllipseVertex* verts = reinterpret_cast<EllipseVertex*>(geo.vertices());

    SkStrokeRec::Style style = stroke.getStyle();
    bool isStroked = (SkStrokeRec::kStroke_Style == style || SkStrokeRec::kHairline_Style == style);
    enum {
        
        
        
        kEdgeEffectStage = GrPaint::kTotalStages,
    };

    GrEffectRef* effect = EllipseEdgeEffect::Create(isStroked);
    static const int kEllipseCenterAttrIndex = 1;
    static const int kEllipseEdgeAttrIndex = 2;
    drawState->setEffect(kEdgeEffectStage, effect,
                         kEllipseCenterAttrIndex, kEllipseEdgeAttrIndex)->unref();

    SkScalar xRadius = SkScalarHalf(xformedRect.width());
    SkScalar yRadius = SkScalarHalf(xformedRect.height());
    SkScalar innerXRadius = 0.0f;
    SkScalar innerRatio = 1.0f;

    if (SkStrokeRec::kFill_Style != style) {
        SkScalar strokeWidth = stroke.getWidth();

        
        SkVector scaledStroke;
        scaledStroke.set(strokeWidth, strokeWidth);
        vm.mapVectors(&scaledStroke, 1);

        if (SkScalarNearlyZero(scaledStroke.length())) {
            scaledStroke.set(SK_ScalarHalf, SK_ScalarHalf);
        } else {
            scaledStroke.scale(0.5f);
        }

        
        if (SkStrokeRec::kStroke_Style == style || SkStrokeRec::kHairline_Style == style) {
            SkScalar innerYRadius = SkMaxScalar(0, yRadius - scaledStroke.fY);
            if (innerYRadius > SK_ScalarNearlyZero) {
                innerXRadius = SkMaxScalar(0, xRadius - scaledStroke.fX);
                innerRatio = innerXRadius/innerYRadius;
            }
        }
        xRadius += scaledStroke.fX;
        yRadius += scaledStroke.fY;
    }

    SkScalar outerRatio = SkScalarDiv(xRadius, yRadius);

    for (int i = 0; i < 4; ++i) {
        verts[i].fCenter = center;
        verts[i].fOuterXRadius = xRadius + 0.5f;
        verts[i].fOuterXYRatio = outerRatio;
        verts[i].fInnerXRadius = innerXRadius - 0.5f;
        verts[i].fInnerXYRatio = innerRatio;
    }

    SkScalar L = -xRadius;
    SkScalar R = +xRadius;
    SkScalar T = -yRadius;
    SkScalar B = +yRadius;

    
    
    L += center.fX - SK_ScalarHalf;
    R += center.fX + SK_ScalarHalf;
    T += center.fY - SK_ScalarHalf;
    B += center.fY + SK_ScalarHalf;

    verts[0].fPos = SkPoint::Make(L, T);
    verts[1].fPos = SkPoint::Make(R, T);
    verts[2].fPos = SkPoint::Make(L, B);
    verts[3].fPos = SkPoint::Make(R, B);

    target->drawNonIndexed(kTriangleStrip_GrPrimitiveType, 0, 4);
}
