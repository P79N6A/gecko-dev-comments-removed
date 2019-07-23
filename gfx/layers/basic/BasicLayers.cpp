




































#include "BasicLayers.h"
#include "nsTArray.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"

namespace mozilla {
namespace layers {

class BasicContainerLayer;





class BasicImplData {
public:
  BasicImplData()
  {
    MOZ_COUNT_CTOR(BasicImplData);
  }
  ~BasicImplData()
  {
    MOZ_COUNT_DTOR(BasicImplData);
  }

  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }

protected:
  nsIntRegion mVisibleRegion;
};

static BasicImplData*
ToData(Layer* aLayer)
{
  return static_cast<BasicImplData*>(aLayer->ImplData());
}

class BasicContainerLayer : public ContainerLayer, BasicImplData {
public:
  BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
  }
  virtual ~BasicContainerLayer();

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mVisibleRegion = aRegion;
  }
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);
  virtual void RemoveChild(Layer* aChild);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

BasicContainerLayer::~BasicContainerLayer()
{
  while (mFirstChild) {
    Layer* next = mFirstChild->GetNextSibling();
    mFirstChild->SetNextSibling(nsnull);
    mFirstChild->SetPrevSibling(nsnull);
    mFirstChild->SetParent(nsnull);
    NS_RELEASE(mFirstChild);
    mFirstChild = next;
  }

  MOZ_COUNT_DTOR(BasicContainerLayer);
}

void
BasicContainerLayer::InsertAfter(Layer* aChild, Layer* aAfter)
{
  NS_ASSERTION(BasicManager()->InConstruction(),
               "Can only set properties in construction phase");
  NS_ASSERTION(aChild->Manager() == Manager(),
               "Child has wrong manager");
  NS_ASSERTION(!aChild->GetParent(),
               "aChild already in the tree");
  NS_ASSERTION(!aChild->GetNextSibling() && !aChild->GetPrevSibling(),
               "aChild already has siblings?");
  NS_ASSERTION(!aAfter ||
               (aAfter->Manager() == Manager() &&
                aAfter->GetParent() == this),
               "aAfter is not our child");

  NS_ADDREF(aChild);

  aChild->SetParent(this);
  if (!aAfter) {
    aChild->SetNextSibling(mFirstChild);
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(aChild);
    }
    mFirstChild = aChild;
    return;
  }

  Layer* next = aAfter->GetNextSibling();
  aChild->SetNextSibling(next);
  aChild->SetPrevSibling(aAfter);
  if (next) {
    next->SetPrevSibling(aChild);
  }
  aAfter->SetNextSibling(aChild);
}

void
BasicContainerLayer::RemoveChild(Layer* aChild)
{
  NS_ASSERTION(BasicManager()->InConstruction(),
               "Can only set properties in construction phase");
  NS_ASSERTION(aChild->Manager() == Manager(),
               "Child has wrong manager");
  NS_ASSERTION(aChild->GetParent() == this,
               "aChild not our child");

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  }

  aChild->SetNextSibling(nsnull);
  aChild->SetPrevSibling(nsnull);
  aChild->SetParent(nsnull);

  NS_RELEASE(aChild);
}







class BasicThebesLayer : public ThebesLayer, BasicImplData {
public:
  BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicThebesLayer);
  }
  virtual ~BasicThebesLayer()
  {
    MOZ_COUNT_DTOR(BasicThebesLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mVisibleRegion = aRegion;
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
  }

  virtual gfxContext* BeginDrawing(nsIntRegion* aRegionToDraw);
  virtual void EndDrawing();
  virtual void CopyFrom(ThebesLayer* aSource,
                        const nsIntRegion& aRegion,
                        const nsIntPoint& aDelta);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

gfxContext*
BasicThebesLayer::BeginDrawing(nsIntRegion* aRegionToDraw)
{
  NS_ASSERTION(BasicManager()->IsBeforeInTree(BasicManager()->GetLastPainted(), this),
               "Painting layers out of order");
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  gfxContext* target = BasicManager()->GetTarget();
  if (!target)
    return nsnull;

  BasicManager()->AdvancePaintingTo(this);

  *aRegionToDraw = mVisibleRegion;
  return target;
}

void
BasicThebesLayer::EndDrawing()
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  NS_ASSERTION(BasicManager()->GetLastPainted() == this,
               "Not currently drawing this layer");
}

