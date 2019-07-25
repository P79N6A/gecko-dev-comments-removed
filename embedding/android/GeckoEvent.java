




































package org.mozilla.gecko;

import android.os.*;
import android.app.*;
import android.view.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;
import android.location.*;
import android.util.FloatMath;
import android.util.DisplayMetrics;

import android.util.Log;






public class GeckoEvent {
    public static final int INVALID = -1;
    public static final int NATIVE_POKE = 0;
    public static final int KEY_EVENT = 1;
    public static final int MOTION_EVENT = 2;
    public static final int ORIENTATION_EVENT = 3;
    public static final int ACCELERATION_EVENT = 4;
    public static final int LOCATION_EVENT = 5;
    public static final int IME_EVENT = 6;
    public static final int DRAW = 7;
    public static final int SIZE_CHANGED = 8;
    public static final int ACTIVITY_STOPPING = 9;
    public static final int ACTIVITY_PAUSING = 10;
    public static final int ACTIVITY_SHUTDOWN = 11;
    public static final int LOAD_URI = 12;
    public static final int SURFACE_CREATED = 13;
    public static final int SURFACE_DESTROYED = 14;
    public static final int GECKO_EVENT_SYNC = 15;
    public static final int ACTIVITY_START = 17;
    public static final int SAVE_STATE = 18;
    public static final int BROADCAST = 19;
    public static final int VIEWPORT = 20;
    public static final int VISITED = 21;
    public static final int NETWORK_CHANGED = 22;

    public static final int IME_COMPOSITION_END = 0;
    public static final int IME_COMPOSITION_BEGIN = 1;
    public static final int IME_SET_TEXT = 2;
    public static final int IME_GET_TEXT = 3;
    public static final int IME_DELETE_TEXT = 4;
    public static final int IME_SET_SELECTION = 5;
    public static final int IME_GET_SELECTION = 6;
    public static final int IME_ADD_RANGE = 7;

    public static final int IME_RANGE_CARETPOSITION = 1;
    public static final int IME_RANGE_RAWINPUT = 2;
    public static final int IME_RANGE_SELECTEDRAWTEXT = 3;
    public static final int IME_RANGE_CONVERTEDTEXT = 4;
    public static final int IME_RANGE_SELECTEDCONVERTEDTEXT = 5;

    public static final int IME_RANGE_UNDERLINE = 1;
    public static final int IME_RANGE_FORECOLOR = 2;
    public static final int IME_RANGE_BACKCOLOR = 4;

    public int mType;
    public int mAction;
    public long mTime;
    public Point[] mPoints;
    public int[] mPointIndicies;
    public int mPointerIndex;
    public float[] mOrientations;
    public float[] mPressures;
    public Point[] mPointRadii;
    public Rect mRect;
    public double mX, mY, mZ;
    public double mAlpha, mBeta, mGamma;

    public int mMetaState, mFlags;
    public int mKeyCode, mUnicodeChar;
    public int mOffset, mCount;
    public String mCharacters, mCharactersExtra;
    public int mRangeType, mRangeStyles;
    public int mRangeForeColor, mRangeBackColor;
    public Location mLocation;
    public Address  mAddress;

    public double mBandwidth;
    public boolean mCanBeMetered;

    public int mNativeWindow;

    public GeckoEvent() {
        mType = NATIVE_POKE;
    }

    public GeckoEvent(int evType) {
        mType = evType;
    }

    public GeckoEvent(KeyEvent k) {
        mType = KEY_EVENT;
        mAction = k.getAction();
        mTime = k.getEventTime();
        mMetaState = k.getMetaState();
        mFlags = k.getFlags();
        mKeyCode = k.getKeyCode();
        mUnicodeChar = k.getUnicodeChar();
        mCharacters = k.getCharacters();
    }

