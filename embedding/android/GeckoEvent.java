




































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

import android.util.Log;






public class GeckoEvent {
    public static final int INVALID = -1;
    public static final int NATIVE_POKE = 0;
    public static final int KEY_EVENT = 1;
    public static final int MOTION_EVENT = 2;
    public static final int SENSOR_EVENT = 3;
    public static final int LOCATION_EVENT = 4;
    public static final int IME_EVENT = 5;
    public static final int DRAW = 6;
    public static final int SIZE_CHANGED = 7;
    public static final int ACTIVITY_STOPPING = 8;
    public static final int ACTIVITY_PAUSING = 9;
    public static final int ACTIVITY_SHUTDOWN = 10;
    public static final int LOAD_URI = 11;
    public static final int SURFACE_CREATED = 12;
    public static final int SURFACE_DESTROYED = 13;
    public static final int GECKO_EVENT_SYNC = 14;

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
    public Point mP0, mP1;
    public Rect mRect;
    public double mAlpha, mBeta, mGamma;

    public int mMetaState, mFlags;
    public int mKeyCode, mUnicodeChar;
    public int mOffset, mCount;
    public String mCharacters;
    public int mRangeType, mRangeStyles;
    public int mRangeForeColor, mRangeBackColor;
    public Location mLocation;
    public Address  mAddress;

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
        mP0 = new Point((int)m.getX(0), (int)m.getY(0));
        mCount = m.getPointerCount();
        if (mCount > 1)
            mP1 = new Point((int)m.getX(1), (int)m.getY(1));
    }

    public GeckoEvent(SensorEvent s) {
        mType = SENSOR_EVENT;
        
        
        
        
        
        float magnitude = FloatMath.sqrt(s.values[0] * s.values[0] +
                                         s.values[1] * s.values[1] + 
                                         s.values[2] * s.values[2]);
        mAlpha = 0; 
        mBeta = Math.toDegrees(Math.asin(s.values[1] / magnitude));
        mGamma = -Math.toDegrees(Math.asin(s.values[0] / magnitude));
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

        mP0 = new Point(w, h);
        mP1 = new Point(screenw, screenh);
    }

    public GeckoEvent(String uri) {
        mType = LOAD_URI;
        mCharacters = uri;
    }
}
