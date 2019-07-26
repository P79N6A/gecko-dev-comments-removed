




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.util.FloatUtils;

import org.json.JSONArray;

import android.util.Log;
import android.view.View;

import java.util.HashMap;
import java.util.Map;







abstract class Axis {
    private static final String LOGTAG = "GeckoAxis";

    private static final String PREF_SCROLLING_FRICTION_SLOW = "ui.scrolling.friction_slow";
    private static final String PREF_SCROLLING_FRICTION_FAST = "ui.scrolling.friction_fast";
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

    static void initPrefs() {
        JSONArray prefs = new JSONArray();
        prefs.put(PREF_SCROLLING_FRICTION_FAST);
        prefs.put(PREF_SCROLLING_FRICTION_SLOW);
        prefs.put(PREF_SCROLLING_MAX_EVENT_ACCELERATION);
        prefs.put(PREF_SCROLLING_OVERSCROLL_DECEL_RATE);
        prefs.put(PREF_SCROLLING_OVERSCROLL_SNAP_LIMIT);
        prefs.put(PREF_SCROLLING_MIN_SCROLLABLE_DISTANCE);

        PrefsHelper.getPrefs(prefs, new PrefsHelper.PrefHandlerBase() {
            Map<String, Integer> mPrefs = new HashMap<String, Integer>();

            @Override public void prefValue(String name, int value) {
                mPrefs.put(name, value);
            }

            @Override public void finish() {
                setPrefs(mPrefs);
            }
        });
    }

    static final float MS_PER_FRAME = 1000.0f / 60.0f;
    private static final float FRAMERATE_MULTIPLIER = (1000f/60f) / MS_PER_FRAME;
    private static final int FLING_VELOCITY_POINTS = 8;

    
    
    
    static float getFrameAdjustedFriction(float baseFriction) {
        return (float)Math.pow(Math.E, (Math.log(baseFriction) / FRAMERATE_MULTIPLIER));
    }

    static void setPrefs(Map<String, Integer> prefs) {
        FRICTION_SLOW = getFrameAdjustedFriction(getFloatPref(prefs, PREF_SCROLLING_FRICTION_SLOW, 850));
        FRICTION_FAST = getFrameAdjustedFriction(getFloatPref(prefs, PREF_SCROLLING_FRICTION_FAST, 970));
        VELOCITY_THRESHOLD = 10 / FRAMERATE_MULTIPLIER;
        MAX_EVENT_ACCELERATION = getFloatPref(prefs, PREF_SCROLLING_MAX_EVENT_ACCELERATION, 12);
        OVERSCROLL_DECEL_RATE = getFrameAdjustedFriction(getFloatPref(prefs, PREF_SCROLLING_OVERSCROLL_DECEL_RATE, 40));
        SNAP_LIMIT = getFloatPref(prefs, PREF_SCROLLING_OVERSCROLL_SNAP_LIMIT, 300);
        MIN_SCROLLABLE_DISTANCE = getFloatPref(prefs, PREF_SCROLLING_MIN_SCROLLABLE_DISTANCE, 500);
        Log.i(LOGTAG, "Prefs: " + FRICTION_SLOW + "," + FRICTION_FAST + "," + VELOCITY_THRESHOLD + ","
                + MAX_EVENT_ACCELERATION + "," + OVERSCROLL_DECEL_RATE + "," + SNAP_LIMIT + "," + MIN_SCROLLABLE_DISTANCE);
    }

    static {
        
        setPrefs(null);
    }

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

    private int mOverscrollMode; 
    private float mFirstTouchPos;           
    private float mTouchPos;                
    private float mLastTouchPos;            
    private float mVelocity;                
    private float[] mRecentVelocities;      
    private int mRecentVelocityCount;       
    private boolean mScrollingDisabled;     
    private boolean mDisableSnap;           
    private float mDisplacement;

    private FlingStates mFlingState = FlingStates.STOPPED; 

    protected abstract float getOrigin();
    protected abstract float getViewportLength();
    protected abstract float getPageStart();
    protected abstract float getPageLength();

    Axis(SubdocumentScrollHelper subscroller) {
        mSubscroller = subscroller;
        mOverscrollMode = View.OVER_SCROLL_IF_CONTENT_SCROLLS;
        mRecentVelocities = new float[FLING_VELOCITY_POINTS];
    }

    public void setOverScrollMode(int overscrollMode) {
        mOverscrollMode = overscrollMode;
    }

