









package org.webrtc.videoengine;

import java.util.List;

import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.hardware.Camera;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

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

  
  
  
  
  
  private static String getDeviceInfo() {
    try {
      JSONArray devices = new JSONArray();
      for (int i = 0; i < Camera.getNumberOfCameras(); ++i) {
        CameraInfo info = new CameraInfo();
        Camera.getCameraInfo(i, info);
        String uniqueName = deviceUniqueName(i, info);
        JSONObject cameraDict = new JSONObject();
        devices.put(cameraDict);
        List<Size> supportedSizes;
        List<int[]> supportedFpsRanges;
        Camera camera = null;
        try {
          camera = Camera.open(i);
          Parameters parameters = camera.getParameters();
          supportedSizes = parameters.getSupportedPreviewSizes();
          supportedFpsRanges = parameters.getSupportedPreviewFpsRange();
          Log.d(TAG, uniqueName);
        } catch (RuntimeException e) {
          Log.e(TAG, "Failed to open " + uniqueName + ", skipping", e);
          continue;
        } finally {
          if (camera != null) {
            camera.release();
          }
        }

        JSONArray sizes = new JSONArray();
        for (Size supportedSize : supportedSizes) {
          JSONObject size = new JSONObject();
          size.put("width", supportedSize.width);
          size.put("height", supportedSize.height);
          sizes.put(size);
        }

        boolean is30fpsRange = false;
        boolean is15fpsRange = false;
        
        
        
        for (int[] range : supportedFpsRanges) {
          if (range[Parameters.PREVIEW_FPS_MIN_INDEX] == 30000 &&
              range[Parameters.PREVIEW_FPS_MAX_INDEX] == 30000) {
            is30fpsRange = true;
          }
          if (range[Parameters.PREVIEW_FPS_MIN_INDEX] == 15000 &&
              range[Parameters.PREVIEW_FPS_MAX_INDEX] == 15000) {
            is15fpsRange = true;
          }
        }
        if (is30fpsRange && !is15fpsRange) {
          Log.d(TAG, "Adding 15 fps support");
          int[] newRange = new int [Parameters.PREVIEW_FPS_MAX_INDEX + 1];
          newRange[Parameters.PREVIEW_FPS_MIN_INDEX] = 15000;
          newRange[Parameters.PREVIEW_FPS_MAX_INDEX] = 15000;
          for (int j = 0; j < supportedFpsRanges.size(); j++ ) {
            int[] range = supportedFpsRanges.get(j);
            if (range[Parameters.PREVIEW_FPS_MAX_INDEX] >
                newRange[Parameters.PREVIEW_FPS_MAX_INDEX]) {
              supportedFpsRanges.add(j, newRange);
              break;
            }
          }
        }

        JSONArray mfpsRanges = new JSONArray();
        for (int[] range : supportedFpsRanges) {
          JSONObject mfpsRange = new JSONObject();
          
          
          
          mfpsRange.put("min_mfps", range[Parameters.PREVIEW_FPS_MIN_INDEX]);
          mfpsRange.put("max_mfps", range[Parameters.PREVIEW_FPS_MAX_INDEX]);
          mfpsRanges.put(mfpsRange);
        }

        cameraDict.put("name", uniqueName);
        cameraDict.put("front_facing", isFrontFacing(info))
            .put("orientation", info.orientation)
            .put("sizes", sizes)
            .put("mfpsRanges", mfpsRanges);
      }
      String ret = devices.toString(2);
      Log.d(TAG, ret);
      return ret;
    } catch (JSONException e) {
      throw new RuntimeException(e);
    }
  }
}
