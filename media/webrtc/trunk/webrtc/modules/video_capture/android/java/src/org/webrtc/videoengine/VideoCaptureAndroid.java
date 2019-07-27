









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
import org.mozilla.gecko.mozglue.WebRTCJNITarget;








public class VideoCaptureAndroid implements PreviewCallback, Callback, AppStateListener {
  private final static String TAG = "WEBRTC-JC";

  
  Camera camera;
  private Camera.CameraInfo info;
  private final int id;
  private final long native_capturer;  
  private SurfaceHolder localPreview;
  private SurfaceTexture dummySurfaceTexture;

  
  
  
  private final int numCaptureBuffers = 3;

  
  volatile int mCaptureRotation;
  int mCaptureWidth;
  int mCaptureHeight;
  int mCaptureMinFPS;
  int mCaptureMaxFPS;
  
  
  boolean mResumeCapture;

  @WebRTCJNITarget
  public VideoCaptureAndroid(int id, long native_capturer) {
    this.id = id;
    this.native_capturer = native_capturer;
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

  
  
  
  
  
  @WebRTCJNITarget
  private synchronized boolean startCapture(
      int width, int height, int min_mfps, int max_mfps) {
    Log.d(TAG, "startCapture: " + width + "x" + height + "@" +
        min_mfps + ":" + max_mfps);
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
            
            dummySurfaceTexture = new SurfaceTexture(42);
            camera.setPreviewTexture(dummySurfaceTexture);
          } catch (IOException e) {
            throw new RuntimeException(e);
          }
        } else {
          throw new RuntimeException("No preview surface for Camera.");
        }
      }

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
      if (android.os.Build.VERSION.SDK_INT>8) {
          parameters.setPreviewFpsRange(min_mfps, max_mfps);
      } else {
          parameters.setPreviewFrameRate(max_mfps / 1000);
      }
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
        GeckoAppShell.getGeckoInterface().addAppStateListener(this);
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

  
  @WebRTCJNITarget
  private synchronized boolean stopCapture() {
    Log.d(TAG, "stopCapture");
    if (camera == null) {
      if (mResumeCapture == true) {
        
        
        mResumeCapture = false;
        return true;
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
        }
      }
      camera.release();
      camera = null;
      
      if (!mResumeCapture) {
        GeckoAppShell.getGeckoInterface().removeAppStateListener(this);
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

  @WebRTCJNITarget
  private native void ProvideCameraFrame(
    byte[] data, int length, long captureObject, int rotation);

  @WebRTCJNITarget
  public synchronized void onPreviewFrame(byte[] data, Camera camera) {
    if (data != null) {
      ProvideCameraFrame(data, data.length, native_capturer, mCaptureRotation);
      camera.addCallbackBuffer(data);
    }
  }

  @WebRTCJNITarget
  public synchronized void surfaceChanged(
      SurfaceHolder holder, int format, int width, int height) {
    Log.d(TAG, "VideoCaptureAndroid::surfaceChanged ignored: " +
        format + ": " + width + "x" + height);
  }

  @WebRTCJNITarget
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

  @WebRTCJNITarget
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
