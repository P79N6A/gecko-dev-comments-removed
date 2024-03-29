









package org.webrtc.videoengine;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Exchanger;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceHolder;
import android.view.WindowManager;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoAppShell.AppStateListener;
import org.mozilla.gecko.mozglue.WebRTCJNITarget;










public class VideoCaptureAndroid implements PreviewCallback, Callback, AppStateListener {
  private final static String TAG = "WEBRTC-JC";

  private static SurfaceHolder localPreview;
  
  Camera camera;
  private Camera.CameraInfo info;
  private CameraThread cameraThread;
  private Handler cameraThreadHandler;
  private Context context;
  private final int id;
  private final long native_capturer;  
  private SurfaceTexture cameraSurfaceTexture;
  private int[] cameraGlTextures = null;

  
  
  
  private final int numCaptureBuffers = 3;

  
  volatile int mCaptureRotation;
  int mCaptureWidth;
  int mCaptureHeight;
  int mCaptureMinFPS;
  int mCaptureMaxFPS;
  
  
  boolean mResumeCapture;

  private double averageDurationMs;
  private long lastCaptureTimeMs;
  private int frameCount;
  private int frameDropRatio;

  
  public static void setLocalPreview(SurfaceHolder localPreview) {
    
    
    
    VideoCaptureAndroid.localPreview = localPreview;
  }

  @WebRTCJNITarget
  public VideoCaptureAndroid(int id, long native_capturer) {
    this.id = id;
    this.native_capturer = native_capturer;
    this.context = GetContext();
    if(android.os.Build.VERSION.SDK_INT>8) {
      this.info = new Camera.CameraInfo();
      Camera.getCameraInfo(id, info);
    }
    mCaptureRotation = GetRotateAmount();
  }

  @Override
  public synchronized void onPause() {
    if (camera != null) {
      mResumeCapture = true;
      stopCapture();
    }
  }

  @Override
  public synchronized void onResume() {
    if (mResumeCapture) {
      startCapture(mCaptureWidth, mCaptureHeight, mCaptureMinFPS, mCaptureMaxFPS);
      mResumeCapture = false;
    }
  }

  @Override
  public void onOrientationChanged() {
    mCaptureRotation = GetRotateAmount();
  }

