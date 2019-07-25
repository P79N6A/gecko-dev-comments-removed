




































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

            mFormat = format;
            mWidth = width;
            mHeight = height;
            mSurfaceValid = true;

            Log.i("GeckoAppJava", "surfaceChanged: fmt: " + format + " dim: " + width + " " + height);

            
            if (!GeckoAppShell.sGeckoRunning) {
                GeckoAppShell.setInitialSize(width, height);
                return;
            }

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
    }

    public ByteBuffer getSoftwareDrawBuffer() {
        
        if (!mSurfaceLock.isHeldByCurrentThread())
            Log.e("GeckoAppJava", "getSoftwareDrawBuffer called outside of mSurfaceLock!");
        

        return mSoftwareBuffer;
    }

    



    public static final int DRAW_ERROR = 0;
    public static final int DRAW_GLES_2 = 1;
    public static final int DRAW_SOFTWARE = 2;

    int innerBeginDrawing() {
        


        if (GeckoApp.useSoftwareDrawing) {
            if (mWidth != mBufferWidth ||
                mHeight != mBufferHeight ||
                mSurfaceChanged)
            {
                if (mWidth*mHeight != mBufferWidth*mBufferHeight)
                    mSoftwareBuffer = ByteBuffer.allocateDirect(mWidth*mHeight*4);

                mSoftwareBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);

                mBufferWidth = mWidth;
                mBufferHeight = mHeight;
                mSurfaceChanged = false;
            }

            mSoftwareCanvas = getHolder().lockCanvas(null);
            if (mSoftwareCanvas == null) {
                Log.e("GeckoAppJava", "lockCanvas failed! << beginDrawing");
                return DRAW_ERROR;
            }

            return DRAW_SOFTWARE;
        }

        return DRAW_GLES_2;
    }

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

        
        int result = innerBeginDrawing();

        if (result == DRAW_ERROR) {
            mSurfaceLock.unlock();
            return DRAW_ERROR;
        }

        mInDrawing = true;
        return result;
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

            if (GeckoApp.useSoftwareDrawing) {
                if (!mSurfaceChanged) {
                    mSoftwareBitmap.copyPixelsFromBuffer(mSoftwareBuffer);
                    mSoftwareCanvas.drawBitmap(mSoftwareBitmap, 0, 0, null);

                    getHolder().unlockCanvasAndPost(mSoftwareCanvas);
                    mSoftwareCanvas = null;
                }
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

    @Override
    public boolean onCheckIsTextEditor () {
        return false;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT |
                             InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
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

    
    GeckoInputConnection inputConnection;
    int mIMEState;

    
    ByteBuffer mSoftwareBuffer;
    Bitmap mSoftwareBitmap;
    Canvas mSoftwareCanvas;
}

class GeckoInputConnection
    extends BaseInputConnection
{
    public GeckoInputConnection (View targetView) {
        super(targetView, true);
        mQueryResult = new SynchronousQueue<String>();
        mExtractedText.partialStartOffset = -1;
        mExtractedText.partialEndOffset = -1;
    }

    @Override
    public Editable getEditable() {
        Log.i("GeckoAppJava", "getEditable");
        return null;
    }

    @Override
    public boolean beginBatchEdit() {
        if (mComposing)
            return true;
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, null));
        mComposing = true;
        return true;
    }
    @Override
    public boolean commitCompletion(CompletionInfo text) {
        Log.i("GeckoAppJava", "Stub: commitCompletion");
        return true;
    }
    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, text.toString()));
        endBatchEdit();
        return true;
    }
    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(leftLength, rightLength));
        updateExtractedText();
        return true;
    }
    @Override
    public boolean endBatchEdit() {
        updateExtractedText();
        if (!mComposing)
            return true;
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, null));
        mComposing = false;
        return true;
    }
    @Override
    public boolean finishComposingText() {
        endBatchEdit();
        return true;
    }
    @Override
    public int getCursorCapsMode(int reqModes) {
        return 0;
    }
    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest req, int flags) {
        mExtractToken = req.token;
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, 0));
        try {
            mExtractedText.text = mQueryResult.take();
            mExtractedText.selectionStart = mSelectionStart;
            mExtractedText.selectionEnd = mSelectionEnd;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getExtractedText: Interrupted!");
        }
        return mExtractedText;
    }
    @Override
    public CharSequence getTextAfterCursor(int length, int flags) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, length));
        try {
            String result = mQueryResult.take();
            return result;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getTextAfterCursor: Interrupted!");
        }
        return null;
    }
    @Override
    public CharSequence getTextBeforeCursor(int length, int flags) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, length));
        try {
            String result = mQueryResult.take();
            return result;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getTextBeforeCursor: Interrupted!");
        }
        return null;
    }
    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        beginBatchEdit();
        GeckoAppShell.sendEventToGecko(new GeckoEvent(true, text.toString()));
        return true;
    }
    @Override
    public boolean setSelection(int start, int end) {
        Log.i("GeckoAppJava", "Stub: setSelection " + start + " " + end);
        return true;
    }

    private void updateExtractedText() {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(false, 0));
        try {
            mExtractedText.text = mQueryResult.take();
            mExtractedText.selectionStart = mSelectionStart;
            mExtractedText.selectionEnd = mSelectionEnd;
        } catch (InterruptedException e) {
            Log.i("GeckoAppJava", "getExtractedText: Interrupted!");
        }

        InputMethodManager imm = (InputMethodManager)
            GeckoApp.surfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.updateExtractedText(GeckoApp.surfaceView, mExtractToken, mExtractedText);
    }

    boolean mComposing;
    int mExtractToken;
    final ExtractedText mExtractedText = new ExtractedText();

    int mSelectionStart, mSelectionEnd;
    SynchronousQueue<String> mQueryResult;
}
