






#include "SkImageView.h"
#include "SkAnimator.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkImageDecoder.h"
#include "SkMatrix.h"
#include "SkSystemEventTypes.h"
#include "SkTime.h"

SkImageView::SkImageView()
{
	fMatrix		= NULL;
	fScaleType	= kMatrix_ScaleType;

	fData.fAnim	= NULL;		
	fDataIsAnim	= true;
	
	fUriIsValid	= false;	
}

SkImageView::~SkImageView()
{
	if (fMatrix)
		sk_free(fMatrix);
		
	this->freeData();
}

void SkImageView::getUri(SkString* uri) const
{
	if (uri)
		*uri = fUri;
}

void SkImageView::setUri(const char uri[])
{
	if (!fUri.equals(uri))
	{
		fUri.set(uri);
		this->onUriChange();
	}
}

void SkImageView::setUri(const SkString& uri)
{
	if (fUri != uri)
	{
		fUri = uri;
		this->onUriChange();
	}
}

void SkImageView::setScaleType(ScaleType st)
{
	SkASSERT((unsigned)st <= kFitEnd_ScaleType);

	if ((ScaleType)fScaleType != st)
	{
		fScaleType = SkToU8(st);
		if (fUriIsValid)
			this->inval(NULL);
	}
}

bool SkImageView::getImageMatrix(SkMatrix* matrix) const
{
	if (fMatrix)
	{
		SkASSERT(!fMatrix->isIdentity());
		if (matrix)
			*matrix = *fMatrix;
		return true;
	}
	else
	{
		if (matrix)
			matrix->reset();
		return false;
	}
}

void SkImageView::setImageMatrix(const SkMatrix* matrix)
{
	bool changed = false;

	if (matrix && !matrix->isIdentity())
	{
		if (fMatrix == NULL)
			fMatrix = (SkMatrix*)sk_malloc_throw(sizeof(SkMatrix));
		*fMatrix = *matrix;
		changed = true;
	}
	else	
	{
		if (fMatrix)
		{
			SkASSERT(!fMatrix->isIdentity());
			sk_free(fMatrix);
			fMatrix = NULL;
			changed = true;
		}
	}

	
	if (changed && this->getScaleType() == kMatrix_ScaleType && fUriIsValid)
		this->inval(NULL);
}



bool SkImageView::onEvent(const SkEvent& evt)
{
	if (evt.isType(SK_EventType_Inval))
	{
		if (fUriIsValid)
			this->inval(NULL);
		return true;
	}
	return this->INHERITED::onEvent(evt);
}

static inline SkMatrix::ScaleToFit scaleTypeToScaleToFit(SkImageView::ScaleType st)
{
	SkASSERT(st != SkImageView::kMatrix_ScaleType);
	SkASSERT((unsigned)st <= SkImageView::kFitEnd_ScaleType);

	SkASSERT(SkImageView::kFitXY_ScaleType - 1 == SkMatrix::kFill_ScaleToFit);
	SkASSERT(SkImageView::kFitStart_ScaleType - 1 == SkMatrix::kStart_ScaleToFit);
	SkASSERT(SkImageView::kFitCenter_ScaleType - 1 == SkMatrix::kCenter_ScaleToFit);
	SkASSERT(SkImageView::kFitEnd_ScaleType - 1 == SkMatrix::kEnd_ScaleToFit);
	
	return (SkMatrix::ScaleToFit)(st - 1);
}

void SkImageView::onDraw(SkCanvas* canvas)
{
	SkRect	src;
	if (!this->getDataBounds(&src))
	{
		SkDEBUGCODE(canvas->drawColor(SK_ColorRED);)
		return;		
	}
		
	SkAutoCanvasRestore	restore(canvas, true);
	SkMatrix			matrix;
	
	if (this->getScaleType() == kMatrix_ScaleType)
		(void)this->getImageMatrix(&matrix);
	else
	{
		SkRect	dst;		
		dst.set(0, 0, this->width(), this->height());
		matrix.setRectToRect(src, dst, scaleTypeToScaleToFit(this->getScaleType()));
	}
	canvas->concat(matrix);

	SkPaint	paint;
	
	paint.setAntiAlias(true);

	if (fDataIsAnim)
	{
		SkMSec	now = SkTime::GetMSecs();
		
		SkAnimator::DifferenceType diff = fData.fAnim->draw(canvas, &paint, now);
		
SkDEBUGF(("SkImageView : now = %X[%12.3f], diff = %d\n", now, now/1000., diff));

		if (diff == SkAnimator::kDifferent)
			this->inval(NULL);
		else if (diff == SkAnimator::kPartiallyDifferent)
		{
			SkRect	bounds;
			fData.fAnim->getInvalBounds(&bounds);
			matrix.mapRect(&bounds);	
			this->inval(&bounds);
		}
	}
	else
		canvas->drawBitmap(*fData.fBitmap, 0, 0, &paint);
}

void SkImageView::onInflate(const SkDOM& dom, const SkDOMNode* node)
{
	this->INHERITED::onInflate(dom, node);
	
	const char* src = dom.findAttr(node, "src");
	if (src)
		this->setUri(src);

	int	index = dom.findList(node, "scaleType", "matrix,fitXY,fitStart,fitCenter,fitEnd");
	if (index >= 0)
		this->setScaleType((ScaleType)index);
		
	
}



void SkImageView::onUriChange()
{
	if (this->freeData())
		this->inval(NULL);
	fUriIsValid = true;		
}

bool SkImageView::freeData()
{
	if (fData.fAnim)	
	{
		if (fDataIsAnim)
			delete fData.fAnim;
		else
			delete fData.fBitmap;

		fData.fAnim = NULL;	
		return true;
	}
	return false;
}

bool SkImageView::getDataBounds(SkRect* bounds)
{
	SkASSERT(bounds);

	if (this->ensureUriIsLoaded())
	{
		SkScalar width, height;

		if (fDataIsAnim)
		{			
			if (SkScalarIsNaN(width = fData.fAnim->getScalar("dimensions", "x")) ||
				SkScalarIsNaN(height = fData.fAnim->getScalar("dimensions", "y")))
			{
				
				width = this->width();
				height = this->height();
			}
		}
		else
		{
			width = SkIntToScalar(fData.fBitmap->width());
			height = SkIntToScalar(fData.fBitmap->height());
		}
		bounds->set(0, 0, width, height);
		return true;
	}
	return false;
}

bool SkImageView::ensureUriIsLoaded()
{
	if (fData.fAnim)	
	{
		SkASSERT(fUriIsValid);
		return true;
	}
	if (!fUriIsValid)
		return false;

	
	if (fUri.endsWith(".xml"))	
	{
		SkAnimator* anim = new SkAnimator;
		
		if (!anim->decodeURI(fUri.c_str()))
		{
			delete anim;
			fUriIsValid = false;
			return false;
		}
		anim->setHostEventSink(this);

		fData.fAnim = anim;
		fDataIsAnim = true;
	}
	else	
	{
    #if 0
		SkBitmap* bitmap = new SkBitmap;

		if (!SkImageDecoder::DecodeURL(fUri.c_str(), bitmap))
		{
			delete bitmap;
			fUriIsValid = false;
			return false;
		}
		fData.fBitmap = bitmap;
		fDataIsAnim = false;
    #else
        return false;
    #endif
	}
	return true;
}

