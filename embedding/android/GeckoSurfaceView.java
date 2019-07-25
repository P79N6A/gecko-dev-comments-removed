




































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
import android.text.method.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;
import android.location.*;
import android.graphics.drawable.*;
import android.content.res.*;

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
        
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().
            getDefaultDisplay().getMetrics(metrics);
        mWidth = metrics.widthPixels;
        mHeight = metrics.heightPixels;
        mBufferWidth = 0;
        mBufferHeight = 0;

        mSurfaceLock = new ReentrantLock();

        mEditableFactory = Editable.Factory.getInstance();
        initEditable("");
        mIMEState = IME_STATE_DISABLED;
        mIMETypeHint = "";
        mIMEActionHint = "";
    }

    protected void finalize() throws Throwable {
        super.finalize();
    }

    void drawSplashScreen() {
        this.drawSplashScreen(getHolder(), mWidth, mHeight);
    }

    void drawSplashScreen(SurfaceHolder holder, int width, int height) {
        Canvas c = holder.lockCanvas();
        if (c == null) {
            Log.i("GeckoSurfaceView", "canvas is null");
            return;
        }
        Resources res = getResources();
        c.drawColor(res.getColor(R.color.splash_background));
        Drawable drawable = res.getDrawable(R.drawable.splash);
        int w = drawable.getIntrinsicWidth();
        int h = drawable.getIntrinsicHeight();
        int x = (width - w)/2;
        int y = (height - h)/2 - 16;
        drawable.setBounds(x, y, x + w, y + h);
        drawable.draw(c);
        Paint p = new Paint();
        p.setTextAlign(Paint.Align.CENTER);
        p.setTextSize(32f);
        p.setAntiAlias(true);
        p.setColor(res.getColor(R.color.splash_font));
        c.drawText(GeckoSurfaceView.mSplashStatusMsg, width/2, y + h + 16, p);
        holder.unlockCanvasAndPost(c);
    }

    



    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mShowingSplashScreen)
            drawSplashScreen(holder, width, height);
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
            boolean doSyncDraw = mDrawMode == DRAW_2D &&
                mSoftwareBuffer != null &&
                GeckoApp.checkLaunchState(GeckoApp.LaunchState.GeckoRunning);
            mSyncDraw = doSyncDraw;

            mFormat = format;
            mWidth = width;
            mHeight = height;
            mSurfaceValid = true;

            Log.i("GeckoAppJava", "surfaceChanged: fmt: " + format + " dim: " + width + " " + height);

            DisplayMetrics metrics = new DisplayMetrics();
            GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

            GeckoEvent e = new GeckoEvent(GeckoEvent.SIZE_CHANGED, width, height,
                                          metrics.widthPixels, metrics.heightPixels);
            GeckoAppShell.sendEventToGecko(e);

            if (mSoftwareBuffer != null)
                GeckoAppShell.scheduleRedraw();

            if (!doSyncDraw) {
                if (mDrawMode == DRAW_GLES_2 || mShowingSplashScreen)
                    return;
                Canvas c = holder.lockCanvas();
                c.drawARGB(255, 255, 255, 255);
                holder.unlockCanvasAndPost(c);
                return;
            }
        } finally {
            mSurfaceLock.unlock();
        }

        ByteBuffer bb = null;
        try {
            bb = mSyncBuf.take();
        } catch (InterruptedException ie) {
            Log.e("GeckoAppJava", "Threw exception while getting sync buf: ", ie);
        }
        if (bb != null && bb.capacity() == (width * height * 2)) {
            mSoftwareBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.RGB_565);
            mSoftwareBitmap.copyPixelsFromBuffer(bb);
            Canvas c = holder.lockCanvas();
            c.drawBitmap(mSoftwareBitmap, 0, 0, null);
            holder.unlockCanvasAndPost(c);
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
        Log.i("GeckoAppJava", "surface created");
        GeckoEvent e = new GeckoEvent(GeckoEvent.SURFACE_CREATED);
        GeckoAppShell.sendEventToGecko(e);
        if (mShowingSplashScreen)
            drawSplashScreen();
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i("GeckoAppJava", "surface destroyed");
        mSurfaceValid = false;
        mSoftwareBuffer = null;
        GeckoEvent e = new GeckoEvent(GeckoEvent.SURFACE_DESTROYED);
        GeckoAppShell.sendEventToGecko(e);
    }

    public ByteBuffer getSoftwareDrawBuffer() {
        mDrawMode = DRAW_2D;
        return mSoftwareBuffer;
    }

    



    public static final int DRAW_ERROR = 0;
    public static final int DRAW_GLES_2 = 1;
    public static final int DRAW_2D = 2;

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
        mDrawMode = DRAW_GLES_2;
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
        } finally {
            mInDrawing = false;

            if (!mSurfaceLock.isHeldByCurrentThread())
                Log.e("GeckoAppJava", "endDrawing while mSurfaceLock not held by current thread!");

            mSurfaceLock.unlock();
        }
    }

    












    public void draw2D(ByteBuffer buffer, int stride) {
        
        
        mSurfaceLock.lock();
        try {
            if (mSyncDraw) {
                if (buffer != mSoftwareBuffer || stride != (mWidth * 2))
                    return;
                mSyncDraw = false;
                try {
                    mSyncBuf.put(buffer);
                } catch (InterruptedException ie) {
                    Log.e("GeckoAppJava", "Threw exception while getting sync buf: ", ie);
                }
                return;
            }
        } finally {
            mSurfaceLock.unlock();
        }

        if (buffer != mSoftwareBuffer || stride != (mWidth * 2))
            return;
        Canvas c = getHolder().lockCanvas();
        if (c == null)
            return;
        if (buffer != mSoftwareBuffer || stride != (mWidth * 2)) {
            




            c.drawARGB(255, 255, 255, 255);
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
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
        outAttrs.imeOptions = EditorInfo.IME_ACTION_NONE;
        outAttrs.actionLabel = null;
        mKeyListener = TextKeyListener.getInstance();

        if (mIMEState == IME_STATE_PASSWORD)
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
        else if (mIMETypeHint.equalsIgnoreCase("url"))
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_URI;
        else if (mIMETypeHint.equalsIgnoreCase("email"))
            outAttrs.inputType |= InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
        else if (mIMETypeHint.equalsIgnoreCase("search"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEARCH;
        else if (mIMETypeHint.equalsIgnoreCase("tel"))
            outAttrs.inputType = InputType.TYPE_CLASS_PHONE;
        else if (mIMETypeHint.equalsIgnoreCase("number") ||
                 mIMETypeHint.equalsIgnoreCase("range"))
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER;
        else if (mIMETypeHint.equalsIgnoreCase("datetime") ||
                 mIMETypeHint.equalsIgnoreCase("datetime-local"))
            outAttrs.inputType = InputType.TYPE_CLASS_DATETIME |
                                 InputType.TYPE_DATETIME_VARIATION_NORMAL;
        else if (mIMETypeHint.equalsIgnoreCase("date"))
            outAttrs.inputType = InputType.TYPE_CLASS_DATETIME |
                                 InputType.TYPE_DATETIME_VARIATION_DATE;
        else if (mIMETypeHint.equalsIgnoreCase("time"))
            outAttrs.inputType = InputType.TYPE_CLASS_DATETIME |
                                 InputType.TYPE_DATETIME_VARIATION_TIME;

        if (mIMEActionHint.equalsIgnoreCase("go"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_GO;
        else if (mIMEActionHint.equalsIgnoreCase("done"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE;
        else if (mIMEActionHint.equalsIgnoreCase("next"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_NEXT;
        else if (mIMEActionHint.equalsIgnoreCase("search"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEARCH;
        else if (mIMEActionHint.equalsIgnoreCase("send"))
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEND;
        else if (mIMEActionHint != null && mIMEActionHint.length() != 0)
            outAttrs.actionLabel = mIMEActionHint;

        if (mIMELandscapeFS == false)
            outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI;

        inputConnection.reset();
        return inputConnection;
    }

    public void setEditable(String contents)
    {
        mEditable.removeSpan(inputConnection);
        mEditable.replace(0, mEditable.length(), contents);
        mEditable.setSpan(inputConnection, 0, contents.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE);
        Selection.setSelection(mEditable, contents.length());
    }

    public void initEditable(String contents)
    {
        mEditable = mEditableFactory.newEditable(contents);
        mEditable.setSpan(inputConnection, 0, contents.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE);
        Selection.setSelection(mEditable, contents.length());
    }

    
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
    }

    public void onSensorChanged(SensorEvent event)
    {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
    }

    private class GeocoderTask extends AsyncTask<Location, Void, Void> {
        protected Void doInBackground(Location... location) {
            try {
                List<Address> addresses = mGeocoder.getFromLocation(location[0].getLatitude(),
                                                                    location[0].getLongitude(), 1);
                
                
                
                mLastGeoAddress = addresses.get(0);
                GeckoAppShell.sendEventToGecko(new GeckoEvent(location[0], mLastGeoAddress));
            } catch (Exception e) {
                Log.w("GeckoSurfaceView", "GeocoderTask "+e);
            }
            return null;
        }
    }

    
    public void onLocationChanged(Location location)
    {
        if (mGeocoder == null)
            mGeocoder = new Geocoder(getContext(), Locale.getDefault());

        if (mLastGeoAddress == null) {
            new GeocoderTask().execute(location);
        }
        else {
            float[] results = new float[1];
            Location.distanceBetween(location.getLatitude(),
                                     location.getLongitude(),
                                     mLastGeoAddress.getLatitude(),
                                     mLastGeoAddress.getLongitude(),
                                     results);
            
            
            
            if (results[0] > 100)
                new GeocoderTask().execute(location);
        }

        GeckoAppShell.sendEventToGecko(new GeckoEvent(location, mLastGeoAddress));
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

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (event.isSystem())
            return super.onKeyPreIme(keyCode, event);

        switch (event.getAction()) {
            case KeyEvent.ACTION_DOWN:
                return onKeyDown(keyCode, event);
            case KeyEvent.ACTION_UP:
                return onKeyUp(keyCode, event);
            case KeyEvent.ACTION_MULTIPLE:
                return onKeyMultiple(keyCode, event.getRepeatCount(), event);
        }
        return super.onKeyPreIme(keyCode, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (event.getRepeatCount() == 0) {
                    event.startTracking();
                    return true;
                } else {
                    return false;
                }
            case KeyEvent.KEYCODE_MENU:
                if (event.getRepeatCount() == 0) {
                    event.startTracking();
                    break;
                } else if ((event.getFlags() & KeyEvent.FLAG_LONG_PRESS) != 0) {
                    break;
                }
                
                return false;
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_SEARCH:
                return false;
            case KeyEvent.KEYCODE_DEL:
                
                if (inputConnection != null &&
                    inputConnection.onKeyDel()) {
                    return true;
                }
                break;
            case KeyEvent.KEYCODE_ENTER:
                if ((event.getFlags() & KeyEvent.FLAG_EDITOR_ACTION) != 0 &&
                    mIMEActionHint.equalsIgnoreCase("next"))
                    event = new KeyEvent(event.getAction(), KeyEvent.KEYCODE_TAB);
                break;
            default:
                break;
        }
        
        if (mIMEState == IME_STATE_DISABLED ||
            keyCode == KeyEvent.KEYCODE_ENTER ||
            (event.getFlags() & KeyEvent.FLAG_SOFT_KEYBOARD) != 0 ||
            !mKeyListener.onKeyDown(this, mEditable, keyCode, event))
            GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (!event.isTracking() || event.isCanceled())
                    return false;
                break;
            default:
                break;
        }
        if (mIMEState == IME_STATE_DISABLED ||
            keyCode == KeyEvent.KEYCODE_ENTER ||
            (event.getFlags() & KeyEvent.FLAG_SOFT_KEYBOARD) != 0 ||
            !mKeyListener.onKeyUp(this, mEditable, keyCode, event))
            GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
                return true;
            case KeyEvent.KEYCODE_MENU:
                InputMethodManager imm = (InputMethodManager)
                    getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.toggleSoftInputFromWindow(getWindowToken(),
                                              imm.SHOW_FORCED, 0);
                return true;
            default:
                break;
        }
        return false;
    }

    
    boolean mSurfaceValid;

    
    boolean mInDrawing;

    
    boolean mSyncDraw;

    
    int mDrawMode;

    static boolean mShowingSplashScreen = true;
    static String  mSplashStatusMsg = "";

    
    
    
    ReentrantLock mSurfaceLock;

    
    
    int mFormat;

    
    int mWidth;
    int mHeight;

    
    
    int mBufferWidth;
    int mBufferHeight;

    
    public static final int IME_STATE_DISABLED = 0;
    public static final int IME_STATE_ENABLED = 1;
    public static final int IME_STATE_PASSWORD = 2;
    public static final int IME_STATE_PLUGIN = 3;

    GeckoInputConnection inputConnection;
    KeyListener mKeyListener;
    Editable mEditable;
    Editable.Factory mEditableFactory;
    int mIMEState;
    String mIMETypeHint;
    String mIMEActionHint;
    boolean mIMELandscapeFS;

    
    ByteBuffer mSoftwareBuffer;
    Bitmap mSoftwareBitmap;

    Geocoder mGeocoder;
    Address  mLastGeoAddress;

    final SynchronousQueue<ByteBuffer> mSyncBuf = new SynchronousQueue<ByteBuffer>();
}