    public int getOverScrollMode() {
        return mOverscrollMode;
    }

    private float getViewportEnd() {
        return getOrigin() + getViewportLength();
    }

    private float getPageEnd() {
        return getPageStart() + getPageLength();
    }

    void startTouch(float pos) {
        mVelocity = 0.0f;
        mScrollingDisabled = false;
        mFirstTouchPos = mTouchPos = mLastTouchPos = pos;
        mRecentVelocityCount = 0;
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

        mRecentVelocities[mRecentVelocityCount % FLING_VELOCITY_POINTS] = newVelocity;
        mRecentVelocityCount++;

        
        
        
        boolean curVelocityIsLow = Math.abs(mVelocity) < 1.0f / FRAMERATE_MULTIPLIER;
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
        boolean minus = (getOrigin() < getPageStart());
        boolean plus = (getViewportEnd() > getPageEnd());
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
        case MINUS:     return getPageStart() - getOrigin();
        case PLUS:      return getViewportEnd() - getPageEnd();
        case BOTH:      return (getViewportEnd() - getPageEnd()) + (getPageStart() - getOrigin());
        default:        return 0.0f;
        }
    }

    



    boolean scrollable() {
        
        
        if (mSubscroller.scrolling()) {
            return !mScrollingDisabled;
        }

        
        if (mScrollingDisabled) {
            return false;
        }

        
        
        return getViewportLength() <= getPageLength() - MIN_SCROLLABLE_DISTANCE ||
               getOverScrollMode() == View.OVER_SCROLL_ALWAYS;
    }

    



    float getEdgeResistance(boolean forPinching) {
        float excess = getExcess();
        if (excess > 0.0f && (getOverscroll() == Overscroll.BOTH || !forPinching)) {
            
            
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

    private float calculateFlingVelocity() {
        int usablePoints = Math.min(mRecentVelocityCount, FLING_VELOCITY_POINTS);
        if (usablePoints <= 1) {
            return mVelocity;
        }
        float average = 0;
        for (int i = 0; i < usablePoints; i++) {
            average += mRecentVelocities[i];
        }
        return average / usablePoints;
    }

    void startFling(boolean stopped) {
        mDisableSnap = mSubscroller.scrolling();

        if (stopped) {
            mFlingState = FlingStates.STOPPED;
        } else {
            mVelocity = calculateFlingVelocity();
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
        Overscroll overscroll = getOverscroll();
        boolean decreasingOverscroll = false;
        if ((overscroll == Overscroll.MINUS && mVelocity > 0) ||
            (overscroll == Overscroll.PLUS && mVelocity < 0))
        {
            decreasingOverscroll = true;
        }

        if (mDisableSnap || FloatUtils.fuzzyEquals(excess, 0.0f) || decreasingOverscroll) {
            
            if (Math.abs(mVelocity) >= VELOCITY_THRESHOLD) {
                mVelocity *= FRICTION_FAST;
            } else {
                float t = mVelocity / VELOCITY_THRESHOLD;
                mVelocity *= FloatUtils.interpolate(FRICTION_SLOW, FRICTION_FAST, t);
            }
        } else {
            
            float elasticity = 1.0f - excess / (getViewportLength() * SNAP_LIMIT);
            if (overscroll == Overscroll.MINUS) {
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
        
        if (!scrollable())
            return;

        if (mFlingState == FlingStates.PANNING)
            mDisplacement += (mLastTouchPos - mTouchPos) * getEdgeResistance(false);
        else
            mDisplacement += mVelocity * getEdgeResistance(false);

        
        
        
        
        if (getOverScrollMode() == View.OVER_SCROLL_NEVER && !mSubscroller.scrolling()) {
            if (mDisplacement + getOrigin() < getPageStart()) {
                mDisplacement = getPageStart() - getOrigin();
                stopFling();
            } else if (mDisplacement + getViewportEnd() > getPageEnd()) {
                mDisplacement = getPageEnd() - getViewportEnd();
                stopFling();
            }
        }
    }

    float resetDisplacement() {
        float d = mDisplacement;
        mDisplacement = 0.0f;
        return d;
    }

    void setAutoscrollVelocity(float velocity) {
        if (mFlingState != FlingStates.STOPPED) {
            Log.e(LOGTAG, "Setting autoscroll velocity while in a fling is not allowed!");
            return;
        }
        mVelocity = velocity;
    }
}
