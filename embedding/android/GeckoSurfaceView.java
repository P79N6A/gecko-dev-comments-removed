




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.locks.*;
import java.util.concurrent.atomic.*;
import java.util.zip.*;
import java.nio.*;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;

import android.util.*;







class GeckoSurfaceView
    extends SurfaceView
    implements SurfaceHolder.Callback, SensorEventListener
{
    public GeckoSurfaceView(Context context) {
        super(context);

        getHolder().addCallback(this);
        inputConnection = new GeckoInputConnection(this);
        setFocusable(true);
        setFocusableInTouchMode(true);

        if (!GeckoApp.useSoftwareDrawing)
            startEgl();

        mWidth = 0;
        mHeight = 0;
        mBufferWidth = 0;
        mBufferHeight = 0;

        mSurfaceLock = new ReentrantLock();
    }

    protected void finalize() throws Throwable {
        super.finalize();
        if (!GeckoApp.useSoftwareDrawing)
            finishEgl();
    }

    private static final int EGL_OPENGL_ES2_BIT = 4;
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

    private void printConfig(EGL10 egl, EGLDisplay display,
                             EGLConfig config) {
        int[] attributes = {
            EGL10.EGL_BUFFER_SIZE,
            EGL10.EGL_ALPHA_SIZE,
            EGL10.EGL_BLUE_SIZE,
            EGL10.EGL_GREEN_SIZE,
            EGL10.EGL_RED_SIZE,
            EGL10.EGL_DEPTH_SIZE,
            EGL10.EGL_STENCIL_SIZE,
            EGL10.EGL_CONFIG_CAVEAT,
            EGL10.EGL_CONFIG_ID,
            EGL10.EGL_LEVEL,
            EGL10.EGL_MAX_PBUFFER_HEIGHT,
            EGL10.EGL_MAX_PBUFFER_PIXELS,
            EGL10.EGL_MAX_PBUFFER_WIDTH,
            EGL10.EGL_NATIVE_RENDERABLE,
            EGL10.EGL_NATIVE_VISUAL_ID,
            EGL10.EGL_NATIVE_VISUAL_TYPE,
            0x3030, 
            EGL10.EGL_SAMPLES,
            EGL10.EGL_SAMPLE_BUFFERS,
            EGL10.EGL_SURFACE_TYPE,
            EGL10.EGL_TRANSPARENT_TYPE,
            EGL10.EGL_TRANSPARENT_RED_VALUE,
            EGL10.EGL_TRANSPARENT_GREEN_VALUE,
            EGL10.EGL_TRANSPARENT_BLUE_VALUE,
            0x3039, 
            0x303A, 
            0x303B, 
            0x303C, 
            EGL10.EGL_LUMINANCE_SIZE,
            EGL10.EGL_ALPHA_MASK_SIZE,
            EGL10.EGL_COLOR_BUFFER_TYPE,
            EGL10.EGL_RENDERABLE_TYPE,
            0x3042 
        };
        String[] names = {
            "EGL_BUFFER_SIZE",
            "EGL_ALPHA_SIZE",
            "EGL_BLUE_SIZE",
            "EGL_GREEN_SIZE",
            "EGL_RED_SIZE",
            "EGL_DEPTH_SIZE",
            "EGL_STENCIL_SIZE",
            "EGL_CONFIG_CAVEAT",
            "EGL_CONFIG_ID",
            "EGL_LEVEL",
            "EGL_MAX_PBUFFER_HEIGHT",
            "EGL_MAX_PBUFFER_PIXELS",
            "EGL_MAX_PBUFFER_WIDTH",
            "EGL_NATIVE_RENDERABLE",
            "EGL_NATIVE_VISUAL_ID",
            "EGL_NATIVE_VISUAL_TYPE",
            "EGL_PRESERVED_RESOURCES",
            "EGL_SAMPLES",
            "EGL_SAMPLE_BUFFERS",
            "EGL_SURFACE_TYPE",
            "EGL_TRANSPARENT_TYPE",
            "EGL_TRANSPARENT_RED_VALUE",
            "EGL_TRANSPARENT_GREEN_VALUE",
            "EGL_TRANSPARENT_BLUE_VALUE",
            "EGL_BIND_TO_TEXTURE_RGB",
            "EGL_BIND_TO_TEXTURE_RGBA",
            "EGL_MIN_SWAP_INTERVAL",
            "EGL_MAX_SWAP_INTERVAL",
            "EGL_LUMINANCE_SIZE",
            "EGL_ALPHA_MASK_SIZE",
            "EGL_COLOR_BUFFER_TYPE",
            "EGL_RENDERABLE_TYPE",
            "EGL_CONFORMANT"
        };
        int[] value = new int[1];
        for (int i = 0; i < attributes.length; i++) {
            int attribute = attributes[i];
            String name = names[i];
            if ( egl.eglGetConfigAttrib(display, config, attribute, value)) {
                Log.w("GeckoAppJava", String.format("  %s: %d\n", name, value[0]));
            } else {
                Log.w("GeckoAppJava", String.format("  %s: failed\n", name));
                
            }
        }
    }

    public void startEgl() {
        if (mEgl != null)
            return;

        mEgl = (EGL10) EGLContext.getEGL();
        mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        
        int[] version = new int[2];
        mEgl.eglInitialize(mEglDisplay, version);

        
        if (false) {
            int[] cs = { EGL10.EGL_NONE };
            int[] ptrnum = new int[1];

            mEgl.eglChooseConfig(mEglDisplay, cs, null, 0, ptrnum);

            int num = ptrnum[0];
            EGLConfig[] confs = new EGLConfig[num];

            mEgl.eglChooseConfig(mEglDisplay, cs, confs, num, ptrnum);

            for (int i = 0; i < num; ++i) {
                Log.w("GeckoAppJava", "===  EGL config " + i + " ===");
                printConfig(mEgl, mEglDisplay, confs[i]);
            }
        }
    }

    public void finishEgl() {
        if (mEglDisplay != null) {
            mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE,
                                EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
        }

        if (mEglContext != null) {
            mEgl.eglDestroyContext(mEglDisplay, mEglContext);
            mEglContext = null;
        }

        if (mEglSurface != null) {
            mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = null;
        }

        if (mEglDisplay != null) {
            mEgl.eglTerminate(mEglDisplay);
            mEglDisplay = null;
        }

        mEglConfig = null;
        mEgl = null;

        mSurfaceChanged = false;
    }

    public void chooseEglConfig() {
        int redBits, greenBits, blueBits, alphaBits;

        
        
        
        
        
        
        

        Log.i("GeckoAppJava", "GeckoView PixelFormat format is " + mFormat);
        if (mFormat == PixelFormat.RGB_565) {
            redBits = 5;
            greenBits = 6;
            blueBits = 5;
            alphaBits = 0;
        } else if (mFormat == PixelFormat.RGB_888 ||
                   mFormat == PixelFormat.RGBX_8888)
        {
            redBits = 8;
            greenBits = 8;
            blueBits = 8;
            alphaBits = 0;
        } else if (mFormat == PixelFormat.RGBA_8888) {
            redBits = 8;
            greenBits = 8;
            blueBits = 8;
            alphaBits = 8;
        } else {
            Log.w("GeckoAppJava", "Unknown PixelFormat for surface (format is " + mFormat + "), assuming 5650!");
            redBits = 5;
            greenBits = 6;
            blueBits = 5;
            alphaBits = 0;
        }

        
        
        
        
        
        

        int[] confSpec = new int[] {
            
            EGL10.EGL_DEPTH_SIZE, 24,
            EGL10.EGL_RED_SIZE, redBits,
            EGL10.EGL_GREEN_SIZE, greenBits,
            EGL10.EGL_BLUE_SIZE, blueBits,
            EGL10.EGL_ALPHA_SIZE, alphaBits,
            EGL10.EGL_STENCIL_SIZE, 0,
            EGL10.EGL_CONFIG_CAVEAT, EGL10.EGL_NONE,
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_NONE };

        
        int[] ptrNumConfigs = new int[1];
        mEgl.eglChooseConfig(mEglDisplay, confSpec, null, 0, ptrNumConfigs);

        int numConfigs = ptrNumConfigs[0];

        if (numConfigs == 0) {
            
            confSpec[1] = 0;
            Log.i("GeckoAppJava", "Couldn't find any valid EGL configs with 24 bit depth, trying 0.");

            mEgl.eglChooseConfig(mEglDisplay, confSpec, null, 0, ptrNumConfigs);
            numConfigs = ptrNumConfigs[0];
        }

        if (numConfigs <= 0) {
            
            Log.w("GeckoAppJava", "Couldn't find any valid EGL configs, blindly trying them all!");

            int[] fallbackConfSpec = new int[] {
                EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL10.EGL_NONE };
            confSpec = fallbackConfSpec;

            mEgl.eglChooseConfig(mEglDisplay, confSpec, null, 0, ptrNumConfigs);
            numConfigs = ptrNumConfigs[0];

            if (numConfigs == 0) {
                Log.e("GeckoAppJava", "There aren't any EGL configs available on this system.");
                finishEgl();
                mSurfaceValid = false;
                return;
            }
        }

        EGLConfig[] configs = new EGLConfig[numConfigs];
        mEgl.eglChooseConfig(mEglDisplay, confSpec, configs, numConfigs, ptrNumConfigs);

        
        
        int[] ptrVal = new int[1];
        for (int i = 0; i < configs.length; ++i) {
            int confRed = -1, confGreen = -1, confBlue = -1, confAlpha = -1;

            if (mEgl.eglGetConfigAttrib(mEglDisplay, configs[i], EGL10.EGL_RED_SIZE, ptrVal))
                confRed = ptrVal[0];
            if (mEgl.eglGetConfigAttrib(mEglDisplay, configs[i], EGL10.EGL_GREEN_SIZE, ptrVal))
                confGreen = ptrVal[0];
            if (mEgl.eglGetConfigAttrib(mEglDisplay, configs[i], EGL10.EGL_BLUE_SIZE, ptrVal))
                confBlue = ptrVal[0];
            if (mEgl.eglGetConfigAttrib(mEglDisplay, configs[i], EGL10.EGL_ALPHA_SIZE, ptrVal))
                confAlpha = ptrVal[0];

            if (confRed == redBits &&
                confGreen == greenBits &&
                confBlue == blueBits &&
                confAlpha == alphaBits)
            {
                mEglConfig = configs[i];
                break;
            }
        }

        if (mEglConfig == null) {
            Log.w("GeckoAppJava", "Couldn't find EGL config matching colors; using first, hope it works!");
            mEglConfig = configs[0];
        }

        Log.i("GeckoAppJava", "====== Chosen config: ======");
        printConfig(mEgl, mEglDisplay, mEglConfig);

        mEglContext = null;
        mEglSurface = null;
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

            if (!GeckoApp.useSoftwareDrawing) {
                chooseEglConfig();
            }

            
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

        



        if (mEgl == null || mEglDisplay == null || mEglConfig == null) {
            Log.e("GeckoAppJava", "beginDrawing called, but EGL was never initialized!");

            mSurfaceLock.unlock();
            return DRAW_ERROR;
        }

        



        if (mEglSurface == null ||
            mWidth != mBufferWidth ||
            mHeight != mBufferHeight ||
            mSurfaceChanged)
        {
            if (mEglContext != null) {
                mEgl.eglDestroyContext(mEglDisplay, mEglContext);
                mEglContext = null;
            }

            if (mEglSurface != null)
                mEgl.eglDestroySurface(mEglDisplay, mEglSurface);

            mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig, getHolder(), null);
            if (mEglSurface == EGL10.EGL_NO_SURFACE)
            {
                Log.e("GeckoAppJava", "eglCreateWindowSurface failed!");
                mSurfaceValid = false;
                return DRAW_ERROR;
            }

            mBufferWidth = mWidth;
            mBufferHeight = mHeight;

            mSurfaceChanged = false;
        }

        if (mEglContext == null) {
            int[] attrib_list = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                  EGL10.EGL_NONE };
            mEglContext = mEgl.eglCreateContext(mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
            if (mEglContext == EGL10.EGL_NO_CONTEXT)
            {
                Log.e("GeckoAppJava", "eglCreateContext failed! " + mEgl.eglGetError());
                mSurfaceValid = false;
                return DRAW_ERROR;
            }

            Log.i("GeckoAppJava", "EglContext created");
        }

        
        
        

        if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
            int err = mEgl.eglGetError();
            Log.e("GeckoAppJava", "eglMakeCurrent failed! " + err);
            return DRAW_ERROR;
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
            } else {
                
                
                
                
                
                if (!mSurfaceChanged) {
                    mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
                    if (mEgl.eglGetError() == EGL11.EGL_CONTEXT_LOST)
                        mSurfaceChanged = true;
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

    
    EGL10 mEgl;
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLConfig mEglConfig;
    EGLContext mEglContext;
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
