




































package org.mozilla.gecko;

import android.os.*;
import android.app.*;
import android.view.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;

import android.util.Log;






public class GeckoEvent {
    public static final int INVALID = -1;
    public static final int NATIVE_POKE = 0;
    public static final int KEY_EVENT = 1;
    public static final int MOTION_EVENT = 2;
    public static final int SENSOR_EVENT = 3;
    public static final int IME_EVENT = 4;
    public static final int DRAW = 5;
    public static final int SIZE_CHANGED = 6;
    public static final int ACTIVITY_STOPPING = 7;

    public static final int IME_BATCH_END = 0;
    public static final int IME_BATCH_BEGIN = 1;
    public static final int IME_SET_TEXT = 2;
    public static final int IME_GET_TEXT = 3;
    public static final int IME_DELETE_TEXT = 4;

    public int mType;
    public int mAction;
    public long mTime;
    public Point mP0, mP1;
    public Rect mRect;
    public float mX, mY, mZ;

    public int mMetaState, mFlags;
    public int mKeyCode, mUnicodeChar;
    public int mCount, mCount2;
    public String mCharacters;

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
        mP0 = new Point((int)m.getX(), (int)m.getY());
    }

    public GeckoEvent(SensorEvent s) {
        mType = SENSOR_EVENT;
        mX = s.values[0] / SensorManager.GRAVITY_EARTH;
        mY = s.values[1] / SensorManager.GRAVITY_EARTH;
        mZ = s.values[2] / SensorManager.GRAVITY_EARTH;
    }

    public GeckoEvent(boolean batchEdit, String text) {
        mType = IME_EVENT;
        if (text != null)
            mAction = IME_SET_TEXT;
        else
            mAction = batchEdit ? IME_BATCH_BEGIN : IME_BATCH_END;
        mCharacters = text;
    }

    public GeckoEvent(boolean forward, int count) {
        mType = IME_EVENT;
        mAction = IME_GET_TEXT;
        if (forward)
            mCount = count;
        else
            mCount2 = count;
    }

    public GeckoEvent(int leftLen, int rightLen) {
        mType = IME_EVENT;
        mAction = IME_DELETE_TEXT;
        mCount = leftLen;
        mCount2 = rightLen;
    }

    public GeckoEvent(int etype, Rect dirty) {
        if (etype != DRAW) {
            mType = INVALID;
            return;
        }

        mType = etype;
        mRect = dirty;
    }

    public GeckoEvent(int etype, int w, int h, int oldw, int oldh) {
        if (etype != SIZE_CHANGED) {
            mType = INVALID;
            return;
        }

        mType = etype;

        mP0 = new Point(w, h);
        mP1 = new Point(oldw, oldh);
    }
}
