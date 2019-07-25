



































#include "nsSVGFilterInstance.h"
#include "nsSVGUtils.h"
#include "nsIDOMSVGUnitTypes.h"
#include "gfxPlatform.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGFilterElement.h"
#include "nsLayoutUtils.h"
#include "gfxUtils.h"

static double Square(double aX)
{
  return aX*aX;
}

float
nsSVGFilterInstance::GetPrimitiveLength(nsSVGLength2 *aLength) const
{
  float value;
  if (mPrimitiveUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    value = nsSVGUtils::ObjectSpace(mTargetBBox, aLength);
  } else {
    value = nsSVGUtils::UserSpace(mTargetFrame, aLength);
  }

  switch (aLength->GetCtxType()) {
  case nsSVGUtils::X:
    return value * mFilterSpaceSize.width / mFilterRect.Width();
  case nsSVGUtils::Y:
    return value * mFilterSpaceSize.height / mFilterRect.Height();
  case nsSVGUtils::XY:
  default:
    return value *
      sqrt(Square(mFilterSpaceSize.width) + Square(mFilterSpaceSize.height)) /
      sqrt(Square(mFilterRect.Width()) + Square(mFilterRect.Height()));
  }
}

void
nsSVGFilterInstance::ConvertLocation(float aValues[3]) const
{
  if (mPrimitiveUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    aValues[0] *= mTargetBBox.Width();
    aValues[1] *= mTargetBBox.Height();
    aValues[2] *= nsSVGUtils::ComputeNormalizedHypotenuse(
                    mTargetBBox.Width(), mTargetBBox.Height());
  }
}

already_AddRefed<gfxImageSurface>
nsSVGFilterInstance::CreateImage()
{
  nsRefPtr<gfxImageSurface> surface =
    new gfxImageSurface(gfxIntSize(mSurfaceRect.width, mSurfaceRect.height),
                        gfxASurface::ImageFormatARGB32);

  if (!surface || surface->CairoStatus())
    return nsnull;

  surface->SetDeviceOffset(gfxPoint(-mSurfaceRect.x, -mSurfaceRect.y));

  gfxImageSurface *retval = nsnull;
  surface.swap(retval);
  return retval;
}

gfxRect
nsSVGFilterInstance::UserSpaceToFilterSpace(const gfxRect& aRect) const
{
  gfxRect r = aRect - mFilterRect.TopLeft();
  r.Scale(mFilterSpaceSize.width / mFilterRect.Width(),
          mFilterSpaceSize.height / mFilterRect.Height());
  return r;
}

gfxMatrix
nsSVGFilterInstance::GetUserSpaceToFilterSpaceTransform() const
{
  gfxFloat widthScale = mFilterSpaceSize.width / mFilterRect.Width();
  gfxFloat heightScale = mFilterSpaceSize.height / mFilterRect.Height();
  return gfxMatrix(widthScale, 0.0f,
                   0.0f, heightScale,
                   -mFilterRect.X() * widthScale, -mFilterRect.Y() * heightScale);
}

