















#ifndef _UI_POINTER_CONTROLLER_H
#define _UI_POINTER_CONTROLLER_H

#include "SpriteController.h"

#include "DisplayInfo.h"
#include "Input.h"
#include <utils/RefBase.h>
#include <utils/Looper.h>
#include <utils/String8.h>

#include <SkBitmap.h>

namespace android {










class PointerControllerInterface : public virtual RefBase {
protected:
    PointerControllerInterface() { }
    virtual ~PointerControllerInterface() { }

public:
    

    virtual bool getBounds(float* outMinX, float* outMinY,
            float* outMaxX, float* outMaxY) const = 0;

    
    virtual void move(float deltaX, float deltaY) = 0;

    
    virtual void setButtonState(int32_t buttonState) = 0;

    
    virtual int32_t getButtonState() const = 0;

    
    virtual void setPosition(float x, float y) = 0;

    
    virtual void getPosition(float* outX, float* outY) const = 0;

    enum Transition {
        
        TRANSITION_IMMEDIATE,
        
        TRANSITION_GRADUAL,
    };

    
    virtual void fade(Transition transition) = 0;

    



    virtual void unfade(Transition transition) = 0;

    enum Presentation {
        
        PRESENTATION_POINTER,
        
        PRESENTATION_SPOT,
    };

    
    virtual void setPresentation(Presentation presentation) = 0;

    








    virtual void setSpots(const PointerCoords* spotCoords, const uint32_t* spotIdToIndex,
            BitSet32 spotIdBits) = 0;

    
    virtual void clearSpots() = 0;
};





struct PointerResources {
    SpriteIcon spotHover;
    SpriteIcon spotTouch;
    SpriteIcon spotAnchor;
};











class PointerControllerPolicyInterface : public virtual RefBase {
protected:
    PointerControllerPolicyInterface() { }
    virtual ~PointerControllerPolicyInterface() { }

public:
    virtual void loadPointerResources(PointerResources* outResources) = 0;
};







class PointerController : public PointerControllerInterface, public MessageHandler {
protected:
    virtual ~PointerController();

public:
    enum InactivityTimeout {
        INACTIVITY_TIMEOUT_NORMAL = 0,
        INACTIVITY_TIMEOUT_SHORT = 1,
    };

    PointerController(const sp<PointerControllerPolicyInterface>& policy,
            const sp<Looper>& looper, const sp<SpriteController>& spriteController);

    virtual bool getBounds(float* outMinX, float* outMinY,
            float* outMaxX, float* outMaxY) const;
    virtual void move(float deltaX, float deltaY);
    virtual void setButtonState(int32_t buttonState);
    virtual int32_t getButtonState() const;
    virtual void setPosition(float x, float y);
    virtual void getPosition(float* outX, float* outY) const;
    virtual void fade(Transition transition);
    virtual void unfade(Transition transition);

    virtual void setPresentation(Presentation presentation);
    virtual void setSpots(const PointerCoords* spotCoords,
            const uint32_t* spotIdToIndex, BitSet32 spotIdBits);
    virtual void clearSpots();

    void setDisplaySize(int32_t width, int32_t height);
    void setDisplayOrientation(int32_t orientation);
    void setPointerIcon(const SpriteIcon& icon);
    void setInactivityTimeout(InactivityTimeout inactivityTimeout);

private:
    static const size_t MAX_RECYCLED_SPRITES = 12;
    static const size_t MAX_SPOTS = 12;

    enum {
        MSG_ANIMATE,
        MSG_INACTIVITY_TIMEOUT,
    };

    struct Spot {
        static const uint32_t INVALID_ID = 0xffffffff;

        uint32_t id;
        sp<Sprite> sprite;
        float alpha;
        float scale;
        float x, y;

        inline Spot(uint32_t id, const sp<Sprite>& sprite)
                : id(id), sprite(sprite), alpha(1.0f), scale(1.0f),
                  x(0.0f), y(0.0f), lastIcon(NULL) { }

        void updateSprite(const SpriteIcon* icon, float x, float y);

    private:
        const SpriteIcon* lastIcon;
    };

    mutable Mutex mLock;

    sp<PointerControllerPolicyInterface> mPolicy;
    sp<Looper> mLooper;
    sp<SpriteController> mSpriteController;
    sp<WeakMessageHandler> mHandler;

    PointerResources mResources;

    struct Locked {
        bool animationPending;
        nsecs_t animationTime;

        int32_t displayWidth;
        int32_t displayHeight;
        int32_t displayOrientation;

        InactivityTimeout inactivityTimeout;

        Presentation presentation;
        bool presentationChanged;

        int32_t pointerFadeDirection;
        float pointerX;
        float pointerY;
        float pointerAlpha;
        sp<Sprite> pointerSprite;
        SpriteIcon pointerIcon;
        bool pointerIconChanged;

        int32_t buttonState;

        Vector<Spot*> spots;
        Vector<sp<Sprite> > recycledSprites;
    } mLocked;

    bool getBoundsLocked(float* outMinX, float* outMinY, float* outMaxX, float* outMaxY) const;
    void setPositionLocked(float x, float y);

    void handleMessage(const Message& message);
    void doAnimate();
    void doInactivityTimeout();

    void startAnimationLocked();

    void resetInactivityTimeoutLocked();
    void removeInactivityTimeoutLocked();
    void updatePointerLocked();

    Spot* getSpotLocked(uint32_t id);
    Spot* createAndAddSpotLocked(uint32_t id);
    Spot* removeFirstFadingSpotLocked();
    void releaseSpotLocked(Spot* spot);
    void fadeOutAndReleaseSpotLocked(Spot* spot);
    void fadeOutAndReleaseAllSpotsLocked();

    void loadResources();
};

} 

#endif
