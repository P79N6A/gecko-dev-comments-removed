




#include "PathD2D.h"
#include "HelpersD2D.h"
#include <math.h>
#include "DrawTargetD2D.h"
#include "Logging.h"
#include "mozilla/Constants.h"

namespace mozilla {
namespace gfx {





class OpeningGeometrySink : public ID2D1SimplifiedGeometrySink
{
public:
  OpeningGeometrySink(ID2D1SimplifiedGeometrySink *aSink)
    : mSink(aSink)
    , mNeedsFigureEnded(false)
  {
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &aIID, void **aPtr)
  {
    if (!aPtr) {
      return E_POINTER;
    }

    if (aIID == IID_IUnknown) {
      *aPtr = static_cast<IUnknown*>(this);
      return S_OK;
    } else if (aIID == IID_ID2D1SimplifiedGeometrySink) {
      *aPtr = static_cast<ID2D1SimplifiedGeometrySink*>(this);
      return S_OK;
    }

    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef()
  {
    return 1;
  }

  ULONG STDMETHODCALLTYPE Release()
  {
    return 1;
  }

  
  STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE aMode)
  { EnsureFigureEnded(); return; }
  STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F aPoint, D2D1_FIGURE_BEGIN aBegin)
  { EnsureFigureEnded(); return mSink->BeginFigure(aPoint, aBegin); }
  STDMETHOD_(void, AddLines)(const D2D1_POINT_2F *aLines, UINT aCount)
  { EnsureFigureEnded(); return mSink->AddLines(aLines, aCount); }
  STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT *aSegments, UINT aCount)
  { EnsureFigureEnded(); return mSink->AddBeziers(aSegments, aCount); }
  STDMETHOD(Close)()
  {  return S_OK; }
  STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT aFlags)
  { return mSink->SetSegmentFlags(aFlags); }

  
  
  
  STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END aEnd)
  {
    if (aEnd == D2D1_FIGURE_END_CLOSED) {
      return mSink->EndFigure(aEnd);
    } else {
      mNeedsFigureEnded = true;
    }
  }
private:
  void EnsureFigureEnded()
  {
    if (mNeedsFigureEnded) {
      mSink->EndFigure(D2D1_FIGURE_END_OPEN);
      mNeedsFigureEnded = false;
    }
  }

  ID2D1SimplifiedGeometrySink *mSink;
  bool mNeedsFigureEnded;
};

class StreamingGeometrySink : public ID2D1SimplifiedGeometrySink
{
public:
  StreamingGeometrySink(PathSink *aSink)
    : mSink(aSink)
  {
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &aIID, void **aPtr)
  {
    if (!aPtr) {
      return E_POINTER;
    }

    if (aIID == IID_IUnknown) {
      *aPtr = static_cast<IUnknown*>(this);
      return S_OK;
    } else if (aIID == IID_ID2D1SimplifiedGeometrySink) {
      *aPtr = static_cast<ID2D1SimplifiedGeometrySink*>(this);
      return S_OK;
    }

    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef()
  {
    return 1;
  }

  ULONG STDMETHODCALLTYPE Release()
  {
    return 1;
  }

  
  STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE aMode)
  { return; }
  STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F aPoint, D2D1_FIGURE_BEGIN aBegin)
  { mSink->MoveTo(ToPoint(aPoint)); }
  STDMETHOD_(void, AddLines)(const D2D1_POINT_2F *aLines, UINT aCount)
  { for (UINT i = 0; i < aCount; i++) { mSink->LineTo(ToPoint(aLines[i])); } }
  STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT *aSegments, UINT aCount)
  {
    for (UINT i = 0; i < aCount; i++) {
      mSink->BezierTo(ToPoint(aSegments[i].point1), ToPoint(aSegments[i].point2), ToPoint(aSegments[i].point3));
    }
  }
  STDMETHOD(Close)()
  {  return S_OK; }
  STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT aFlags)
  {  }

  STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END aEnd)
  {
    if (aEnd == D2D1_FIGURE_END_CLOSED) {
      return mSink->Close();
    }
  }
private:

  PathSink *mSink;
};

PathBuilderD2D::~PathBuilderD2D()
{
}

void
PathBuilderD2D::MoveTo(const Point &aPoint)
{
  if (mFigureActive) {
    mSink->EndFigure(D2D1_FIGURE_END_OPEN);
    mFigureActive = false;
  }
  EnsureActive(aPoint);
  mCurrentPoint = aPoint;
}

void
PathBuilderD2D::LineTo(const Point &aPoint)
{
  EnsureActive(aPoint);
  mSink->AddLine(D2DPoint(aPoint));

  mCurrentPoint = aPoint;
}

