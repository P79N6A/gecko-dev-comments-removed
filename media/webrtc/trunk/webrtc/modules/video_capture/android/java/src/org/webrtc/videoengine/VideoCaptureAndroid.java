









package org.webrtc.videoengine;

import java.io.IOException;
import java.util.Locale;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoAppShell.AppStateListener;








public class VideoCaptureAndroid implements PreviewCallback, Callback {
  private final static String TAG = "WEBRTC-JC";

  private Camera camera;  
  private Camera.CameraInfo info = null;
  private final int id;
  private final long native_capturer;  
  private SurfaceHolder localPreview;
  private SurfaceTexture dummySurfaceTexture;
  
  
  
  private final int numCaptureBuffers = 3;
  
  private AppStateListener mAppStateListener = null;
  private int mCaptureRotation = 0;
  private int mCaptureWidth = 0;
  private int mCaptureHeight = 0;
  private int mCaptureMinFPS = 0;
  private int mCaptureMaxFPS = 0;
  
  
  private boolean mResumeCapture = false;

  public VideoCaptureAndroid(int id, long native_capturer) {
    this.id = id;
    this.native_capturer = native_capturer;
    if(android.os.Build.VERSION.SDK_INT>8) {
      this.info = new Camera.CameraInfo();
      Camera.getCameraInfo(id, info);
    }
    mCaptureRotation = GetRotateAmount();
  }

  private void LinkAppStateListener() {
    mAppStateListener = new AppStateListener() {
      @Override
      public void onPause() {
        if (camera != null) {
          mResumeCapture = true;
          stopCapture();
        }
      }
      @Override
      public void onResume() {
        if (mResumeCapture) {
          startCapture(mCaptureWidth, mCaptureHeight, mCaptureMinFPS, mCaptureMaxFPS);
          mResumeCapture = false;
        }
      }
      @Override
      public void onOrientationChanged() {
        mCaptureRotation = GetRotateAmount();
      }
    };
    GeckoAppShell.getGeckoInterface().addAppStateListener(mAppStateListener);
  }

  private void RemoveAppStateListener() {
      GeckoAppShell.getGeckoInterface().removeAppStateListener(mAppStateListener);
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

  
  
  
  
  
  private synchronized boolean startCapture(
      int width, int height, int min_mfps, int max_mfps) {
    Log.d(TAG, "startCapture: " + width + "x" + height + "@" +
        min_mfps + ":" + max_mfps);
    if (!mResumeCapture) {
      ViERenderer.CreateLocalRenderer();
    }
    Throwable error = null;
    try {
      camera = Camera.open(id);

      localPreview = ViERenderer.GetLocalRenderer();
      if (localPreview != null) {
        localPreview.addCallback(this);
        if (localPreview.getSurface() != null &&
            localPreview.getSurface().isValid()) {
          camera.setPreviewDisplay(localPreview);
        }
      } else {
        
        
        
        
        
        try {
          
          dummySurfaceTexture = new SurfaceTexture(42);
          camera.setPreviewTexture(dummySurfaceTexture);
        } catch (IOException e) {
          throw new RuntimeException(e);
        }
      }

      Camera.Parameters parameters = camera.getParameters();
      Log.d(TAG, "isVideoStabilizationSupported: " +
          parameters.isVideoStabilizationSupported());
      if (parameters.isVideoStabilizationSupported()) {
        parameters.setVideoStabilization(true);
      }
      parameters.setPreviewSize(width, height);
      parameters.setPreviewFpsRange(min_mfps, max_mfps);
      int format = ImageFormat.NV21;
      parameters.setPreviewFormat(format);
      camera.setParameters(parameters);
      int bufSize = width * height * ImageFormat.getBitsPerPixel(format) / 8;
      for (int i = 0; i < numCaptureBuffers; i++) {
        camera.addCallbackBuffer(new byte[bufSize]);
      }
      camera.setPreviewCallbackWithBuffer(this);
      camera.startPreview();
      
      mCaptureWidth = width;
      mCaptureHeight = height;
      mCaptureMinFPS = min_mfps;
      mCaptureMaxFPS = max_mfps;
      
      if (!mResumeCapture) {
        LinkAppStateListener();
      }
      return true;
    } catch (IOException e) {
      error = e;
    } catch (RuntimeException e) {
      error = e;
    }
    Log.e(TAG, "startCapture failed", error);
    if (camera != null) {
      stopCapture();
    }
    return false;
  }

  
  private synchronized boolean stopCapture() {
    Log.d(TAG, "stopCapture");
    if (camera == null) {
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
        camera.setPreviewTexture(null);
      }
      camera.release();
      camera = null;
      
      if (!mResumeCapture) {
        RemoveAppStateListener();
        ViERenderer.DestroyLocalRenderer();
      }
      return true;
    } catch (IOException e) {
      error = e;
    } catch (RuntimeException e) {
      error = e;
    }
    Log.e(TAG, "Failed to stop camera", error);
    return false;
  }

  private native void ProvideCameraFrame(
    byte[] data, int length, long captureObject, int rotation);

  public synchronized void onPreviewFrame(byte[] data, Camera camera) {
    if (data != null) {
      ProvideCameraFrame(data, data.length, native_capturer, mCaptureRotation);
      camera.addCallbackBuffer(data);
    }
  }

  public synchronized void surfaceChanged(
      SurfaceHolder holder, int format, int width, int height) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceChanged ignored: " +
        format + ": " + width + "x" + height);
  }

  public synchronized void surfaceCreated(SurfaceHolder holder) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceCreated");
    try {
      if (camera != null) {
        camera.setPreviewDisplay(holder);
      }
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  public synchronized void surfaceDestroyed(SurfaceHolder holder) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceDestroyed");
    try {
      if (camera != null) {
        camera.setPreviewDisplay(null);
      }
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
