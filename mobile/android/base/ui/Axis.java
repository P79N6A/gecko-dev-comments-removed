





































package org.mozilla.gecko.ui;

import java.util.Map;
import android.util.Log;
import org.json.JSONArray;
import org.mozilla.gecko.FloatUtils;







abstract class Axis {
    private static final String LOGTAG = "GeckoAxis";

    private static final String PREF_SCROLLING_FRICTION_SLOW = "ui.scrolling.friction_slow";
    private static final String PREF_SCROLLING_FRICTION_FAST = "ui.scrolling.friction_fast";
    private static final String PREF_SCROLLING_VELOCITY_THRESHOLD = "ui.scrolling.velocity_threshold";
    private static final String PREF_SCROLLING_MAX_EVENT_ACCELERATION = "ui.scrolling.max_event_acceleration";
    private static final String PREF_SCROLLING_OVERSCROLL_DECEL_RATE = "ui.scrolling.overscroll_decel_rate";
    private static final String PREF_SCROLLING_OVERSCROLL_SNAP_LIMIT = "ui.scrolling.overscroll_snap_limit";
    private static final String PREF_SCROLLING_MIN_SCROLLABLE_DISTANCE = "ui.scrolling.min_scrollable_distance";

    
    private static float FRICTION_SLOW;
    
    private static float FRICTION_FAST;
    
    
    private static float VELOCITY_THRESHOLD;
    
    
    private static float MAX_EVENT_ACCELERATION;

    
    private static float OVERSCROLL_DECEL_RATE;
    
    private static float SNAP_LIMIT;

    
    
    private static float MIN_SCROLLABLE_DISTANCE;

    private static float getFloatPref(Map<String, Integer> prefs, String prefName, int defaultValue) {
        Integer value = (prefs == null ? null : prefs.get(prefName));
        return (float)(value == null || value < 0 ? defaultValue : value) / 1000f;
    }

    private static int getIntPref(Map<String, Integer> prefs, String prefName, int defaultValue) {
        Integer value = (prefs == null ? null : prefs.get(prefName));
        return (value == null || value < 0 ? defaultValue : value);
    }

    static void addPrefNames(JSONArray prefs) {
        prefs.put(PREF_SCROLLING_FRICTION_FAST);
        prefs.put(PREF_SCROLLING_FRICTION_SLOW);
        prefs.put(PREF_SCROLLING_VELOCITY_THRESHOLD);
        prefs.put(PREF_SCROLLING_MAX_EVENT_ACCELERATION);
        prefs.put(PREF_SCROLLING_OVERSCROLL_DECEL_RATE);
        prefs.put(PREF_SCROLLING_OVERSCROLL_SNAP_LIMIT);
        prefs.put(PREF_SCROLLING_MIN_SCROLLABLE_DISTANCE);
    }

    static void setPrefs(Map<String, Integer> prefs) {
        FRICTION_SLOW = getFloatPref(prefs, PREF_SCROLLING_FRICTION_SLOW, 850);
        FRICTION_FAST = getFloatPref(prefs, PREF_SCROLLING_FRICTION_FAST, 970);
        VELOCITY_THRESHOLD = getIntPref(prefs, PREF_SCROLLING_VELOCITY_THRESHOLD, 10);
        MAX_EVENT_ACCELERATION = getFloatPref(prefs, PREF_SCROLLING_MAX_EVENT_ACCELERATION, 12);
        OVERSCROLL_DECEL_RATE = getFloatPref(prefs, PREF_SCROLLING_OVERSCROLL_DECEL_RATE, 40);
        SNAP_LIMIT = getFloatPref(prefs, PREF_SCROLLING_OVERSCROLL_SNAP_LIMIT, 300);
        MIN_SCROLLABLE_DISTANCE = getFloatPref(prefs, PREF_SCROLLING_MIN_SCROLLABLE_DISTANCE, 500);
        Log.i(LOGTAG, "Prefs: " + FRICTION_SLOW + "," + FRICTION_FAST + "," + VELOCITY_THRESHOLD + ","
                + MAX_EVENT_ACCELERATION + "," + OVERSCROLL_DECEL_RATE + "," + SNAP_LIMIT + "," + MIN_SCROLLABLE_DISTANCE);
    }

    static {
        
        setPrefs(null);
    }

    
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
    public boolean mScrollingDisabled;      
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
        mScrollingDisabled = false;
        mFirstTouchPos = mTouchPos = mLastTouchPos = pos;
    }

    float panDistance(float currentPos) {
        return currentPos - mFirstTouchPos;
    }

    void setScrollingDisabled(boolean disabled) {
        mScrollingDisabled = disabled;
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
        return getViewportLength() <= getPageLength() - MIN_SCROLLABLE_DISTANCE &&
               !mScrollingDisabled;
    }

    



    float getEdgeResistance() {
        float excess = getExcess();
        if (excess > 0.0f) {
            
            
            return Math.max(0.0f, SNAP_LIMIT - excess / getViewportLength());
        }
        return 1.0f;
    }

    
    float getRealVelocity() {
        return scrollable() ? mVelocity : 0f;
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
        if (mSubscroller.scrolling() && !mSubscroller.lastScrollSucceeded()) {
            
            
            
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
        if (!mSubscroller.scrolling() && !scrollable())
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