void
BasicThebesLayer::CopyFrom(ThebesLayer* aSource,
                           const nsIntRegion& aRegion,
                           const nsIntPoint& aDelta)
{
  NS_ASSERTION(!BasicManager()->IsBeforeInTree(aSource, this),
               "aSource must not be before this layer in tree");
  NS_ASSERTION(BasicManager()->IsBeforeInTree(BasicManager()->GetLastPainted(), this),
               "Cannot copy into a layer already painted");
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  
  
  
}

BasicLayerManager::BasicLayerManager(gfxContext* aContext) :
  mDefaultTarget(aContext), mLastPainted(nsnull)
#ifdef DEBUG
  , mPhase(PHASE_NONE)
#endif
{
  MOZ_COUNT_CTOR(BasicLayerManager);
}

BasicLayerManager::~BasicLayerManager()
{
  NS_ASSERTION(mPhase == PHASE_NONE, "Died during transaction?");
  MOZ_COUNT_DTOR(BasicLayerManager);
}

void
BasicLayerManager::SetDefaultTarget(gfxContext* aContext)
{
  NS_ASSERTION(mPhase == PHASE_NONE,
               "Must set default target outside transaction");
  mDefaultTarget = aContext;
}

void
BasicLayerManager::BeginTransaction()
{
  NS_ASSERTION(mPhase == PHASE_NONE, "Nested transactions not allowed");
#ifdef DEBUG
  mPhase = PHASE_CONSTRUCTION;
#endif
  mTarget = mDefaultTarget;
}

void
BasicLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  NS_ASSERTION(mPhase == PHASE_NONE, "Nested transactions not allowed");
#ifdef DEBUG
  mPhase = PHASE_CONSTRUCTION;
#endif
  mTarget = aTarget;
}

void
BasicLayerManager::EndConstruction()
{
  NS_ASSERTION(mRoot, "Root not set");
  NS_ASSERTION(mPhase == PHASE_CONSTRUCTION, "Should be in construction phase");
#ifdef DEBUG
  mPhase = PHASE_DRAWING;
#endif
}

void
BasicLayerManager::EndTransaction()
{
  NS_ASSERTION(mPhase == PHASE_DRAWING, "Should be in drawing phase");
#ifdef DEBUG
  mPhase = PHASE_NONE;
#endif
  AdvancePaintingTo(nsnull);
  mTarget = nsnull;
  
  mRoot = nsnull;
}

void
BasicLayerManager::SetRoot(Layer* aLayer)
{
  NS_ASSERTION(aLayer, "Root can't be null");
  NS_ASSERTION(aLayer->Manager() == this, "Wrong manager");
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  mRoot = aLayer;
}


static PRBool
NeedsGroup(Layer* aLayer)
{
  return aLayer->GetOpacity() != 1.0;
}




static PRBool
NeedsState(Layer* aLayer)
{
  return aLayer->GetClipRect() != nsnull ||
         !aLayer->GetTransform().IsIdentity();
}






static PRBool
UseOpaqueSurface(Layer* aLayer)
{
  
  
  if (aLayer->IsOpaqueContent())
    return PR_TRUE;
  
  
  
  
  BasicContainerLayer* parent =
    static_cast<BasicContainerLayer*>(aLayer->GetParent());
  return parent && parent->GetFirstChild() == aLayer &&
         UseOpaqueSurface(parent);
}

void
BasicLayerManager::BeginPaintingLayer(Layer* aLayer)
{
  PRBool needsGroup = NeedsGroup(aLayer);
  if ((needsGroup || NeedsState(aLayer)) && mTarget) {
    mTarget->Save();

    if (aLayer->GetClipRect()) {
      const nsIntRect& r = *aLayer->GetClipRect();
      mTarget->NewPath();
      mTarget->Rectangle(gfxRect(r.x, r.y, r.width, r.height));
      mTarget->Clip();
    }

    gfxMatrix transform;
    
    
    NS_ASSERTION(aLayer->GetTransform().Is2D(),
                 "Only 2D transforms supported currently");
    aLayer->GetTransform().Is2D(&transform);
    mTarget->Multiply(transform);

    if (needsGroup) {
      
      
      nsIntRect bbox = ToData(aLayer)->GetVisibleRegion().GetBounds();
      gfxRect deviceRect =
        mTarget->UserToDevice(gfxRect(bbox.x, bbox.y, bbox.width, bbox.height));
      deviceRect.RoundOut();

      gfxMatrix currentMatrix = mTarget->CurrentMatrix();
      mTarget->IdentityMatrix();
      mTarget->NewPath();
      mTarget->Rectangle(deviceRect);
      mTarget->Clip();
      mTarget->SetMatrix(currentMatrix);

      gfxASurface::gfxContentType type = UseOpaqueSurface(aLayer)
          ? gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA;
      mTarget->PushGroup(type);
    }
  }

  mLastPainted = aLayer;
}