    public GeckoEvent(MotionEvent m) {
        mType = MOTION_EVENT;
        mAction = m.getAction();
        mTime = m.getEventTime();
        mMetaState = m.getMetaState();

        switch (mAction & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE: {
                mCount = m.getPointerCount();
                mPoints = new Point[mCount];
                mPointIndicies = new int[mCount];
                mOrientations = new float[mCount];
                mPressures = new float[mCount];
                mPointRadii = new Point[mCount];
                mPointerIndex = (mAction & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                for (int i = 0; i < mCount; i++) {
                    addMotionPoint(i, i, m);
                }
                break;
            }
            default: {
                mCount = 0;
                mPointerIndex = -1;
                mPoints = new Point[mCount];
                mPointIndicies = new int[mCount];
                mOrientations = new float[mCount];
                mPressures = new float[mCount];
                mPointRadii = new Point[mCount];
            }
        }
    }

    public void addMotionPoint(int index, int eventIndex, MotionEvent event) {
        PointF geckoPoint = new PointF(event.getX(eventIndex), event.getY(eventIndex));
    
        mPoints[index] = new Point((int)Math.round(geckoPoint.x), (int)Math.round(geckoPoint.y));
        mPointIndicies[index] = event.getPointerId(eventIndex);
        
        if (Build.VERSION.SDK_INT >= 9) {
            double radians = event.getOrientation(eventIndex);
            mOrientations[index] = (float) Math.toDegrees(radians);
            
            
            if (mOrientations[index] == 90)
                mOrientations[index] = -90;

            
            
            
            
            
            
            
            if (mOrientations[index] < 0) {
                mOrientations[index] += 90;
                mPointRadii[index] = new Point((int)event.getToolMajor(eventIndex)/2,
                                               (int)event.getToolMinor(eventIndex)/2);
            } else {
                mPointRadii[index] = new Point((int)event.getToolMinor(eventIndex)/2,
                                               (int)event.getToolMajor(eventIndex)/2);
            }
        } else {
            float size = event.getSize(eventIndex);
            DisplayMetrics displaymetrics = new DisplayMetrics();
            GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(displaymetrics);
            size = size*Math.min(displaymetrics.heightPixels, displaymetrics.widthPixels);
            mPointRadii[index] = new Point((int)size,(int)size);
            mOrientations[index] = 0;
        }
        mPressures[index] = event.getPressure(eventIndex);
    }

    public GeckoEvent(SensorEvent s) {

        if (s.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            mType = ACCELERATION_EVENT;
            mX = s.values[0];
            mY = s.values[1];
            mZ = s.values[2];
        }
        else {
            mType = ORIENTATION_EVENT;
            mAlpha = -s.values[0];
            mBeta = -s.values[1];
            mGamma = -s.values[2];
            Log.i("GeckoEvent", "SensorEvent type = " + s.sensor.getType() + " " + s.sensor.getName() + " " + mAlpha + " " + mBeta + " " + mGamma );
        }
    }

    public GeckoEvent(Location l, Address a) {
        mType = LOCATION_EVENT;
        mLocation = l;
        mAddress  = a;
    }

    public GeckoEvent(int imeAction, int offset, int count) {
        mType = IME_EVENT;
        mAction = imeAction;
        mOffset = offset;
        mCount = count;
    }

    private void InitIMERange(int action, int offset, int count,
                              int rangeType, int rangeStyles,
                              int rangeForeColor, int rangeBackColor) {
        mType = IME_EVENT;
        mAction = action;
        mOffset = offset;
        mCount = count;
        mRangeType = rangeType;
        mRangeStyles = rangeStyles;
        mRangeForeColor = rangeForeColor;
        mRangeBackColor = rangeBackColor;
        return;
    }
    
    public GeckoEvent(int offset, int count,
                      int rangeType, int rangeStyles,
                      int rangeForeColor, int rangeBackColor, String text) {
        InitIMERange(IME_SET_TEXT, offset, count, rangeType, rangeStyles,
                     rangeForeColor, rangeBackColor);
        mCharacters = text;
    }

    public GeckoEvent(int offset, int count,
                      int rangeType, int rangeStyles,
                      int rangeForeColor, int rangeBackColor) {
        InitIMERange(IME_ADD_RANGE, offset, count, rangeType, rangeStyles,
                     rangeForeColor, rangeBackColor);
    }

    public GeckoEvent(int etype, Rect dirty) {
        if (etype != DRAW) {
            mType = INVALID;
            return;
        }

        mType = etype;
        mRect = dirty;
    }

    public GeckoEvent(int etype, int w, int h, int screenw, int screenh) {
        if (etype != SIZE_CHANGED) {
            mType = INVALID;
            return;
        }

        mType = etype;

        mPoints = new Point[3];
        mPoints[0] = new Point(w, h);
        mPoints[1] = new Point(screenw, screenh);
        mPoints[2] = new Point(0, 0);
    }

    public GeckoEvent(String subject, String data) {
        mType = BROADCAST;
        mCharacters = subject;
        mCharactersExtra = data;
    }

    public GeckoEvent(String uri) {
        mType = LOAD_URI;
        mCharacters = uri;
    }

    public GeckoEvent(double bandwidth, boolean canBeMetered) {
        mType = NETWORK_CHANGED;
        mBandwidth = bandwidth;
        mCanBeMetered = canBeMetered;
    }
}