void
PathBuilderD2D::BezierTo(const Point &aCP1,
                         const Point &aCP2,
                         const Point &aCP3)
  {
  EnsureActive(aCP1);
  mSink->AddBezier(D2D1::BezierSegment(D2DPoint(aCP1),
                                       D2DPoint(aCP2),
                                       D2DPoint(aCP3)));

  mCurrentPoint = aCP3;
}

void
PathBuilderD2D::QuadraticBezierTo(const Point &aCP1,
                                  const Point &aCP2)
{
  EnsureActive(aCP1);
  mSink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(D2DPoint(aCP1),
                                                         D2DPoint(aCP2)));

  mCurrentPoint = aCP2;
}

void
PathBuilderD2D::Close()
{
  if (mFigureActive) {
    mSink->EndFigure(D2D1_FIGURE_END_CLOSED);

    mFigureActive = false;

    EnsureActive(mBeginPoint);
  }
}

void
PathBuilderD2D::Arc(const Point &aOrigin, Float aRadius, Float aStartAngle,
                 Float aEndAngle, bool aAntiClockwise)
{
  if (aAntiClockwise && aStartAngle < aEndAngle) {
    
    
    
    
    
    Float oldStart = aStartAngle;
    aStartAngle = aEndAngle;
    aEndAngle = oldStart;
  }

  const Float kSmallRadius = 0.007f;
  Float midAngle = 0;
  bool smallFullCircle = false;

  
  
  if (aEndAngle - aStartAngle >= 2 * M_PI) {
    if (aRadius > kSmallRadius) {
      aEndAngle = Float(aStartAngle + M_PI * 1.9999);
    }
    else {
      smallFullCircle = true;
      midAngle = Float(aStartAngle + M_PI);
      aEndAngle = Float(aStartAngle + 2 * M_PI);
    }
  } else if (aStartAngle - aEndAngle >= 2 * M_PI) {
    if (aRadius > kSmallRadius) {
      aStartAngle = Float(aEndAngle + M_PI * 1.9999);
    }
    else {
      smallFullCircle = true;
      midAngle = Float(aEndAngle + M_PI);
      aStartAngle = Float(aEndAngle + 2 * M_PI);
    }
  }

  Point startPoint;
  startPoint.x = aOrigin.x + aRadius * cos(aStartAngle);
  startPoint.y = aOrigin.y + aRadius * sin(aStartAngle);

  if (!mFigureActive) {
    EnsureActive(startPoint);
  } else {
    mSink->AddLine(D2DPoint(startPoint));
  }

  Point endPoint;
  endPoint.x = aOrigin.x + aRadius * cos(aEndAngle);
  endPoint.y = aOrigin.y + aRadius * sin(aEndAngle);

  D2D1_ARC_SIZE arcSize = D2D1_ARC_SIZE_SMALL;
  D2D1_SWEEP_DIRECTION direction =
    aAntiClockwise ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE :
                     D2D1_SWEEP_DIRECTION_CLOCKWISE;

  if (!smallFullCircle) {

    if (aAntiClockwise) {
      if (aStartAngle - aEndAngle > M_PI) {
        arcSize = D2D1_ARC_SIZE_LARGE;
      }
    } else {
      if (aEndAngle - aStartAngle > M_PI) {
        arcSize = D2D1_ARC_SIZE_LARGE;
      }
    }

    mSink->AddArc(D2D1::ArcSegment(D2DPoint(endPoint),
                                   D2D1::SizeF(aRadius, aRadius),
                                   0.0f,
                                   direction,
                                   arcSize));
  }
  else {
    
    Point midPoint;
    midPoint.x = aOrigin.x + aRadius * cos(midAngle);
    midPoint.y = aOrigin.y + aRadius * sin(midAngle);

    mSink->AddArc(D2D1::ArcSegment(D2DPoint(midPoint),
                                   D2D1::SizeF(aRadius, aRadius),
                                   0.0f,
                                   direction,
                                   arcSize));

    mSink->AddArc(D2D1::ArcSegment(D2DPoint(endPoint),
                                   D2D1::SizeF(aRadius, aRadius),
                                   0.0f,
                                   direction,
                                   arcSize));
  }

  mCurrentPoint = endPoint;
}

Point
PathBuilderD2D::CurrentPoint() const
{
  return mCurrentPoint;
}

void
PathBuilderD2D::EnsureActive(const Point &aPoint)
{
  if (!mFigureActive) {
    mSink->BeginFigure(D2DPoint(aPoint), D2D1_FIGURE_BEGIN_FILLED);
    mBeginPoint = aPoint;
    mFigureActive = true;
  }
}

