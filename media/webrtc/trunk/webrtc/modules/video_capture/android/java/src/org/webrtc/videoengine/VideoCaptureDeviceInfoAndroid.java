









package org.webrtc.videoengine;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import android.content.Context;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.hardware.Camera;
import android.util.Log;

import org.mozilla.gecko.mozglue.WebRTCJNITarget;

public class VideoCaptureDeviceInfoAndroid {
  private final static String TAG = "WEBRTC-JC";

  private static boolean isFrontFacing(CameraInfo info) {
    return info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT;
  }

  private static String deviceUniqueName(int index, CameraInfo info) {
    return "Camera " + index +", Facing " +
        (isFrontFacing(info) ? "front" : "back") +
        ", Orientation "+ info.orientation;
  }

  
  
  
  
  @WebRTCJNITarget
  private static CaptureCapabilityAndroid[] getDeviceInfo() {
      ArrayList<CaptureCapabilityAndroid> allDevices = new ArrayList<CaptureCapabilityAndroid>();
      int numCameras = 1;
      if (android.os.Build.VERSION.SDK_INT >= 9) {
          numCameras = Camera.getNumberOfCameras();
      }
      for (int i = 0; i < numCameras; ++i) {
          String uniqueName = null;
          CameraInfo info = null;
          if (android.os.Build.VERSION.SDK_INT >= 9) {
              info = new CameraInfo();
              Camera.getCameraInfo(i, info);
              uniqueName = deviceUniqueName(i, info);
          } else {
              uniqueName = "Camera 0, Facing back, Orientation 90";
          }

          List<Size> supportedSizes = null;
          List<int[]> supportedFpsRanges = null;
          try {
              Camera camera = null;
              if (android.os.Build.VERSION.SDK_INT >= 9) {
                  camera = Camera.open(i);
              } else {
                  camera = Camera.open();
              }
              Parameters parameters = camera.getParameters();
              supportedSizes = parameters.getSupportedPreviewSizes();
              if (android.os.Build.VERSION.SDK_INT >= 9) {
                  supportedFpsRanges = parameters.getSupportedPreviewFpsRange();
              }
              
              
              if (supportedFpsRanges == null) {
                  supportedFpsRanges = new ArrayList<int[]>();
                  List<Integer> frameRates = parameters.getSupportedPreviewFrameRates();
                  if (frameRates != null) {
                      for (Integer rate: frameRates) {
                          int[] range = new int[2];
                          
                          range[0] = rate * 1000;
                          range[1] = rate * 1000;
                          supportedFpsRanges.add(range);
                      }
                  } else {
                      Log.e(TAG, "Camera doesn't know its own framerate, guessing 25fps.");
                      int[] range = new int[2];
                      
                      range[0] = 25 * 1000;
                      range[1] = 25 * 1000;
                      supportedFpsRanges.add(range);
                  }
              }
              camera.release();
              Log.d(TAG, uniqueName);
          } catch (RuntimeException e) {
              Log.e(TAG, "Failed to open " + uniqueName + ", skipping due to: "
                    + e.getLocalizedMessage());
              continue;
          }

          CaptureCapabilityAndroid device = new CaptureCapabilityAndroid();

          int sizeLen = supportedSizes.size();
          device.width  = new int[sizeLen];
          device.height = new int[sizeLen];

          int j = 0;
          for (Size size : supportedSizes) {
                device.width[j] = size.width;
                device.height[j] = size.height;
                j++;
          }

          
          
          
          int[] mfps = supportedFpsRanges.get(supportedFpsRanges.size() - 1);
          device.name = uniqueName;
          if (android.os.Build.VERSION.SDK_INT >= 9) {
              device.frontFacing = isFrontFacing(info);
              device.orientation = info.orientation;
              device.minMilliFPS = mfps[Parameters.PREVIEW_FPS_MIN_INDEX];
              device.maxMilliFPS = mfps[Parameters.PREVIEW_FPS_MAX_INDEX];
          } else {
              device.frontFacing = false;
              device.orientation = 90;
              device.minMilliFPS = mfps[0];
              device.maxMilliFPS = mfps[1];
          }
          allDevices.add(device);
      }
      return allDevices.toArray(new CaptureCapabilityAndroid[0]);
  }
}
