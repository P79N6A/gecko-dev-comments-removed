








#ifndef SkEdgeClipper_DEFINED
#define SkEdgeClipper_DEFINED

#include "SkPath.h"




class SkEdgeClipper {
public:
    bool clipQuad(const SkPoint pts[3], const SkRect& clip);
    bool clipCubic(const SkPoint pts[4], const SkRect& clip);

    SkPath::Verb next(SkPoint pts[]);

private:
    SkPoint*        fCurrPoint;
    SkPath::Verb*   fCurrVerb;

    enum {
        kMaxVerbs = 13,
        kMaxPoints = 32
    };
    SkPoint         fPoints[kMaxPoints];
    SkPath::Verb    fVerbs[kMaxVerbs];

    void clipMonoQuad(const SkPoint srcPts[3], const SkRect& clip);
    void clipMonoCubic(const SkPoint srcPts[4], const SkRect& clip);
    void appendVLine(SkScalar x, SkScalar y0, SkScalar y1, bool reverse);
    void appendQuad(const SkPoint pts[3], bool reverse);
    void appendCubic(const SkPoint pts[4], bool reverse);
};

#ifdef SK_DEBUG
    void sk_assert_monotonic_x(const SkPoint pts[], int count);
    void sk_assert_monotonic_y(const SkPoint pts[], int count);
#else
    #define sk_assert_monotonic_x(pts, count)
    #define sk_assert_monotonic_y(pts, count)
#endif

#endif