already_AddRefed<Path>
PathBuilderD2D::Finish()
{
  if (mFigureActive) {
    mSink->EndFigure(D2D1_FIGURE_END_OPEN);
  }

  HRESULT hr = mSink->Close();
  if (FAILED(hr)) {
    gfxDebug() << "Failed to close PathSink. Code: " << hexa(hr);
    return nullptr;
  }

  return MakeAndAddRef<PathD2D>(mGeometry, mFigureActive, mCurrentPoint, mFillRule, mBackendType);
}

already_AddRefed<PathBuilder>
PathD2D::CopyToBuilder(FillRule aFillRule) const
{
  return TransformedCopyToBuilder(Matrix(), aFillRule);
}

already_AddRefed<PathBuilder>
PathD2D::TransformedCopyToBuilder(const Matrix &aTransform, FillRule aFillRule) const
{
  RefPtr<ID2D1PathGeometry> path;
  HRESULT hr = DrawTargetD2D::factory()->CreatePathGeometry(byRef(path));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create PathGeometry. Code: " << hexa(hr);
    return nullptr;
  }

  RefPtr<ID2D1GeometrySink> sink;
  hr = path->Open(byRef(sink));
  if (FAILED(hr)) {
    gfxWarning() << "Failed to open Geometry for writing. Code: " << hexa(hr);
    return nullptr;
  }

  if (aFillRule == FillRule::FILL_WINDING) {
    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
  }

  if (mEndedActive) {
    OpeningGeometrySink wrapSink(sink);
    mGeometry->Simplify(D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
                        D2DMatrix(aTransform),
                        &wrapSink);
  } else {
    mGeometry->Simplify(D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
                        D2DMatrix(aTransform),
                        sink);
  }

  RefPtr<PathBuilderD2D> pathBuilder = new PathBuilderD2D(sink, path, aFillRule, mBackendType);
  
  pathBuilder->mCurrentPoint = aTransform * mEndPoint;
  
  if (mEndedActive) {
    pathBuilder->mFigureActive = true;
  }

  return pathBuilder.forget();
}

void
PathD2D::StreamToSink(PathSink *aSink) const
{
  HRESULT hr;

  StreamingGeometrySink sink(aSink);

  hr = mGeometry->Simplify(D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
                           D2D1::IdentityMatrix(), &sink);

  if (FAILED(hr)) {
    gfxWarning() << "Failed to stream D2D path to sink. Code: " << hexa(hr);
    return;
  }
}
 
bool
PathD2D::ContainsPoint(const Point &aPoint, const Matrix &aTransform) const
{
  BOOL result;

  HRESULT hr = mGeometry->FillContainsPoint(D2DPoint(aPoint), D2DMatrix(aTransform), 0.001f, &result);

  if (FAILED(hr)) {
    
    return false;
  }

  return !!result;
}

bool
PathD2D::StrokeContainsPoint(const StrokeOptions &aStrokeOptions,
                             const Point &aPoint,
                             const Matrix &aTransform) const
{
  BOOL result;

  RefPtr<ID2D1StrokeStyle> strokeStyle = CreateStrokeStyleForOptions(aStrokeOptions);
  HRESULT hr = mGeometry->StrokeContainsPoint(D2DPoint(aPoint),
                                              aStrokeOptions.mLineWidth,
                                              strokeStyle,
                                              D2DMatrix(aTransform),
                                              &result);

  if (FAILED(hr)) {
    
    return false;
  }

  return !!result;
}

Rect
PathD2D::GetBounds(const Matrix &aTransform) const
{
  D2D1_RECT_F d2dBounds;

  HRESULT hr = mGeometry->GetBounds(D2DMatrix(aTransform), &d2dBounds);

  Rect bounds = ToRect(d2dBounds);
  if (FAILED(hr) || !bounds.IsFinite()) {
    gfxWarning() << "Failed to get stroked bounds for path. Code: " << hexa(hr);
    return Rect();
  }

  return bounds;
}

Rect
PathD2D::GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                          const Matrix &aTransform) const
{
  D2D1_RECT_F d2dBounds;

  RefPtr<ID2D1StrokeStyle> strokeStyle = CreateStrokeStyleForOptions(aStrokeOptions);
  HRESULT hr =
    mGeometry->GetWidenedBounds(aStrokeOptions.mLineWidth, strokeStyle,
                                D2DMatrix(aTransform), &d2dBounds);

  Rect bounds = ToRect(d2dBounds);
  if (FAILED(hr) || !bounds.IsFinite()) {
    gfxWarning() << "Failed to get stroked bounds for path. Code: " << hexa(hr);
    return Rect();
  }

  return bounds;
}

}
}