void
BasicLayerManager::EndPaintingLayer()
{
  PRBool needsGroup = NeedsGroup(mLastPainted);
  if ((needsGroup || NeedsState(mLastPainted)) && mTarget) {
    if (needsGroup) {
      mTarget->PopGroupToSource();
      mTarget->Paint(mLastPainted->GetOpacity());
    }

    mTarget->Restore();
  }
}

void
BasicLayerManager::AdvancePaintingTo(BasicThebesLayer* aLayer)
{
  NS_ASSERTION(!aLayer || IsBeforeInTree(mLastPainted, aLayer),
               "Painting layers out of order");

  
  
  do {
    Layer* firstChild;
    Layer* nextSibling;
    
    if (!mLastPainted) {
      
      BeginPaintingLayer(mRoot);
    } else if ((firstChild = mLastPainted->GetFirstChild()) != nsnull) {
      
      BeginPaintingLayer(firstChild);
    } else if ((nextSibling = mLastPainted->GetNextSibling()) != nsnull) {
      
      
      EndPaintingLayer();
      BeginPaintingLayer(nextSibling);
    } else {
      
      
      
      do {
        EndPaintingLayer();
        mLastPainted = mLastPainted->GetParent();
      } while (mLastPainted && !mLastPainted->GetNextSibling());
      if (mLastPainted) {
        EndPaintingLayer();
        BeginPaintingLayer(mLastPainted->GetNextSibling());
      }
    }
  } while (mLastPainted != aLayer);
}

already_AddRefed<ThebesLayer>
BasicLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ThebesLayer> layer = new BasicThebesLayer(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
BasicLayerManager::CreateContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ContainerLayer> layer = new BasicContainerLayer(this);
  return layer.forget();
}

#ifdef DEBUG
static void
AppendAncestors(Layer* aLayer, nsTArray<Layer*>* aAncestors)
{
  while (aLayer) {
    aAncestors->AppendElement(aLayer);
    aLayer = aLayer->GetParent();
  }
}

PRBool
BasicLayerManager::IsBeforeInTree(Layer* aBefore, Layer* aLayer)
{
  if (!aBefore) {
    return PR_TRUE;
  }
  nsAutoTArray<Layer*,8> beforeAncestors, afterAncestors;
  AppendAncestors(aBefore, &beforeAncestors);
  AppendAncestors(aLayer, &afterAncestors);
  PRInt32 beforeIndex = beforeAncestors.Length() - 1;
  PRInt32 afterIndex = afterAncestors.Length() - 1;
  NS_ASSERTION(beforeAncestors[beforeIndex] == mRoot, "aBefore not in tree?");
  NS_ASSERTION(afterAncestors[afterIndex] == mRoot, "aLayer not in tree?");
  --beforeIndex;
  --afterIndex;
  while (beforeIndex >= 0 && afterIndex >= 0) {
    if (beforeAncestors[beforeIndex] != afterAncestors[afterIndex]) {
      BasicContainerLayer* parent =
        static_cast<BasicContainerLayer*>(beforeAncestors[beforeIndex + 1]);
      for (Layer* child = parent->GetFirstChild();
           child != afterAncestors[afterIndex];
           child = child->GetNextSibling()) {
        if (child == beforeAncestors[beforeIndex]) {
          return PR_TRUE;
        }
      }
      return PR_FALSE;
    }
    --beforeIndex;
    --afterIndex;
  }
  if (afterIndex > 0) {
    
    return PR_TRUE;
  }
  return PR_FALSE;
}
#endif

}
}
