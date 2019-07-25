





































package org.mozilla.gecko.ui;

import org.mozilla.gecko.FloatUtils;







abstract class Axis {
    
    private static final float FRICTION_SLOW = 0.85f;
    
    private static final float FRICTION_FAST = 0.97f;
    
    
    private static final float VELOCITY_THRESHOLD = 10.0f;
    
    
    private static final float MAX_EVENT_ACCELERATION = 0.012f;

    
    private static final float OVERSCROLL_DECEL_RATE = 0.04f;
    
    private static final float SNAP_LIMIT = 0.75f;

    
    
    private static final float MIN_SCROLLABLE_DISTANCE = 0.5f;
    
    private static final float MS_PER_FRAME = 1000.0f / 60.0f;

    private enum FlingStates {
        STOPPED,
        PANNING,
        FLINGING,
    }

    private enum Overscroll {
        NONE,
        MINUS,      
        PLUS,       
        BOTH,       
    }

    private final SubdocumentScrollHelper mSubscroller;

    private float mFirstTouchPos;           
    private float mTouchPos;                
    private float mLastTouchPos;            
    private float mVelocity;                
    private boolean mLocked;                
    private boolean mDisableSnap;           
    private float mDisplacement;

    private FlingStates mFlingState;        

    protected abstract float getOrigin();
    protected abstract float getViewportLength();
    protected abstract float getPageLength();

    Axis(SubdocumentScrollHelper subscroller) {
        mSubscroller = subscroller;
    }

    private float getViewportEnd() {
        return getOrigin() + getViewportLength();
    }

    void startTouch(float pos) {
        mVelocity = 0.0f;
        mLocked = false;
        mFirstTouchPos = mTouchPos = mLastTouchPos = pos;
    }

    float panDistance(float currentPos) {
        return currentPos - mFirstTouchPos;
    }

    void setLocked(boolean locked) {
        mLocked = locked;
    }

    void saveTouchPos() {
        mLastTouchPos = mTouchPos;
    }

    void updateWithTouchAt(float pos, float timeDelta) {
        float newVelocity = (mTouchPos - pos) / timeDelta * MS_PER_FRAME;

        
        
        
        boolean curVelocityIsLow = Math.abs(mVelocity) < 1.0f;
        boolean directionChange = (mVelocity > 0) != (newVelocity > 0);
        if (curVelocityIsLow || (directionChange && !FloatUtils.fuzzyEquals(newVelocity, 0.0f))) {
            mVelocity = newVelocity;
        } else {
            float maxChange = Math.abs(mVelocity * timeDelta * MAX_EVENT_ACCELERATION);
            mVelocity = Math.min(mVelocity + maxChange, Math.max(mVelocity - maxChange, newVelocity));
        }

        mTouchPos = pos;
    }

    boolean overscrolled() {
        return getOverscroll() != Overscroll.NONE;
    }

    private Overscroll getOverscroll() {
        boolean minus = (getOrigin() < 0.0f);
        boolean plus = (getViewportEnd() > getPageLength());
        if (minus && plus) {
            return Overscroll.BOTH;
        } else if (minus) {
            return Overscroll.MINUS;
        } else if (plus) {
            return Overscroll.PLUS;
        } else {
            return Overscroll.NONE;
        }
    }

    
    
    private float getExcess() {
        switch (getOverscroll()) {
        case MINUS:     return -getOrigin();
        case PLUS:      return getViewportEnd() - getPageLength();
        case BOTH:      return getViewportEnd() - getPageLength() - getOrigin();
        default:        return 0.0f;
        }
    }

    



    private boolean scrollable() {
        return getViewportLength() <= getPageLength() - MIN_SCROLLABLE_DISTANCE;
    }

    



    float getEdgeResistance() {
        float excess = getExcess();
        return (excess > 0.0f) ? SNAP_LIMIT - excess / getViewportLength() : 1.0f;
    }

    
    float getRealVelocity() {
        return mLocked ? 0.0f : mVelocity;
    }

    void startPan() {
        mFlingState = FlingStates.PANNING;
    }

    void startFling(boolean stopped) {
        mDisableSnap = mSubscroller.scrolling();

        if (stopped) {
            mFlingState = FlingStates.STOPPED;
        } else {
            mFlingState = FlingStates.FLINGING;
        }
    }

    
    boolean advanceFling() {
        if (mFlingState != FlingStates.FLINGING) {
            return false;
        }

        float excess = getExcess();
        if (mDisableSnap || FloatUtils.fuzzyEquals(excess, 0.0f)) {
            
            if (Math.abs(mVelocity) >= VELOCITY_THRESHOLD) {
                mVelocity *= FRICTION_FAST;
            } else {
                float t = mVelocity / VELOCITY_THRESHOLD;
                mVelocity *= FloatUtils.interpolate(FRICTION_SLOW, FRICTION_FAST, t);
            }
        } else {
            
            float elasticity = 1.0f - excess / (getViewportLength() * SNAP_LIMIT);
            if (getOverscroll() == Overscroll.MINUS) {
                mVelocity = Math.min((mVelocity + OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);
            } else { 
                mVelocity = Math.max((mVelocity - OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);
            }
        }

        return true;
    }

    void stopFling() {
        mVelocity = 0.0f;
        mFlingState = FlingStates.STOPPED;
    }

    
    void displace() {
        if (!mSubscroller.scrolling() && (mLocked || !scrollable()))
            return;

        if (mFlingState == FlingStates.PANNING)
            mDisplacement += (mLastTouchPos - mTouchPos) * getEdgeResistance();
        else
            mDisplacement += mVelocity;
    }

    float resetDisplacement() {
        float d = mDisplacement;
        mDisplacement = 0.0f;
        return d;
    }
}
