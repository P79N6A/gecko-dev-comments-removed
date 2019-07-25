















#define LOG_TAG "Sprites"



#include "SpriteController.h"

#include "cutils_log.h"
#include "String8.h"

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
#include <SkXfermode.h>

namespace android {



SpriteController::SpriteController(const sp<Looper>& looper, int32_t overlayLayer) :
        mLooper(looper), mOverlayLayer(overlayLayer) {
    mHandler = new WeakMessageHandler(this);

    mLocked.transactionNestingCount = 0;
    mLocked.deferredSpriteUpdate = false;
}

SpriteController::~SpriteController() {
    mLooper->removeMessages(mHandler);

#ifdef HAVE_ANDROID_OS
    if (mSurfaceComposerClient != NULL) {
        mSurfaceComposerClient->dispose();
        mSurfaceComposerClient.clear();
    }
#endif
}

sp<Sprite> SpriteController::createSprite() {
    return new SpriteImpl(this);
}

void SpriteController::openTransaction() {
    AutoMutex _l(mLock);

    mLocked.transactionNestingCount += 1;
}

void SpriteController::closeTransaction() {
    AutoMutex _l(mLock);

    LOG_ALWAYS_FATAL_IF(mLocked.transactionNestingCount == 0,
            "Sprite closeTransaction() called but there is no open sprite transaction");

    mLocked.transactionNestingCount -= 1;
    if (mLocked.transactionNestingCount == 0 && mLocked.deferredSpriteUpdate) {
        mLocked.deferredSpriteUpdate = false;
        mLooper->sendMessage(mHandler, Message(MSG_UPDATE_SPRITES));
    }
}

void SpriteController::invalidateSpriteLocked(const sp<SpriteImpl>& sprite) {
    bool wasEmpty = mLocked.invalidatedSprites.isEmpty();
    mLocked.invalidatedSprites.push(sprite);
    if (wasEmpty) {
        if (mLocked.transactionNestingCount != 0) {
            mLocked.deferredSpriteUpdate = true;
        } else {
            mLooper->sendMessage(mHandler, Message(MSG_UPDATE_SPRITES));
        }
    }
}

#ifdef HAVE_ANDROID_OS
void SpriteController::disposeSurfaceLocked(const sp<SurfaceControl>& surfaceControl) {
    bool wasEmpty = mLocked.disposedSurfaces.isEmpty();
    mLocked.disposedSurfaces.push(surfaceControl);
    if (wasEmpty) {
        mLooper->sendMessage(mHandler, Message(MSG_DISPOSE_SURFACES));
    }
}
#endif

void SpriteController::handleMessage(const Message& message) {
    switch (message.what) {
    case MSG_UPDATE_SPRITES:
        doUpdateSprites();
        break;
    case MSG_DISPOSE_SURFACES:
        doDisposeSurfaces();
        break;
    }
}

void SpriteController::doUpdateSprites() {
    
    
    
    
    
    Vector<SpriteUpdate> updates;
    size_t numSprites;
    { 
        AutoMutex _l(mLock);

        numSprites = mLocked.invalidatedSprites.size();
        for (size_t i = 0; i < numSprites; i++) {
            const sp<SpriteImpl>& sprite = mLocked.invalidatedSprites.itemAt(i);

            updates.push(SpriteUpdate(sprite, sprite->getStateLocked()));
            sprite->resetDirtyLocked();
        }
        mLocked.invalidatedSprites.clear();
    } 

    
    bool surfaceChanged = false;
    for (size_t i = 0; i < numSprites; i++) {
        SpriteUpdate& update = updates.editItemAt(i);

#ifdef HAVE_ANDROID_OS
        if (update.state.surfaceControl == NULL && update.state.wantSurfaceVisible()) {
            update.state.surfaceWidth = update.state.icon.bitmap.width();
            update.state.surfaceHeight = update.state.icon.bitmap.height();
            update.state.surfaceDrawn = false;
            update.state.surfaceVisible = false;
            update.state.surfaceControl = obtainSurface(
                    update.state.surfaceWidth, update.state.surfaceHeight);
            if (update.state.surfaceControl != NULL) {
                update.surfaceChanged = surfaceChanged = true;
            }
        }
#endif
    }

    
    bool haveGlobalTransaction = false;
    for (size_t i = 0; i < numSprites; i++) {
        SpriteUpdate& update = updates.editItemAt(i);

#ifdef HAVE_ANDROID_OS
        if (update.state.surfaceControl != NULL && update.state.wantSurfaceVisible()) {
            int32_t desiredWidth = update.state.icon.bitmap.width();
            int32_t desiredHeight = update.state.icon.bitmap.height();
            if (update.state.surfaceWidth < desiredWidth
                    || update.state.surfaceHeight < desiredHeight) {
                if (!haveGlobalTransaction) {
                    SurfaceComposerClient::openGlobalTransaction();
                    haveGlobalTransaction = true;
                }

                status_t status = update.state.surfaceControl->setSize(desiredWidth, desiredHeight);
                if (status) {
                    ALOGE("Error %d resizing sprite surface from %dx%d to %dx%d",
                            status, update.state.surfaceWidth, update.state.surfaceHeight,
                            desiredWidth, desiredHeight);
                } else {
                    update.state.surfaceWidth = desiredWidth;
                    update.state.surfaceHeight = desiredHeight;
                    update.state.surfaceDrawn = false;
                    update.surfaceChanged = surfaceChanged = true;

                    if (update.state.surfaceVisible) {
                        status = update.state.surfaceControl->hide();
                        if (status) {
                            ALOGE("Error %d hiding sprite surface after resize.", status);
                        } else {
                            update.state.surfaceVisible = false;
                        }
                    }
                }
            }
        }
#endif
    }
#ifdef HAVE_ANDROID_OS
    if (haveGlobalTransaction) {
        SurfaceComposerClient::closeGlobalTransaction();
    }
#endif

    
    for (size_t i = 0; i < numSprites; i++) {
        SpriteUpdate& update = updates.editItemAt(i);

        if ((update.state.dirty & DIRTY_BITMAP) && update.state.surfaceDrawn) {
            update.state.surfaceDrawn = false;
            update.surfaceChanged = surfaceChanged = true;
        }

#ifdef HAVE_ANDROID_OS
        if (update.state.surfaceControl != NULL && !update.state.surfaceDrawn
                && update.state.wantSurfaceVisible()) {
            sp<Surface> surface = update.state.surfaceControl->getSurface();
            Surface::SurfaceInfo surfaceInfo;
            status_t status = surface->lock(&surfaceInfo);
            if (status) {
                ALOGE("Error %d locking sprite surface before drawing.", status);
            } else {
                SkBitmap surfaceBitmap;
                ssize_t bpr = surfaceInfo.s * bytesPerPixel(surfaceInfo.format);
                surfaceBitmap.setConfig(SkBitmap::kARGB_8888_Config,
                        surfaceInfo.w, surfaceInfo.h, bpr);
                surfaceBitmap.setPixels(surfaceInfo.bits);

                SkCanvas surfaceCanvas;
                surfaceCanvas.setBitmapDevice(surfaceBitmap);

                SkPaint paint;
                paint.setXfermodeMode(SkXfermode::kSrc_Mode);
                surfaceCanvas.drawBitmap(update.state.icon.bitmap, 0, 0, &paint);

                if (surfaceInfo.w > uint32_t(update.state.icon.bitmap.width())) {
                    paint.setColor(0); 
                    surfaceCanvas.drawRectCoords(update.state.icon.bitmap.width(), 0,
                            surfaceInfo.w, update.state.icon.bitmap.height(), paint);
                }
                if (surfaceInfo.h > uint32_t(update.state.icon.bitmap.height())) {
                    paint.setColor(0); 
                    surfaceCanvas.drawRectCoords(0, update.state.icon.bitmap.height(),
                            surfaceInfo.w, surfaceInfo.h, paint);
                }

                status = surface->unlockAndPost();
                if (status) {
                    ALOGE("Error %d unlocking and posting sprite surface after drawing.", status);
                } else {
                    update.state.surfaceDrawn = true;
                    update.surfaceChanged = surfaceChanged = true;
                }
            }
        }
#endif
    }

    
    bool haveTransaction = false;
    for (size_t i = 0; i < numSprites; i++) {
        SpriteUpdate& update = updates.editItemAt(i);

        bool wantSurfaceVisibleAndDrawn = update.state.wantSurfaceVisible()
                && update.state.surfaceDrawn;
        bool becomingVisible = wantSurfaceVisibleAndDrawn && !update.state.surfaceVisible;
        bool becomingHidden = !wantSurfaceVisibleAndDrawn && update.state.surfaceVisible;
#ifdef HAVE_ANDROID_OS
        if (update.state.surfaceControl != NULL && (becomingVisible || becomingHidden
                || (wantSurfaceVisibleAndDrawn && (update.state.dirty & (DIRTY_ALPHA
                        | DIRTY_POSITION | DIRTY_TRANSFORMATION_MATRIX | DIRTY_LAYER
                        | DIRTY_VISIBILITY | DIRTY_HOTSPOT))))) {
            status_t status;
            if (!haveTransaction) {
                SurfaceComposerClient::openGlobalTransaction();
                haveTransaction = true;
            }

            if (wantSurfaceVisibleAndDrawn
                    && (becomingVisible || (update.state.dirty & DIRTY_ALPHA))) {
                status = update.state.surfaceControl->setAlpha(update.state.alpha);
                if (status) {
                    ALOGE("Error %d setting sprite surface alpha.", status);
                }
            }

            if (wantSurfaceVisibleAndDrawn
                    && (becomingVisible || (update.state.dirty & (DIRTY_POSITION
                            | DIRTY_HOTSPOT)))) {
                status = update.state.surfaceControl->setPosition(
                        update.state.positionX - update.state.icon.hotSpotX,
                        update.state.positionY - update.state.icon.hotSpotY);
                if (status) {
                    ALOGE("Error %d setting sprite surface position.", status);
                }
            }

            if (wantSurfaceVisibleAndDrawn
                    && (becomingVisible
                            || (update.state.dirty & DIRTY_TRANSFORMATION_MATRIX))) {
                status = update.state.surfaceControl->setMatrix(
                        update.state.transformationMatrix.dsdx,
                        update.state.transformationMatrix.dtdx,
                        update.state.transformationMatrix.dsdy,
                        update.state.transformationMatrix.dtdy);
                if (status) {
                    ALOGE("Error %d setting sprite surface transformation matrix.", status);
                }
            }

            int32_t surfaceLayer = mOverlayLayer + update.state.layer;
            if (wantSurfaceVisibleAndDrawn
                    && (becomingVisible || (update.state.dirty & DIRTY_LAYER))) {
                status = update.state.surfaceControl->setLayer(surfaceLayer);
                if (status) {
                    ALOGE("Error %d setting sprite surface layer.", status);
                }
            }

            if (becomingVisible) {
                status = update.state.surfaceControl->show(surfaceLayer);
                if (status) {
                    ALOGE("Error %d showing sprite surface.", status);
                } else {
                    update.state.surfaceVisible = true;
                    update.surfaceChanged = surfaceChanged = true;
                }
            } else if (becomingHidden) {
                status = update.state.surfaceControl->hide();
                if (status) {
                    ALOGE("Error %d hiding sprite surface.", status);
                } else {
                    update.state.surfaceVisible = false;
                    update.surfaceChanged = surfaceChanged = true;
                }
            }
        }
#endif
    }

#ifdef HAVE_ANDROID_OS
    if (haveTransaction) {
        SurfaceComposerClient::closeGlobalTransaction();
    }
#endif

    
    if (surfaceChanged) { 
        AutoMutex _l(mLock);

        for (size_t i = 0; i < numSprites; i++) {
            const SpriteUpdate& update = updates.itemAt(i);

#ifdef HAVE_ANDROID_OS
            if (update.surfaceChanged) {
                update.sprite->setSurfaceLocked(update.state.surfaceControl,
                        update.state.surfaceWidth, update.state.surfaceHeight,
                        update.state.surfaceDrawn, update.state.surfaceVisible);
            }
#endif
        }
    } 

    
    
    
    
    
    updates.clear();
}

void SpriteController::doDisposeSurfaces() {
#ifdef HAVE_ANDROID_OS
    
    Vector<sp<SurfaceControl> > disposedSurfaces;
    { 
        AutoMutex _l(mLock);

        disposedSurfaces = mLocked.disposedSurfaces;
        mLocked.disposedSurfaces.clear();
    } 

    
    
    disposedSurfaces.clear();
#endif
}

void SpriteController::ensureSurfaceComposerClient() {
#ifdef HAVE_ANDROID_OS
    if (mSurfaceComposerClient == NULL) {
        mSurfaceComposerClient = new SurfaceComposerClient();
    }
#endif
}

#ifdef HAVE_ANDROID_OS
sp<SurfaceControl> SpriteController::obtainSurface(int32_t width, int32_t height) {
    ensureSurfaceComposerClient();

    sp<SurfaceControl> surfaceControl = mSurfaceComposerClient->createSurface(
            String8("Sprite"), 0, width, height, PIXEL_FORMAT_RGBA_8888);
    if (surfaceControl == NULL || !surfaceControl->isValid()
            || !surfaceControl->getSurface()->isValid()) {
        ALOGE("Error creating sprite surface.");
        return NULL;
    }
    return surfaceControl;
}
#endif




SpriteController::SpriteImpl::SpriteImpl(const sp<SpriteController> controller) :
        mController(controller) {
}

SpriteController::SpriteImpl::~SpriteImpl() {
    AutoMutex _m(mController->mLock);

#ifdef HAVE_ANDROID_OS
    
    
    if (mLocked.state.surfaceControl != NULL) {
        mController->disposeSurfaceLocked(mLocked.state.surfaceControl);
        mLocked.state.surfaceControl.clear();
    }
#endif
}

void SpriteController::SpriteImpl::setIcon(const SpriteIcon& icon) {
    AutoMutex _l(mController->mLock);

#ifdef HAVE_ANDROID_OS
    uint32_t dirty;
    if (icon.isValid()) {
        icon.bitmap.copyTo(&mLocked.state.icon.bitmap, SkBitmap::kARGB_8888_Config);

        if (!mLocked.state.icon.isValid()
                || mLocked.state.icon.hotSpotX != icon.hotSpotX
                || mLocked.state.icon.hotSpotY != icon.hotSpotY) {
            mLocked.state.icon.hotSpotX = icon.hotSpotX;
            mLocked.state.icon.hotSpotY = icon.hotSpotY;
            dirty = DIRTY_BITMAP | DIRTY_HOTSPOT;
        } else {
            dirty = DIRTY_BITMAP;
        }
    } else if (mLocked.state.icon.isValid()) {
        mLocked.state.icon.bitmap.reset();
        dirty = DIRTY_BITMAP | DIRTY_HOTSPOT;
    } else {
        return; 
    }

    invalidateLocked(dirty);
#endif
}

void SpriteController::SpriteImpl::setVisible(bool visible) {
    AutoMutex _l(mController->mLock);

    if (mLocked.state.visible != visible) {
        mLocked.state.visible = visible;
        invalidateLocked(DIRTY_VISIBILITY);
    }
}

void SpriteController::SpriteImpl::setPosition(float x, float y) {
    AutoMutex _l(mController->mLock);

    if (mLocked.state.positionX != x || mLocked.state.positionY != y) {
        mLocked.state.positionX = x;
        mLocked.state.positionY = y;
        invalidateLocked(DIRTY_POSITION);
    }
}

void SpriteController::SpriteImpl::setLayer(int32_t layer) {
    AutoMutex _l(mController->mLock);

    if (mLocked.state.layer != layer) {
        mLocked.state.layer = layer;
        invalidateLocked(DIRTY_LAYER);
    }
}

void SpriteController::SpriteImpl::setAlpha(float alpha) {
    AutoMutex _l(mController->mLock);

    if (mLocked.state.alpha != alpha) {
        mLocked.state.alpha = alpha;
        invalidateLocked(DIRTY_ALPHA);
    }
}

void SpriteController::SpriteImpl::setTransformationMatrix(
        const SpriteTransformationMatrix& matrix) {
    AutoMutex _l(mController->mLock);

    if (mLocked.state.transformationMatrix != matrix) {
        mLocked.state.transformationMatrix = matrix;
        invalidateLocked(DIRTY_TRANSFORMATION_MATRIX);
    }
}

void SpriteController::SpriteImpl::invalidateLocked(uint32_t dirty) {
    bool wasDirty = mLocked.state.dirty;
    mLocked.state.dirty |= dirty;

    if (!wasDirty) {
        mController->invalidateSpriteLocked(this);
    }
}

} 