void
nsSVGFilterInstance::ComputeFilterPrimitiveSubregion(PrimitiveInfo* aPrimitive)
{
  nsSVGFE* fE = aPrimitive->mFE;

  gfxRect defaultFilterSubregion(0,0,0,0);
  if (fE->SubregionIsUnionOfRegions()) {
    for (PRUint32 i = 0; i < aPrimitive->mInputs.Length(); ++i) {
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

  if (!fE->HasAttr(kNameSpaceID_None, nsGkAtoms::x))
    region.x = defaultFilterSubregion.X();
  if (!fE->HasAttr(kNameSpaceID_None, nsGkAtoms::y))
    region.y = defaultFilterSubregion.Y();
  if (!fE->HasAttr(kNameSpaceID_None, nsGkAtoms::width))
    region.width = defaultFilterSubregion.Width();
  if (!fE->HasAttr(kNameSpaceID_None, nsGkAtoms::height))
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

  nsIntRect sourceBoundsInt;
  gfxRect sourceBounds = UserSpaceToFilterSpace(mTargetBBox);
  sourceBounds.RoundOut();
  
  if (!gfxUtils::GfxRectToIntRect(sourceBounds, &sourceBoundsInt))
    return NS_ERROR_FAILURE;

  mSourceColorAlpha.mResultBoundingBox = sourceBoundsInt;
  mSourceAlpha.mResultBoundingBox = sourceBoundsInt;
  return NS_OK;
}

nsresult
nsSVGFilterInstance::BuildPrimitives()
{
  
  
  PRUint32 count = mFilterElement->GetChildCount();
  PRUint32 i;
  for (i = 0; i < count; ++i) {
    nsIContent* child = mFilterElement->GetChildAt(i);
    nsRefPtr<nsSVGFE> primitive;
    CallQueryInterface(child, (nsSVGFE**)getter_AddRefs(primitive));
    if (!primitive)
      continue;

    PrimitiveInfo* info = mPrimitives.AppendElement();
    info->mFE = primitive;
  }

  
  nsTHashtable<ImageAnalysisEntry> imageTable;
  imageTable.Init(10);

  for (i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsSVGFE* filter = info->mFE;
    nsAutoTArray<nsSVGStringInfo,2> sources;
    filter->GetSourceImageNames(sources);
 
    for (PRUint32 j=0; j<sources.Length(); ++j) {
      nsAutoString str;
      sources[j].mString->GetAnimValue(str, sources[j].mElement);
      PrimitiveInfo* sourceInfo;

      if (str.EqualsLiteral("SourceGraphic")) {
        sourceInfo = &mSourceColorAlpha;
      } else if (str.EqualsLiteral("SourceAlpha")) {
        sourceInfo = &mSourceAlpha;
      } else if (str.EqualsLiteral("BackgroundImage") ||
                 str.EqualsLiteral("BackgroundAlpha") ||
                 str.EqualsLiteral("FillPaint") ||
                 str.EqualsLiteral("StrokePaint")) {
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
  for (PRUint32 i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceBBoxes;
    for (PRUint32 j = 0; j < info->mInputs.Length(); ++j) {
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
  for (PRUint32 i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceChangeBoxes;
    for (PRUint32 j = 0; j < info->mInputs.Length(); ++j) {
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
    mPrimitives[mPrimitives.Length() - 1].mResultBoundingBox, mDirtyOutputRect);

  for (PRInt32 i = mPrimitives.Length() - 1; i >= 0; --i) {
    PrimitiveInfo* info = &mPrimitives[i];
    nsAutoTArray<nsIntRect,2> sourceBBoxes;
    for (PRUint32 j = 0; j < info->mInputs.Length(); ++j) {
      sourceBBoxes.AppendElement(info->mInputs[j]->mResultBoundingBox);
    }
    
    info->mFE->ComputeNeededSourceBBoxes(
      info->mResultNeededBox, sourceBBoxes, *this);
    
    for (PRUint32 j = 0; j < info->mInputs.Length(); ++j) {
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
  for (PRUint32 i = 0; i < mPrimitives.Length(); ++i) {
    r.UnionRect(r, mPrimitives[i].mResultNeededBox);
  }
  return r;
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
  
    nsSVGRenderState tmpState(offscreen);
    gfxMatrix userSpaceToFilterSpace = GetUserSpaceToFilterSpaceTransform();

    gfxRect r(neededRect.x, neededRect.y, neededRect.width, neededRect.height);
    gfxMatrix m = userSpaceToFilterSpace;
    m.Invert();
    r = m.TransformBounds(r);
    r.RoundOut();
    nsIntRect dirty;
    if (!gfxUtils::GfxRectToIntRect(r, &dirty))
      return NS_ERROR_FAILURE;

    
    
    
    
    
    
    
    
    
    
    
    gfxMatrix deviceToFilterSpace = GetFilterSpaceToDeviceSpaceTransform().Invert();
    tmpState.GetGfxContext()->Multiply(deviceToFilterSpace);
    mPaintCallback->Paint(&tmpState, mTargetFrame, &dirty);

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
    

    
    const PRUint32* src = reinterpret_cast<PRUint32*>(sourceColorAlpha->Data());
    PRUint32* dest = reinterpret_cast<PRUint32*>(mSourceAlpha.mImage.mImage->Data());
    for (PRInt32 y = 0; y < mSurfaceRect.height; y++) {
      PRUint32 rowOffset = (mSourceAlpha.mImage.mImage->Stride()*y) >> 2;
      for (PRInt32 x = 0; x < mSurfaceRect.width; x++) {
        dest[rowOffset + x] = src[rowOffset + x] & 0xFF000000U;
      }
    }
    mSourceAlpha.mImage.mConstantColorChannels = PR_TRUE;
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

  PRUint8* data = aPrimitive->mImage.mImage->Data();
  PRInt32 stride = aPrimitive->mImage.mImage->Stride();

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
  *aOutput = nsnull;

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

  for (PRUint32 i = 0; i < mPrimitives.Length(); ++i) {
    PrimitiveInfo* primitive = &mPrimitives[i];

    nsIntRect dataRect;
    
    
    if (!dataRect.IntersectRect(primitive->mResultNeededBox, mSurfaceRect))
      continue;
    dataRect -= mSurfaceRect.TopLeft();

    primitive->mImage.mImage = CreateImage();
    if (!primitive->mImage.mImage)
      return NS_ERROR_OUT_OF_MEMORY;

    nsAutoTArray<const Image*,2> inputs;
    for (PRUint32 j = 0; j < primitive->mInputs.Length(); ++j) {
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

    for (PRUint32 j = 0; j < primitive->mInputs.Length(); ++j) {
      PrimitiveInfo* input = primitive->mInputs[j];
      --input->mImageUsers;
      NS_ASSERTION(input->mImageUsers >= 0, "Bad mImageUsers tracking");
      if (input->mImageUsers == 0) {
        
        input->mImage.mImage = nsnull;
      }
    }
  }
  
  PrimitiveInfo* result = &mPrimitives[mPrimitives.Length() - 1];
  ColorModel premulSRGB; 
  EnsureColorModel(result, premulSRGB);
  gfxImageSurface* surf = nsnull;
  result->mImage.mImage.swap(surf);
  *aOutput = surf;
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputeOutputDirtyRect(nsIntRect* aDirty)
{
  *aDirty = nsIntRect();

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

  mSourceColorAlpha.mResultChangeBox = mDirtyInputRect;
  mSourceAlpha.mResultChangeBox = mDirtyInputRect;
  ComputeResultChangeBoxes();

  PrimitiveInfo* result = &mPrimitives[mPrimitives.Length() - 1];
  *aDirty = result->mResultChangeBox;
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