  public int GetRotateAmount() {
    int rotation = GeckoAppShell.getGeckoInterface().getActivity().getWindowManager().getDefaultDisplay().getRotation();
    int degrees = 0;
    switch (rotation) {
      case Surface.ROTATION_0: degrees = 0; break;
      case Surface.ROTATION_90: degrees = 90; break;
      case Surface.ROTATION_180: degrees = 180; break;
      case Surface.ROTATION_270: degrees = 270; break;
    }
    if(android.os.Build.VERSION.SDK_INT>8) {
      int result;
      if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
        result = (info.orientation + degrees) % 360;
      } else {  
        result = (info.orientation - degrees + 360) % 360;
      }
      return result;
    } else {
      
      
      int orientation = 90;
      int result = (orientation - degrees + 360) % 360;
      return result;
    }
  }

  
  private static native Context GetContext();

  private class CameraThread extends Thread {
    private Exchanger<Handler> handlerExchanger;
    public CameraThread(Exchanger<Handler> handlerExchanger) {
      this.handlerExchanger = handlerExchanger;
    }

    @Override public void run() {
      Looper.prepare();
      exchange(handlerExchanger, new Handler());
      Looper.loop();
    }
  }

  
  
  
  
  
  @WebRTCJNITarget
  private synchronized boolean startCapture(
      final int width, final int height,
      final int min_mfps, final int max_mfps) {
    Log.d(TAG, "startCapture: " + width + "x" + height + "@" +
        min_mfps + ":" + max_mfps);
    if (cameraThread != null || cameraThreadHandler != null) {
      throw new RuntimeException("Camera thread already started!");
    }
    Exchanger<Handler> handlerExchanger = new Exchanger<Handler>();
    cameraThread = new CameraThread(handlerExchanger);
    cameraThread.start();
    cameraThreadHandler = exchange(handlerExchanger, null);

    final Exchanger<Boolean> result = new Exchanger<Boolean>();
    cameraThreadHandler.post(new Runnable() {
        @Override public void run() {
          startCaptureOnCameraThread(width, height, min_mfps, max_mfps, result);
        }
      });
    boolean startResult = exchange(result, false); 
    return startResult;
  }

  private void startCaptureOnCameraThread(
      int width, int height, int min_mfps, int max_mfps,
      Exchanger<Boolean> result) {
    if (!mResumeCapture) {
      ViERenderer.CreateLocalRenderer();
    }
    Throwable error = null;
    try {
      if(android.os.Build.VERSION.SDK_INT>8) {
        camera = Camera.open(id);
      } else {
        camera = Camera.open();
      }

      localPreview = ViERenderer.GetLocalRenderer();
      if (localPreview != null) {
        localPreview.addCallback(this);
        if (localPreview.getSurface() != null &&
            localPreview.getSurface().isValid()) {
          camera.setPreviewDisplay(localPreview);
        }
      } else {
        if(android.os.Build.VERSION.SDK_INT>10) {
          
          
          
          
          
          try {
            cameraGlTextures = new int[1];

            
            GLES20.glGenTextures(1, cameraGlTextures, 0);
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                                 cameraGlTextures[0]);
            GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                                   GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                                   GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                                   GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
                                   GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

            cameraSurfaceTexture = new SurfaceTexture(cameraGlTextures[0]);
            cameraSurfaceTexture.setOnFrameAvailableListener(null);
            camera.setPreviewTexture(cameraSurfaceTexture);
          } catch (IOException e) {
            throw new RuntimeException(e);
          }
        } else {
          throw new RuntimeException("No preview surface for Camera.");
        }
      }

      Log.d(TAG, "Camera orientation: " + info.orientation +
          ". Device orientation: " + getDeviceOrientation());
      Camera.Parameters parameters = camera.getParameters();
      
      if(android.os.Build.VERSION.SDK_INT>14) {
        Log.d(TAG, "isVideoStabilizationSupported: " +
              parameters.isVideoStabilizationSupported());
        if (parameters.isVideoStabilizationSupported()) {
          parameters.setVideoStabilization(true);
        }
      }
      List<String> focusModeList = parameters.getSupportedFocusModes();
      
      if (focusModeList != null) {
        if (focusModeList.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
        }
      }
      parameters.setPreviewSize(width, height);

      
      
      List<int[]> supportedFpsRanges = parameters.getSupportedPreviewFpsRange();
      frameDropRatio = Integer.MAX_VALUE;
      for (int i = 0; i < supportedFpsRanges.size(); i++) {
        int[] range = supportedFpsRanges.get(i);
        if (range[Parameters.PREVIEW_FPS_MIN_INDEX] == min_mfps &&
            range[Parameters.PREVIEW_FPS_MAX_INDEX] == max_mfps) {
          frameDropRatio = 1;
          break;
        }
        if (range[Parameters.PREVIEW_FPS_MIN_INDEX] % min_mfps == 0 &&
            range[Parameters.PREVIEW_FPS_MAX_INDEX] % max_mfps == 0) {
          int dropRatio = range[Parameters.PREVIEW_FPS_MAX_INDEX] / max_mfps;
          frameDropRatio = Math.min(dropRatio, frameDropRatio);
        }
      }
      if (frameDropRatio == Integer.MAX_VALUE) {
        Log.e(TAG, "Can not find camera fps range");
        error = new RuntimeException("Can not find camera fps range");
        exchange(result, false);
        return;
      }
      if (frameDropRatio > 1) {
        Log.d(TAG, "Frame dropper is enabled. Ratio: " + frameDropRatio);
      }
      min_mfps *= frameDropRatio;
      max_mfps *= frameDropRatio;
      Log.d(TAG, "Camera preview mfps range: " + min_mfps + " - " + max_mfps);
      if (android.os.Build.VERSION.SDK_INT>8) {
          parameters.setPreviewFpsRange(min_mfps, max_mfps);
      } else {
          parameters.setPreviewFrameRate(max_mfps / 1000);
      }

      int format = ImageFormat.NV21;
      parameters.setPreviewFormat(format);
      camera.setParameters(parameters);
      try {
          
          parameters.setPictureSize(width, height);
          camera.setParameters(parameters);
      } catch(RuntimeException e) {
          Log.d(TAG, "Failed to apply Nexus 7 workaround");
      }

      int bufSize = width * height * ImageFormat.getBitsPerPixel(format) / 8;
      for (int i = 0; i < numCaptureBuffers; i++) {
        camera.addCallbackBuffer(new byte[bufSize]);
      }
      camera.setPreviewCallbackWithBuffer(this);
      frameCount = 0;
      averageDurationMs = 1000000.0f / (max_mfps / frameDropRatio);
      camera.startPreview();
      
      mCaptureWidth = width;
      mCaptureHeight = height;
      mCaptureMinFPS = min_mfps;
      mCaptureMaxFPS = max_mfps;
      
      if (!mResumeCapture) {
        GeckoAppShell.getGeckoInterface().addAppStateListener(this);
      }
      exchange(result, true);
      return;
    } catch (IOException e) {
      error = e;
    } catch (RuntimeException e) {
      error = e;
    }
    Log.e(TAG, "startCapture failed", error);
    if (camera != null) {
      Exchanger<Boolean> resultDropper = new Exchanger<Boolean>();
      stopCaptureOnCameraThread(resultDropper);
      exchange(resultDropper, false);
    }
    exchange(result, false);
    return;
  }

  
  @WebRTCJNITarget
  private synchronized boolean stopCapture() {
    Log.d(TAG, "stopCapture");
    
    if (cameraThreadHandler == null) {
      return true;
    }
    final Exchanger<Boolean> result = new Exchanger<Boolean>();
    cameraThreadHandler.post(new Runnable() {
        @Override public void run() {
          stopCaptureOnCameraThread(result);
        }
      });
    boolean status = exchange(result, false);  
    try {
      cameraThread.join();
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }
    cameraThreadHandler = null;
    cameraThread = null;
    Log.d(TAG, "stopCapture done");
    return status;
  }

  private void stopCaptureOnCameraThread(
      Exchanger<Boolean> result) {
    if (camera == null) {
      if (mResumeCapture == true) {
        
        
        mResumeCapture = false;
        return;
      }
      throw new RuntimeException("Camera is already stopped!");
    }
    Throwable error = null;
    try {
      camera.setPreviewCallbackWithBuffer(null);
      camera.stopPreview();
      if (localPreview != null) {
        localPreview.removeCallback(this);
        camera.setPreviewDisplay(null);
      } else {
        if(android.os.Build.VERSION.SDK_INT>10) {
          camera.setPreviewTexture(null);
          cameraSurfaceTexture = null;
          if (cameraGlTextures != null) {
            GLES20.glDeleteTextures(1, cameraGlTextures, 0);
            cameraGlTextures = null;
          }
        }
      }
      camera.release();
      camera = null;
      
      if (!mResumeCapture) {
        GeckoAppShell.getGeckoInterface().removeAppStateListener(this);
        ViERenderer.DestroyLocalRenderer();
      }
      exchange(result, true);
      Looper.myLooper().quit();
      return;
    } catch (IOException e) {
      error = e;
    } catch (RuntimeException e) {
      error = e;
    }
    Log.e(TAG, "Failed to stop camera", error);
    exchange(result, false);
    Looper.myLooper().quit();
    return;
  }

  @WebRTCJNITarget
  private int getDeviceOrientation() {
    int orientation = 0;
    if (context != null) {
      WindowManager wm = (WindowManager) context.getSystemService(
          Context.WINDOW_SERVICE);
      switch(wm.getDefaultDisplay().getRotation()) {
        case Surface.ROTATION_90:
          orientation = 90;
          break;
        case Surface.ROTATION_180:
          orientation = 180;
          break;
        case Surface.ROTATION_270:
          orientation = 270;
          break;
        case Surface.ROTATION_0:
        default:
          orientation = 0;
          break;
      }
    }
    return orientation;
  }

  @WebRTCJNITarget
  private native void ProvideCameraFrame(
      byte[] data, int length, int rotation, long timeStamp, long captureObject);

  
  @WebRTCJNITarget
  @Override
  public void onPreviewFrame(byte[] data, Camera callbackCamera) {
    if (Thread.currentThread() != cameraThread) {
      throw new RuntimeException("Camera callback not on camera thread?!?");
    }
    if (camera == null) {
      return;
    }
    if (camera != callbackCamera) {
      throw new RuntimeException("Unexpected camera in callback!");
    }
    frameCount++;
    
    if ((frameDropRatio > 1) && (frameCount % frameDropRatio) > 0) {
      camera.addCallbackBuffer(data);
      return;
    }
    long captureTimeMs = SystemClock.elapsedRealtime();
    if (frameCount > frameDropRatio) {
      double durationMs = captureTimeMs - lastCaptureTimeMs;
      averageDurationMs = 0.9 * averageDurationMs + 0.1 * durationMs;
      if ((frameCount % 30) == 0) {
        Log.d(TAG, "Camera TS " + captureTimeMs +
            ". Duration: " + (int)durationMs + " ms. FPS: " +
            (int) (1000 / averageDurationMs + 0.5));
      }
    }
    lastCaptureTimeMs = captureTimeMs;

    int rotation = getDeviceOrientation();
    if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
      rotation = 360 - rotation;
    }
    rotation = (info.orientation + rotation) % 360;

    if (data != null) {
      ProvideCameraFrame(data, data.length, mCaptureRotation, lastCaptureTimeMs, native_capturer);
      camera.addCallbackBuffer(data);
    }
  }

  @WebRTCJNITarget
  @Override
  public synchronized void surfaceChanged(
      SurfaceHolder holder, int format, int width, int height) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceChanged ignored: " +
        format + ": " + width + "x" + height);
  }

  @WebRTCJNITarget
  @Override
  public synchronized void surfaceCreated(final SurfaceHolder holder) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceCreated");
    if (camera == null || cameraThreadHandler == null) {
      return;
    }
    final Exchanger<IOException> result = new Exchanger<IOException>();
    cameraThreadHandler.post(new Runnable() {
        @Override public void run() {
          setPreviewDisplayOnCameraThread(holder, result);
        }
      });
    IOException e = exchange(result, null);  
    if (e != null) {
      throw new RuntimeException(e);
    }
  }

  @WebRTCJNITarget
  @Override
  public synchronized void surfaceDestroyed(SurfaceHolder holder) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceDestroyed");
    if (camera == null || cameraThreadHandler == null) {
      return;
    }
    final Exchanger<IOException> result = new Exchanger<IOException>();
    cameraThreadHandler.post(new Runnable() {
        @Override public void run() {
          setPreviewDisplayOnCameraThread(null, result);
        }
      });
    IOException e = exchange(result, null);  
    if (e != null) {
      throw new RuntimeException(e);
    }
  }

  private void setPreviewDisplayOnCameraThread(
      SurfaceHolder holder, Exchanger<IOException> result) {
    try {
      camera.setPreviewDisplay(holder);
    } catch (IOException e) {
      exchange(result, e);
      return;
    }
    exchange(result, null);
    return;
  }

  
  
  private static <T> T exchange(Exchanger<T> exchanger, T value) {
    try {
      return exchanger.exchange(value);
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }
  }
}
