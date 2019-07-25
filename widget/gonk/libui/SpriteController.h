















#ifndef _UI_SPRITES_H
#define _UI_SPRITES_H

#include <utils/RefBase.h>
#include <utils/Looper.h>

#ifdef HAVE_ANDROID_OS
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <surfaceflinger/ISurfaceComposer.h>

#include <SkBitmap.h>
#endif

namespace android {




struct SpriteTransformationMatrix {
    inline SpriteTransformationMatrix() : dsdx(1.0f), dtdx(0.0f), dsdy(0.0f), dtdy(1.0f) { }
    inline SpriteTransformationMatrix(float dsdx, float dtdx, float dsdy, float dtdy) :
            dsdx(dsdx), dtdx(dtdx), dsdy(dsdy), dtdy(dtdy) { }

    float dsdx;
    float dtdx;
    float dsdy;
    float dtdy;

    inline bool operator== (const SpriteTransformationMatrix& other) {
        return dsdx == other.dsdx
                && dtdx == other.dtdx
                && dsdy == other.dsdy
                && dtdy == other.dtdy;
    }

    inline bool operator!= (const SpriteTransformationMatrix& other) {
        return !(*this == other);
    }
};




struct SpriteIcon {
    inline SpriteIcon() : hotSpotX(0), hotSpotY(0) { }
#ifdef HAVE_ANDROID_OS
    inline SpriteIcon(const SkBitmap& bitmap, float hotSpotX, float hotSpotY) :
            bitmap(bitmap), hotSpotX(hotSpotX), hotSpotY(hotSpotY) { }

    SkBitmap bitmap;
#endif
    float hotSpotX;
    float hotSpotY;

    inline SpriteIcon copy() const {
#ifdef HAVE_ANDROID_OS
        SkBitmap bitmapCopy;
        bitmap.copyTo(&bitmapCopy, SkBitmap::kARGB_8888_Config);
        return SpriteIcon(bitmapCopy, hotSpotX, hotSpotY);
#else
	return SpriteIcon();
#endif
    }

    inline void reset() {
#ifdef HAVE_ANDROID_OS
        bitmap.reset();
        hotSpotX = 0;
        hotSpotY = 0;
#endif
    }

    inline bool isValid() const {
#ifdef HAVE_ANDROID_OS
        return !bitmap.isNull() && !bitmap.empty();
#else
	return false;
#endif
    }
};






class Sprite : public RefBase {
protected:
    Sprite() { }
    virtual ~Sprite() { }

public:
    enum {
        
        BASE_LAYER_POINTER = 0, 

        
        BASE_LAYER_SPOT = 1, 
    };

    

    virtual void setIcon(const SpriteIcon& icon) = 0;

    inline void clearIcon() {
        setIcon(SpriteIcon());
    }

    
    virtual void setVisible(bool visible) = 0;

    
    virtual void setPosition(float x, float y) = 0;

    

    virtual void setLayer(int32_t layer) = 0;

    
    virtual void setAlpha(float alpha) = 0;

    
    virtual void setTransformationMatrix(const SpriteTransformationMatrix& matrix) = 0;
};












#ifdef HAVE_ANDROID_OS
class SpriteController : public MessageHandler {
#else
class SpriteController : public virtual RefBase {
#endif
protected:
    virtual ~SpriteController();

public:
    SpriteController(const sp<Looper>& looper, int32_t overlayLayer);

    
    sp<Sprite> createSprite();

    




    void openTransaction();
    void closeTransaction();

private:
    enum {
        MSG_UPDATE_SPRITES,
        MSG_DISPOSE_SURFACES,
    };

    enum {
        DIRTY_BITMAP = 1 << 0,
        DIRTY_ALPHA = 1 << 1,
        DIRTY_POSITION = 1 << 2,
        DIRTY_TRANSFORMATION_MATRIX = 1 << 3,
        DIRTY_LAYER = 1 << 4,
        DIRTY_VISIBILITY = 1 << 5,
        DIRTY_HOTSPOT = 1 << 6,
    };

    




    struct SpriteState {
        inline SpriteState() :
                dirty(0), visible(false),
                positionX(0), positionY(0), layer(0), alpha(1.0f),
                surfaceWidth(0), surfaceHeight(0), surfaceDrawn(false), surfaceVisible(false) {
        }

        uint32_t dirty;

        SpriteIcon icon;
        bool visible;
        float positionX;
        float positionY;
        int32_t layer;
        float alpha;
        SpriteTransformationMatrix transformationMatrix;

#ifdef HAVE_ANDROID_OS
        sp<SurfaceControl> surfaceControl;
#endif
        int32_t surfaceWidth;
        int32_t surfaceHeight;
        bool surfaceDrawn;
        bool surfaceVisible;

        inline bool wantSurfaceVisible() const {
            return visible && alpha > 0.0f && icon.isValid();
        }
    };

    






    class SpriteImpl : public Sprite {
    protected:
        virtual ~SpriteImpl();

    public:
        SpriteImpl(const sp<SpriteController> controller);

        virtual void setIcon(const SpriteIcon& icon);
        virtual void setVisible(bool visible);
        virtual void setPosition(float x, float y);
        virtual void setLayer(int32_t layer);
        virtual void setAlpha(float alpha);
        virtual void setTransformationMatrix(const SpriteTransformationMatrix& matrix);

        inline const SpriteState& getStateLocked() const {
            return mLocked.state;
        }

        inline void resetDirtyLocked() {
            mLocked.state.dirty = 0;
        }

#ifdef HAVE_ANDROID_OS
        inline void setSurfaceLocked(const sp<SurfaceControl>& surfaceControl,
                int32_t width, int32_t height, bool drawn, bool visible) {
            mLocked.state.surfaceControl = surfaceControl;
            mLocked.state.surfaceWidth = width;
            mLocked.state.surfaceHeight = height;
            mLocked.state.surfaceDrawn = drawn;
            mLocked.state.surfaceVisible = visible;
        }
#endif

    private:
        sp<SpriteController> mController;

        struct Locked {
            SpriteState state;
        } mLocked; 

        void invalidateLocked(uint32_t dirty);
    };

    
    struct SpriteUpdate {
        inline SpriteUpdate() : surfaceChanged(false) { }
        inline SpriteUpdate(const sp<SpriteImpl> sprite, const SpriteState& state) :
                sprite(sprite), state(state), surfaceChanged(false) {
        }

        sp<SpriteImpl> sprite;
        SpriteState state;
        bool surfaceChanged;
    };

    mutable Mutex mLock;

    sp<Looper> mLooper;
    const int32_t mOverlayLayer;
#ifdef HAVE_ANDROID_OS
    sp<WeakMessageHandler> mHandler;

    sp<SurfaceComposerClient> mSurfaceComposerClient;
#endif

    struct Locked {
        Vector<sp<SpriteImpl> > invalidatedSprites;
#ifdef HAVE_ANDROID_OS
        Vector<sp<SurfaceControl> > disposedSurfaces;
#endif
        uint32_t transactionNestingCount;
        bool deferredSpriteUpdate;
    } mLocked; 

    void invalidateSpriteLocked(const sp<SpriteImpl>& sprite);
#ifdef HAVE_ANDROID_OS
    void disposeSurfaceLocked(const sp<SurfaceControl>& surfaceControl);

    void handleMessage(const Message& message);
#endif
    void doUpdateSprites();
    void doDisposeSurfaces();

    void ensureSurfaceComposerClient();
#ifdef HAVE_ANDROID_OS
    sp<SurfaceControl> obtainSurface(int32_t width, int32_t height);
#endif
};

} 

#endif 
