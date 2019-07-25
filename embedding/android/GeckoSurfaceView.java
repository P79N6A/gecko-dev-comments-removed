




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.locks.*;
import java.util.concurrent.atomic.*;
import java.util.zip.*;
import java.nio.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;
import android.location.*;

import android.util.*;







class GeckoSurfaceView
    extends SurfaceView
    implements SurfaceHolder.Callback, SensorEventListener, LocationListener
{
    public GeckoSurfaceView(Context context) {
        super(context);

        getHolder().addCallback(this);
        inputConnection = new GeckoInputConnection(this);
        setFocusable(true);
        setFocusableInTouchMode(true);

        mWidth = 0;
        mHeight = 0;
        mBufferWidth = 0;
        mBufferHeight = 0;

        mSurfaceLock = new ReentrantLock();

        mIMEState = IME_STATE_DISABLED;
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }

    



    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mSurfaceLock.lock();

        try {
            if (mInDrawing) {
                Log.w("GeckoAppJava", "surfaceChanged while mInDrawing is true!");
            }

            if (width == 0 || height == 0)
                mSoftwareBuffer = null;
            else if (mSoftwareBuffer == null ||
                     mSoftwareBuffer.capacity() < (width * height * 2) ||
                     mWidth != width || mHeight != height)
                mSoftwareBuffer = ByteBuffer.allocateDirect(width * height * 2);

            mFormat = format;
            mWidth = width;
            mHeight = height;
            mSurfaceValid = true;

            Log.i("GeckoAppJava", "surfaceChanged: fmt: " + format + " dim: " + width + " " + height);

            if (!GeckoAppShell.sGeckoRunning)
                return;

            GeckoEvent e = new GeckoEvent(GeckoEvent.SIZE_CHANGED, width, height, -1, -1);
            GeckoAppShell.sendEventToGecko(e);

            if (mSurfaceNeedsRedraw) {
                GeckoAppShell.scheduleRedraw();
                mSurfaceNeedsRedraw = false;
            }

            mSurfaceChanged = true;
        } finally {
            mSurfaceLock.unlock();
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
        if (GeckoAppShell.sGeckoRunning)
            mSurfaceNeedsRedraw = true;
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i("GeckoAppJava", "surface destroyed");
        mSurfaceValid = false;
        mSoftwareBuffer = null;
    }

    public ByteBuffer getSoftwareDrawBuffer() {
        return mSoftwareBuffer;
    }

    



    public static final int DRAW_ERROR = 0;
    public static final int DRAW_GLES_2 = 1;

    public int beginDrawing() {
        if (mInDrawing) {
            Log.e("GeckoAppJava", "Recursive beginDrawing call!");
            return DRAW_ERROR;
        }

        










        mSurfaceLock.lock();

        if (!mSurfaceValid) {
            Log.e("GeckoAppJava", "Surface not valid");
            mSurfaceLock.unlock();
            return DRAW_ERROR;
        }

        mInDrawing = true;
        return DRAW_GLES_2;
    }

    public void endDrawing() {
        if (!mInDrawing) {
            Log.e("GeckoAppJava", "endDrawing without beginDrawing!");
            return;
        }

        try {
            if (!mSurfaceValid) {
                Log.e("GeckoAppJava", "endDrawing with false mSurfaceValid");
                return;
            }
        } catch (java.lang.IllegalArgumentException ex) {
            mSurfaceChanged = true;
        } finally {
            mInDrawing = false;

            if (!mSurfaceLock.isHeldByCurrentThread())
                Log.e("GeckoAppJava", "endDrawing while mSurfaceLock not held by current thread!");

            mSurfaceLock.unlock();
        }
    }

    public void draw2D(ByteBuffer buffer) {
        if (GeckoApp.mAppContext.mProgressDialog != null) {
            GeckoApp.mAppContext.mProgressDialog.dismiss();
            GeckoApp.mAppContext.mProgressDialog = null;
        }
        if (buffer != mSoftwareBuffer)
            return;
        Canvas c = getHolder().lockCanvas();
        if (c == null)
            return;
        if (buffer != mSoftwareBuffer) {
            getHolder().unlockCanvasAndPost(c);
            return;
        }
        if (mSoftwareBitmap == null ||
            mSoftwareBitmap.getHeight() != mHeight ||
            mSoftwareBitmap.getWidth() != mWidth) {
            mSoftwareBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.RGB_565);
        }
        mSoftwareBitmap.copyPixelsFromBuffer(mSoftwareBuffer);
        c.drawBitmap(mSoftwareBitmap, 0, 0, null);
        getHolder().unlockCanvasAndPost(c);
    }

    @Override
    public boolean onCheckIsTextEditor () {
        return false;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (!mIMEFocus)
            return null;

        outAttrs.inputType = InputType.TYPE_CLASS_TEXT |
                             InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;

        if (mIMEState == IME_STATE_PASSWORD)
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_PASSWORD;

        inputConnection.reset();
        return inputConnection;
    }

    
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
    }

    public void onSensorChanged(SensorEvent event)
    {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
    }

    
    public void onLocationChanged(Location location)
    {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(location));
    }

    public void onProviderDisabled(String provider)
    {
    }

    public void onProviderEnabled(String provider)
    {
    }

    public void onStatusChanged(String provider, int status, Bundle extras)
    {
    }

    
    public boolean onTouchEvent(MotionEvent event) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    
    boolean mSurfaceValid;

    
    boolean mSurfaceNeedsRedraw;

    
    
    boolean mSurfaceChanged;

    
    boolean mInDrawing;

    
    
    
    ReentrantLock mSurfaceLock;

    
    
    int mFormat;

    
    int mWidth;
    int mHeight;

    
    
    int mBufferWidth;
    int mBufferHeight;

    
    public static final int IME_STATE_DISABLED = 0;
    public static final int IME_STATE_ENABLED = 1;
    public static final int IME_STATE_PASSWORD = 2;

    GeckoInputConnection inputConnection;
    boolean mIMEFocus;
    int mIMEState;

    
    ByteBuffer mSoftwareBuffer;
    Bitmap mSoftwareBitmap;
}
