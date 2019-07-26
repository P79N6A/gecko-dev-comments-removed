





#include "nsSVGFilterInstance.h"


#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"

float
nsSVGFilterInstance::GetPrimitiveNumber(uint8_t aCtxType, float aValue) const
{
  nsSVGLength2 val;
  val.Init(aCtxType, 0xff, aValue,
           nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);

  float value;
  if (mPrimitiveUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    value = nsSVGUtils::ObjectSpace(mTargetBBox, &val);
  } else {
    value = nsSVGUtils::UserSpace(mTargetFrame, &val);
  }

  switch (aCtxType) {
  case SVGContentUtils::X:
    return value * mFilterSpaceSize.width / mFilterRegion.Width();
  case SVGContentUtils::Y:
    return value * mFilterSpaceSize.height / mFilterRegion.Height();
  case SVGContentUtils::XY:
  default:
    return value * SVGContentUtils::ComputeNormalizedHypotenuse(
                     mFilterSpaceSize.width / mFilterRegion.Width(),
                     mFilterSpaceSize.height / mFilterRegion.Height());
  }
}

void
nsSVGFilterInstance::ConvertLocation(float aValues[3]) const
{
  nsSVGLength2 val[4];
  val[0].Init(SVGContentUtils::X, 0xff, aValues[0],
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  val[1].Init(SVGContentUtils::Y, 0xff, aValues[1],
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  
  val[2].Init(SVGContentUtils::X, 0xff, 0,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  val[3].Init(SVGContentUtils::Y, 0xff, 0,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);

  gfxRect feArea = nsSVGUtils::GetRelativeRect(mPrimitiveUnits,
    val, mTargetBBox, mTargetFrame);
  aValues[0] = feArea.X();
  aValues[1] = feArea.Y();
  aValues[2] = GetPrimitiveNumber(SVGContentUtils::XY, aValues[2]);
}

already_AddRefed<gfxImageSurface>
nsSVGFilterInstance::CreateImage()
{
  nsRefPtr<gfxImageSurface> surface =
    new gfxImageSurface(gfxIntSize(mSurfaceRect.width, mSurfaceRect.height),
                        gfxASurface::ImageFormatARGB32);

  if (!surface || surface->CairoStatus())
    return nullptr;

  surface->SetDeviceOffset(gfxPoint(-mSurfaceRect.x, -mSurfaceRect.y));

  gfxImageSurface *retval = nullptr;
  surface.swap(retval);
  return retval;
}

gfxRect
nsSVGFilterInstance::UserSpaceToFilterSpace(const gfxRect& aRect) const
{
  gfxRect r = aRect - mFilterRegion.TopLeft();
  r.Scale(mFilterSpaceSize.width / mFilterRegion.Width(),
          mFilterSpaceSize.height / mFilterRegion.Height());
  return r;
}

gfxPoint
nsSVGFilterInstance::FilterSpaceToUserSpace(const gfxPoint& aPt) const
{
  return gfxPoint(aPt.x * mFilterRegion.Width() / mFilterSpaceSize.width + mFilterRegion.X(),
                  aPt.y * mFilterRegion.Height() / mFilterSpaceSize.height + mFilterRegion.Y());
}

gfxMatrix
nsSVGFilterInstance::GetUserSpaceToFilterSpaceTransform() const
{
  gfxFloat widthScale = mFilterSpaceSize.width / mFilterRegion.Width();
  gfxFloat heightScale = mFilterSpaceSize.height / mFilterRegion.Height();
  return gfxMatrix(widthScale, 0.0f,
                   0.0f, heightScale,
                   -mFilterRegion.X() * widthScale, -mFilterRegion.Y() * heightScale);
}

void
nsSVGFilterInstance::ComputeFilterPrimitiveSubregion(PrimitiveInfo* aPrimitive)
{
  nsSVGFE* fE = aPrimitive->mFE;

  gfxRect defaultFilterSubregion(0,0,0,0);
  if (fE->SubregionIsUnionOfRegions()) {
    for (uint32_t i = 0; i < aPrimitive->mInputs.Length(); ++i) {
      defaultFilterSubregion =
          defaultFilterSubregion.Union(
              aPrimitive->mInputs[i]->mImage.mFilterPrimitiveSubregion);
    }
  } else {
    defaultFilterSubregion =
      gfxRect(0, 0, mFilterSpaceSize.width, mFilterSpaceSize.height);
  }

  gfxRect feArea = nsSVGUtils::GetRelativeRect(mPrimitiveUnits,
    &fE->mLengthAttributes[nsSVGFE::X], mTargetBBox, mTargetFrame);
  gfxRect region = UserSpaceToFilterSpace(feArea);

  if (!fE->mLengthAttributes[nsSVGFE::X].IsExplicitlySet())
    region.x = defaultFilterSubregion.X();
  if (!fE->mLengthAttributes[nsSVGFE::Y].IsExplicitlySet())
    region.y = defaultFilterSubregion.Y();
  if (!fE->mLengthAttributes[nsSVGFE::WIDTH].IsExplicitlySet())
    region.width = defaultFilterSubregion.Width();
  if (!fE->mLengthAttributes[nsSVGFE::HEIGHT].IsExplicitlySet())
    region.height = defaultFilterSubregion.Height();

  
  
  
  region.RoundOut();
  aPrimitive->mImage.mFilterPrimitiveSubregion = region;
}

nsresult
nsSVGFilterInstance::BuildSources()
{
  gfxRect filterRegion = gfxRect(0, 0, mFilterSpaceSize.width, mFilterSpaceSize.height);
  mSourceColorAlpha.mImage.mFilterPrimitiveSubregion = filterRegion;
  mSourceAlpha.mImage.mFilterPrimitiveSubregion = filterRegion;
  mFillPaint.mImage.mFilterPrimitiveSubregion = filterRegion;
  mStrokePaint.mImage.mFilterPrimitiveSubregion = filterRegion;

  nsIntRect sourceBoundsInt;
  gfxRect sourceBounds = UserSpaceToFilterSpace(mTargetBBox);
  sourceBounds.RoundOut();
  
  if (!gfxUtils::GfxRectToIntRect(sourceBounds, &sourceBoundsInt))
    return NS_ERROR_FAILURE;
  sourceBoundsInt.UnionRect(sourceBoundsInt, mTargetBounds);

  mSourceColorAlpha.mResultBoundingBox = sourceBoundsInt;
  mSourceAlpha.mResultBoundingBox = sourceBoundsInt;
  mFillPaint.mResultBoundingBox = sourceBoundsInt;
  mStrokePaint.mResultBoundingBox = sourceBoundsInt;
  return NS_OK;
}

nsresult
nsSVGFilterInstance::BuildPrimitives()
{
  
  
  for (nsIContent* child = mFilterElement->nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    nsRefPtr<nsSVGFE> primitive;
    CallQueryInterface(child, (nsSVGFE**)getter_AddRefs(primitive));
    if (!primitive)
      continue;

    PrimitiveInfo* info = mPrimitives.AppendElement();
    info->mFE = primitive;
  }

  
  nsTHashtable<ImageAnalysisEntry> imageTable;
  imageTable.Init(10);

  for (uint32_t i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsSVGFE* filter = info->mFE;
    nsAutoTArray<nsSVGStringInfo,2> sources;
    filter->GetSourceImageNames(sources);
 
    for (uint32_t j=0; j<sources.Length(); ++j) {
      nsAutoString str;
      sources[j].mString->GetAnimValue(str, sources[j].mElement);
      PrimitiveInfo* sourceInfo;

      if (str.EqualsLiteral("SourceGraphic")) {
        sourceInfo = &mSourceColorAlpha;
      } else if (str.EqualsLiteral("SourceAlpha")) {
        sourceInfo = &mSourceAlpha;
      } else if (str.EqualsLiteral("FillPaint")) {
        sourceInfo = &mFillPaint;
      } else if (str.EqualsLiteral("StrokePaint")) {
        sourceInfo = &mStrokePaint;
      } else if (str.EqualsLiteral("BackgroundImage") ||
                 str.EqualsLiteral("BackgroundAlpha")) {
        return NS_ERROR_NOT_IMPLEMENTED;
      } else if (str.EqualsLiteral("")) {
        sourceInfo = i == 0 ? &mSourceColorAlpha : &mPrimitives[i - 1];
      } else {
        ImageAnalysisEntry* entry = imageTable.GetEntry(str);
        if (!entry)
          return NS_ERROR_FAILURE;
        sourceInfo = entry->mInfo;
      }
      
      ++sourceInfo->mImageUsers;
      info->mInputs.AppendElement(sourceInfo);
    }

    ComputeFilterPrimitiveSubregion(info);

    nsAutoString str;
    filter->GetResultImageName().GetAnimValue(str, filter);

    ImageAnalysisEntry* entry = imageTable.PutEntry(str);
    if (entry) {
      entry->mInfo = info;
    }
    
    
    if (i == mPrimitives.Length() - 1) {
      ++info->mImageUsers;
    }
  }

  return NS_OK;
}

void
nsSVGFilterInstance::ComputeResultBoundingBoxes()
{
  for (uint32_t i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceBBoxes;
    for (uint32_t j = 0; j < info->mInputs.Length(); ++j) {
      sourceBBoxes.AppendElement(info->mInputs[j]->mResultBoundingBox);
    }
    
    nsIntRect resultBBox = info->mFE->ComputeTargetBBox(sourceBBoxes, *this);
    ClipToFilterSpace(&resultBBox);
    nsSVGUtils::ClipToGfxRect(&resultBBox, info->mImage.mFilterPrimitiveSubregion);
    info->mResultBoundingBox = resultBBox;
  }
}

void
nsSVGFilterInstance::ComputeResultChangeBoxes()
{
  for (uint32_t i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceChangeBoxes;
    for (uint32_t j = 0; j < info->mInputs.Length(); ++j) {
      sourceChangeBoxes.AppendElement(info->mInputs[j]->mResultChangeBox);
    }

    nsIntRect resultChangeBox = info->mFE->ComputeChangeBBox(sourceChangeBoxes, *this);
    info->mResultChangeBox.IntersectRect(resultChangeBox, info->mResultBoundingBox);
  }
}

void
nsSVGFilterInstance::ComputeNeededBoxes()
{
  if (mPrimitives.IsEmpty())
    return;

  
  
  mPrimitives[mPrimitives.Length() - 1].mResultNeededBox.IntersectRect(
    mPrimitives[mPrimitives.Length() - 1].mResultBoundingBox, mPostFilterDirtyRect);

  for (int32_t i = mPrimitives.Length() - 1; i >= 0; --i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceBBoxes;
    for (uint32_t j = 0; j < info->mInputs.Length(); ++j) {
      sourceBBoxes.AppendElement(info->mInputs[j]->mResultBoundingBox);
    }
    
    info->mFE->ComputeNeededSourceBBoxes(
      info->mResultNeededBox, sourceBBoxes, *this);
    
    for (uint32_t j = 0; j < info->mInputs.Length(); ++j) {
      nsIntRect* r = &info->mInputs[j]->mResultNeededBox;
      r->UnionRect(*r, sourceBBoxes[j]);
      
      ClipToFilterSpace(r);
      nsSVGUtils::ClipToGfxRect(r, info->mInputs[j]->mImage.mFilterPrimitiveSubregion);
    }
  }
}

nsIntRect
nsSVGFilterInstance::ComputeUnionOfAllNeededBoxes()
{
  nsIntRect r;
  r.UnionRect(mSourceColorAlpha.mResultNeededBox,
              mSourceAlpha.mResultNeededBox);
  r.UnionRect(r, mFillPaint.mResultNeededBox);
  r.UnionRect(r, mStrokePaint.mResultNeededBox);
  for (uint32_t i = 0; i < mPrimitives.Length(); ++i) {
    r.UnionRect(r, mPrimitives[i].mResultNeededBox);
  }
  return r;
}

nsresult
nsSVGFilterInstance::BuildSourcePaint(PrimitiveInfo *aPrimitive)
{
  NS_ASSERTION(aPrimitive->mImageUsers > 0, "Some user must have needed this");

  nsRefPtr<gfxImageSurface> image = CreateImage();
  if (!image)
    return NS_ERROR_OUT_OF_MEMORY;

  nsRefPtr<gfxASurface> offscreen =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(
            gfxIntSize(mSurfaceRect.width, mSurfaceRect.height),
            gfxASurface::CONTENT_COLOR_ALPHA);
  if (!offscreen || offscreen->CairoStatus())
    return NS_ERROR_OUT_OF_MEMORY;
  offscreen->SetDeviceOffset(gfxPoint(-mSurfaceRect.x, -mSurfaceRect.y));

  nsRenderingContext tmpCtx;
  tmpCtx.Init(mTargetFrame->PresContext()->DeviceContext(), offscreen);

  gfxRect r = aPrimitive->mImage.mFilterPrimitiveSubregion;
  gfxMatrix m = GetUserSpaceToFilterSpaceTransform();
  m.Invert();
  r = m.TransformBounds(r);

  gfxMatrix deviceToFilterSpace = GetFilterSpaceToDeviceSpaceTransform().Invert();
  gfxContext *gfx = tmpCtx.ThebesContext();
  gfx->Multiply(deviceToFilterSpace);

  gfx->Save();

  gfxMatrix matrix =
    nsSVGUtils::GetCanvasTM(mTargetFrame, nsISVGChildFrame::FOR_PAINTING);
  if (!matrix.IsSingular()) {
    gfx->Multiply(matrix);
    gfx->Rectangle(r);
    if ((aPrimitive == &mFillPaint && 
         nsSVGUtils::SetupCairoFillPaint(mTargetFrame, gfx)) ||
        (aPrimitive == &mStrokePaint &&
         nsSVGUtils::SetupCairoStrokePaint(mTargetFrame, gfx))) {
      gfx->Fill();
    }
  }
  gfx->Restore();

  gfxContext copyContext(image);
  copyContext.SetSource(offscreen);
  copyContext.Paint();

  aPrimitive->mImage.mImage = image;
  

  return NS_OK;
}

nsresult
nsSVGFilterInstance::BuildSourcePaints()
{
  nsresult rv = NS_OK;

  if (!mFillPaint.mResultNeededBox.IsEmpty()) {
    rv = BuildSourcePaint(&mFillPaint);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mStrokePaint.mResultNeededBox.IsEmpty()) {
    rv = BuildSourcePaint(&mStrokePaint);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return  rv;
}

nsresult
nsSVGFilterInstance::BuildSourceImages()
{
  nsIntRect neededRect;
  neededRect.UnionRect(mSourceColorAlpha.mResultNeededBox,
                       mSourceAlpha.mResultNeededBox);
  if (neededRect.IsEmpty())
    return NS_OK;

  nsRefPtr<gfxImageSurface> sourceColorAlpha = CreateImage();
  if (!sourceColorAlpha)
    return NS_ERROR_OUT_OF_MEMORY;

  {
    
    
    
    nsRefPtr<gfxASurface> offscreen =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(
              gfxIntSize(mSurfaceRect.width, mSurfaceRect.height),
              gfxASurface::CONTENT_COLOR_ALPHA);
    if (!offscreen || offscreen->CairoStatus())
      return NS_ERROR_OUT_OF_MEMORY;
    offscreen->SetDeviceOffset(gfxPoint(-mSurfaceRect.x, -mSurfaceRect.y));

    nsRenderingContext tmpCtx;
    tmpCtx.Init(mTargetFrame->PresContext()->DeviceContext(), offscreen);

    gfxRect r(neededRect.x, neededRect.y, neededRect.width, neededRect.height);
    gfxMatrix m = GetUserSpaceToFilterSpaceTransform();
    m.Invert();
    r = m.TransformBounds(r);
    r.RoundOut();
    nsIntRect dirty;
    if (!gfxUtils::GfxRectToIntRect(r, &dirty))
      return NS_ERROR_FAILURE;

    
    
    
    
    
    
    
    
    
    
    
    gfxMatrix deviceToFilterSpace = GetFilterSpaceToDeviceSpaceTransform().Invert();
    tmpCtx.ThebesContext()->Multiply(deviceToFilterSpace);
    mPaintCallback->Paint(&tmpCtx, mTargetFrame, &dirty);

    gfxContext copyContext(sourceColorAlpha);
    copyContext.SetSource(offscreen);
    copyContext.Paint();
  }

  if (!mSourceColorAlpha.mResultNeededBox.IsEmpty()) {
    NS_ASSERTION(mSourceColorAlpha.mImageUsers > 0, "Some user must have needed this");
    mSourceColorAlpha.mImage.mImage = sourceColorAlpha;
    
  }

  if (!mSourceAlpha.mResultNeededBox.IsEmpty()) {
    NS_ASSERTION(mSourceAlpha.mImageUsers > 0, "Some user must have needed this");

    mSourceAlpha.mImage.mImage = CreateImage();
    if (!mSourceAlpha.mImage.mImage)
      return NS_ERROR_OUT_OF_MEMORY;
    

    
    const uint32_t* src = reinterpret_cast<uint32_t*>(sourceColorAlpha->Data());
    uint32_t* dest = reinterpret_cast<uint32_t*>(mSourceAlpha.mImage.mImage->Data());
    for (int32_t y = 0; y < mSurfaceRect.height; y++) {
      uint32_t rowOffset = (mSourceAlpha.mImage.mImage->Stride()*y) >> 2;
      for (int32_t x = 0; x < mSurfaceRect.width; x++) {
        dest[rowOffset + x] = src[rowOffset + x] & 0xFF000000U;
      }
    }
    mSourceAlpha.mImage.mConstantColorChannels = true;
  }
  
  return NS_OK;
}

void
nsSVGFilterInstance::EnsureColorModel(PrimitiveInfo* aPrimitive,
                                      ColorModel aColorModel)
{
  ColorModel currentModel = aPrimitive->mImage.mColorModel;
  if (aColorModel == currentModel)
    return;

  uint8_t* data = aPrimitive->mImage.mImage->Data();
  int32_t stride = aPrimitive->mImage.mImage->Stride();

  nsIntRect r = aPrimitive->mResultNeededBox - mSurfaceRect.TopLeft();

  if (currentModel.mAlphaChannel == ColorModel::PREMULTIPLIED) {
    nsSVGUtils::UnPremultiplyImageDataAlpha(data, stride, r);
  }
  if (aColorModel.mColorSpace != currentModel.mColorSpace) {
    if (aColorModel.mColorSpace == ColorModel::LINEAR_RGB) {
      nsSVGUtils::ConvertImageDataToLinearRGB(data, stride, r);
    } else {
      nsSVGUtils::ConvertImageDataFromLinearRGB(data, stride, r);
    }
  }
  if (aColorModel.mAlphaChannel == ColorModel::PREMULTIPLIED) {
    nsSVGUtils::PremultiplyImageDataAlpha(data, stride, r);
  }
  aPrimitive->mImage.mColorModel = aColorModel;
}

nsresult
nsSVGFilterInstance::Render(gfxASurface** aOutput)
{
  *aOutput = nullptr;

  nsresult rv = BuildSources();
  if (NS_FAILED(rv))
    return rv;

  rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitives.IsEmpty()) {
    
    return NS_OK;
  }

  ComputeResultBoundingBoxes();
  ComputeNeededBoxes();
  
  
  mSurfaceRect = ComputeUnionOfAllNeededBoxes();

  rv = BuildSourceImages();
  if (NS_FAILED(rv))
    return rv;
  rv = BuildSourcePaints();
  if (NS_FAILED(rv))
    return rv;

  for (uint32_t i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* primitive = &mPrimitives[i];

    nsIntRect dataRect;
    
    
    if (!dataRect.IntersectRect(primitive->mResultNeededBox, mSurfaceRect))
      continue;
    dataRect -= mSurfaceRect.TopLeft();

    primitive->mImage.mImage = CreateImage();
    if (!primitive->mImage.mImage)
      return NS_ERROR_OUT_OF_MEMORY;

    nsAutoTArray<const Image*,2> inputs;
    for (uint32_t j = 0; j < primitive->mInputs.Length(); ++j) {
      PrimitiveInfo* input = primitive->mInputs[j];
      
      if (!input->mImage.mImage) {
        
        
        input->mImage.mImage = CreateImage();
        if (!input->mImage.mImage)
          return NS_ERROR_OUT_OF_MEMORY;
      }
      
      ColorModel desiredColorModel =
        primitive->mFE->GetInputColorModel(this, j, &input->mImage);
      if (j == 0) {
        
        primitive->mImage.mColorModel = desiredColorModel;
      }
      EnsureColorModel(input, desiredColorModel);
      NS_ASSERTION(input->mImage.mImage->Stride() == primitive->mImage.mImage->Stride(),
                   "stride mismatch");
      inputs.AppendElement(&input->mImage);
    }

    if (primitive->mInputs.Length() == 0) {
      primitive->mImage.mColorModel = primitive->mFE->GetOutputColorModel(this);
    }

    rv = primitive->mFE->Filter(this, inputs, &primitive->mImage, dataRect);
    if (NS_FAILED(rv))
      return rv;

    for (uint32_t j = 0; j < primitive->mInputs.Length(); ++j) {
      PrimitiveInfo* input = primitive->mInputs[j];
      --input->mImageUsers;
      NS_ASSERTION(input->mImageUsers >= 0, "Bad mImageUsers tracking");
      if (input->mImageUsers == 0) {
        
        input->mImage.mImage = nullptr;
      }
    }
  }
  
  PrimitiveInfo* result = &mPrimitives[mPrimitives.Length() - 1];
  ColorModel premulSRGB; 
  EnsureColorModel(result, premulSRGB);
  gfxImageSurface* surf = nullptr;
  result->mImage.mImage.swap(surf);
  *aOutput = surf;
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputePostFilterDirtyRect(nsIntRect* aPostFilterDirtyRect)
{
  *aPostFilterDirtyRect = nsIntRect();

  nsresult rv = BuildSources();
  if (NS_FAILED(rv))
    return rv;

  rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitives.IsEmpty()) {
    
    return NS_OK;
  }

  ComputeResultBoundingBoxes();

  mSourceColorAlpha.mResultChangeBox = mPreFilterDirtyRect;
  mSourceAlpha.mResultChangeBox = mPreFilterDirtyRect;
  ComputeResultChangeBoxes();

  PrimitiveInfo* result = &mPrimitives[mPrimitives.Length() - 1];
  *aPostFilterDirtyRect = result->mResultChangeBox;
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputeSourceNeededRect(nsIntRect* aDirty)
{
  nsresult rv = BuildSources();
  if (NS_FAILED(rv))
    return rv;

  rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitives.IsEmpty()) {
    
    return NS_OK;
  }

  ComputeResultBoundingBoxes();
  ComputeNeededBoxes();
  aDirty->UnionRect(mSourceColorAlpha.mResultNeededBox,
                    mSourceAlpha.mResultNeededBox);
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputeOutputBBox(nsIntRect* aDirty)
{
  nsresult rv = BuildSources();
  if (NS_FAILED(rv))
    return rv;

  rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitives.IsEmpty()) {
    
    *aDirty = nsIntRect();
    return NS_OK;
  }

  ComputeResultBoundingBoxes();

  PrimitiveInfo* result = &mPrimitives[mPrimitives.Length() - 1];
  *aDirty = result->mResultBoundingBox;
  return NS_OK;
}
