









package org.webrtc.videoengine;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.locks.ReentrantLock;

import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;








public class VideoCaptureAndroid implements PreviewCallback, Callback {
  private final static String TAG = "WEBRTC-JC";

  private Camera camera;  
  private final int id;
  private final Camera.CameraInfo info;
  private final long native_capturer;  
  private SurfaceHolder localPreview;
  private SurfaceTexture dummySurfaceTexture;
  
  
  
  private final int numCaptureBuffers = 3;

  public VideoCaptureAndroid(int id, long native_capturer) {
    this.id = id;
    this.native_capturer = native_capturer;
    this.info = new Camera.CameraInfo();
    Camera.getCameraInfo(id, info);
  }

  
  
  
  
  
  private synchronized boolean startCapture(
      int width, int height, int min_mfps, int max_mfps) {
    Log.d(TAG, "startCapture: " + width + "x" + height + "@" +
        min_mfps + ":" + max_mfps);
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
      camera.stopPreview();
      camera.setPreviewCallbackWithBuffer(null);
      if (localPreview != null) {
        localPreview.removeCallback(this);
        camera.setPreviewDisplay(null);
      } else {
        camera.setPreviewTexture(null);
      }
      camera.release();
      camera = null;
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
      byte[] data, int length, long captureObject);

  public synchronized void onPreviewFrame(byte[] data, Camera camera) {
    ProvideCameraFrame(data, data.length, native_capturer);
    camera.addCallbackBuffer(data);
  }

  
  
  
  private synchronized void setPreviewRotation(int rotation) {
    Log.v(TAG, "setPreviewRotation:" + rotation);

    if (camera == null) {
      return;
    }

    int resultRotation = 0;
    if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
      
      
      resultRotation = ( 360 - rotation ) % 360; 
    } else {
      
      resultRotation = rotation;
    }
    camera.setDisplayOrientation(resultRotation);
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
