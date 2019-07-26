









package org.webrtc.videoengine;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import dalvik.system.DexClassLoader;

import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.Log;

public class VideoCaptureDeviceInfoAndroid {

    
    Context context;

    
    
    private final static String TAG = "WEBRTC";

    
    public class AndroidVideoCaptureDevice {
        AndroidVideoCaptureDevice() {
            frontCameraType = FrontFacingCameraType.None;
            index = 0;
        }

        public String deviceUniqueName;
        public CaptureCapabilityAndroid captureCapabilies[];
        public FrontFacingCameraType frontCameraType;

        
        
        public int orientation;
        
        public int index;
    }

    public enum FrontFacingCameraType {
        None, 
                GalaxyS, 
                HTCEvo, 
                Android23, 
                }

    String currentDeviceUniqueId;
    int id;
    List<AndroidVideoCaptureDevice> deviceList;

    public static VideoCaptureDeviceInfoAndroid
    CreateVideoCaptureDeviceInfoAndroid(int in_id, Context in_context) {
        Log.d(TAG,
                String.format(Locale.US, "VideoCaptureDeviceInfoAndroid"));

        VideoCaptureDeviceInfoAndroid self =
                new VideoCaptureDeviceInfoAndroid(in_id, in_context);
        if(self != null && self.Init() == 0) {
            return self;
        }
        else {
            Log.d(TAG, "Failed to create VideoCaptureDeviceInfoAndroid.");
        }
        return null;
    }

    private VideoCaptureDeviceInfoAndroid(int in_id,
            Context in_context) {
        id = in_id;
        context = in_context;
        deviceList = new ArrayList<AndroidVideoCaptureDevice>();
    }

    private int Init() {
        
        Camera camera = null;
        try{
            if(android.os.Build.VERSION.SDK_INT > 8) {
                
                for(int i = 0; i < Camera.getNumberOfCameras(); ++i) {
                    AndroidVideoCaptureDevice newDevice = new AndroidVideoCaptureDevice();

                    Camera.CameraInfo info = new Camera.CameraInfo();
                    Camera.getCameraInfo(i, info);
                    newDevice.index = i;
                    newDevice.orientation=info.orientation;
                    if(info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
                        newDevice.deviceUniqueName =
                                "Camera " + i +", Facing back, Orientation "+ info.orientation;
                        Log.d(TAG, "Camera " + i +", Facing back, Orientation "+ info.orientation);

                    }
                    else {
                        newDevice.deviceUniqueName =
                                "Camera " + i +", Facing front, Orientation "+ info.orientation;
                        newDevice.frontCameraType = FrontFacingCameraType.Android23;
                        Log.d(TAG, "Camera " + i +", Facing front, Orientation "+ info.orientation);
                    }

                    camera = Camera.open(i);
                    Camera.Parameters parameters = camera.getParameters();
                    AddDeviceInfo(newDevice, parameters);
                    camera.release();
                    camera = null;
                    deviceList.add(newDevice);
                }
            }
        }
        catch (Exception ex) {
            Log.e(TAG, "Failed to init VideoCaptureDeviceInfo ex" +
                    ex.getLocalizedMessage());
            return -1;
        }
        VerifyCapabilities();
        return 0;
    }

    
    private void AddDeviceInfo(AndroidVideoCaptureDevice newDevice,
            Camera.Parameters parameters) {

        List<Size> sizes = parameters.getSupportedPreviewSizes();
        List<Integer> frameRates = parameters.getSupportedPreviewFrameRates();
        int maxFPS = 0;
        for(Integer frameRate:frameRates) {
            if(frameRate > maxFPS) {
                maxFPS = frameRate;
            }
        }

        newDevice.captureCapabilies = new CaptureCapabilityAndroid[sizes.size()];
        for(int i = 0; i < sizes.size(); ++i) {
            Size s = sizes.get(i);
            newDevice.captureCapabilies[i] = new CaptureCapabilityAndroid();
            newDevice.captureCapabilies[i].height = s.height;
            newDevice.captureCapabilies[i].width = s.width;
            newDevice.captureCapabilies[i].maxFPS = maxFPS;
            Log.v(TAG,
                    "VideoCaptureDeviceInfo " + "maxFPS:" + maxFPS +
                    " width:" + s.width + " height:" + s.height);
        }
    }

    
    
    
    
    
    private void VerifyCapabilities() {
        
        if(android.os.Build.DEVICE.equals("GT-I9000") ||
                android.os.Build.DEVICE.equals("crespo")) {
            CaptureCapabilityAndroid specificCapability =
                    new CaptureCapabilityAndroid();
            specificCapability.width = 352;
            specificCapability.height = 288;
            specificCapability.maxFPS = 15;
            AddDeviceSpecificCapability(specificCapability);

            specificCapability = new CaptureCapabilityAndroid();
            specificCapability.width = 176;
            specificCapability.height = 144;
            specificCapability.maxFPS = 15;
            AddDeviceSpecificCapability(specificCapability);

            specificCapability = new CaptureCapabilityAndroid();
            specificCapability.width = 320;
            specificCapability.height = 240;
            specificCapability.maxFPS = 15;
            AddDeviceSpecificCapability(specificCapability);
        }
        
        
        if(android.os.Build.MANUFACTURER.equals("motorola") &&
                android.os.Build.DEVICE.equals("umts_sholes")) {
            for(AndroidVideoCaptureDevice device:deviceList) {
                for(CaptureCapabilityAndroid capability:device.captureCapabilies) {
                    capability.maxFPS=15;
                }
            }
        }
    }

    private void AddDeviceSpecificCapability(
        CaptureCapabilityAndroid specificCapability) {
        for(AndroidVideoCaptureDevice device:deviceList) {
            boolean foundCapability = false;
            for(CaptureCapabilityAndroid capability:device.captureCapabilies) {
                if(capability.width == specificCapability.width &&
                        capability.height == specificCapability.height) {
                    foundCapability = true;
                    break;
                }
            }
            if(foundCapability==false) {
                CaptureCapabilityAndroid newCaptureCapabilies[]=
                        new CaptureCapabilityAndroid[device.captureCapabilies.length+1];
                for(int i = 0; i < device.captureCapabilies.length; ++i) {
                    newCaptureCapabilies[i+1] = device.captureCapabilies[i];
                }
                newCaptureCapabilies[0] = specificCapability;
                device.captureCapabilies = newCaptureCapabilies;
            }
        }
    }

    
    public int NumberOfDevices() {
        return deviceList.size();
    }

    public String GetDeviceUniqueName(int deviceNumber) {
        if(deviceNumber < 0 || deviceNumber >= deviceList.size()) {
            return null;
        }
        return deviceList.get(deviceNumber).deviceUniqueName;
    }

    public CaptureCapabilityAndroid[] GetCapabilityArray (String deviceUniqueId)
    {
        for (AndroidVideoCaptureDevice device: deviceList) {
            if(device.deviceUniqueName.equals(deviceUniqueId)) {
                return (CaptureCapabilityAndroid[]) device.captureCapabilies;
            }
        }
        return null;
    }

    
    
    public int GetOrientation(String deviceUniqueId) {
        for (AndroidVideoCaptureDevice device: deviceList) {
            if(device.deviceUniqueName.equals(deviceUniqueId)) {
                return device.orientation;
            }
        }
        return -1;
    }

    
    public VideoCaptureAndroid AllocateCamera(int id, long context,
            String deviceUniqueId) {
        try {
            Log.d(TAG, "AllocateCamera " + deviceUniqueId);

            Camera camera = null;
            AndroidVideoCaptureDevice deviceToUse = null;
            for (AndroidVideoCaptureDevice device: deviceList) {
                if(device.deviceUniqueName.equals(deviceUniqueId)) {
                    
                    deviceToUse = device;
                    switch(device.frontCameraType) {
                        case GalaxyS:
                            camera = AllocateGalaxySFrontCamera();
                            break;
                        case HTCEvo:
                            camera = AllocateEVOFrontFacingCamera();
                            break;
                        default:
                            
                            if(android.os.Build.VERSION.SDK_INT>8)
                                camera=Camera.open(device.index);
                            else
                                camera=Camera.open(); 
                    }
                }
            }

            if(camera == null) {
                return null;
            }
            Log.v(TAG, "AllocateCamera - creating VideoCaptureAndroid");

            return new VideoCaptureAndroid(id, context, camera, deviceToUse);

        }catch (Exception ex) {
            Log.e(TAG, "AllocateCamera Failed to open camera- ex " +
                    ex.getLocalizedMessage());
        }
        return null;
    }

    
    private Camera.Parameters
    SearchOldFrontFacingCameras(AndroidVideoCaptureDevice newDevice)
            throws SecurityException, IllegalArgumentException,
            NoSuchMethodException, ClassNotFoundException,
            IllegalAccessException, InvocationTargetException {
        
        
        Camera camera = Camera.open();
        Camera.Parameters parameters = camera.getParameters();
        String cameraId = parameters.get("camera-id");
        if(cameraId != null && cameraId.equals("1")) {
            
            try {
                parameters.set("camera-id", 2);
                camera.setParameters(parameters);
                parameters = camera.getParameters();
                newDevice.frontCameraType = FrontFacingCameraType.GalaxyS;
                newDevice.orientation = 0;
                camera.release();
                return parameters;
            }
            catch (Exception ex) {
                
                Log.e(TAG, "Init Failed to open front camera camera - ex " +
                        ex.getLocalizedMessage());
            }
        }
        camera.release();

        
        File file =
                new File("/system/framework/com.htc.hardware.twinCamDevice.jar");
        boolean exists = file.exists();
        if (!exists) {
            file =
                    new File("/system/framework/com.sprint.hardware.twinCamDevice.jar");
            exists = file.exists();
        }
        if(exists) {
            newDevice.frontCameraType = FrontFacingCameraType.HTCEvo;
            newDevice.orientation = 0;
            Camera evCamera = AllocateEVOFrontFacingCamera();
            parameters = evCamera.getParameters();
            evCamera.release();
            return parameters;
        }
        return null;
    }

    
    
    private Camera AllocateEVOFrontFacingCamera()
            throws SecurityException, NoSuchMethodException,
            ClassNotFoundException, IllegalArgumentException,
            IllegalAccessException, InvocationTargetException {
        String classPath = null;
        File file =
                new File("/system/framework/com.htc.hardware.twinCamDevice.jar");
        classPath = "com.htc.hardware.twinCamDevice.FrontFacingCamera";
        boolean exists = file.exists();
        if (!exists){
            file =
                    new File("/system/framework/com.sprint.hardware.twinCamDevice.jar");
            classPath = "com.sprint.hardware.twinCamDevice.FrontFacingCamera";
            exists = file.exists();
        }
        if(!exists) {
            return null;
        }

        String dexOutputDir = "";
        if(context != null) {
            dexOutputDir = context.getFilesDir().getAbsolutePath();
            File mFilesDir = new File(dexOutputDir, "dexfiles");
            if(!mFilesDir.exists()){
                
                if(!mFilesDir.mkdirs()) {
                    
                }
            }
        }

        dexOutputDir += "/dexfiles";

        DexClassLoader loader =
                new DexClassLoader(file.getAbsolutePath(), dexOutputDir,
                        null, ClassLoader.getSystemClassLoader());

        Method method = loader.loadClass(classPath).getDeclaredMethod(
            "getFrontFacingCamera", (Class[]) null);
        Camera camera = (Camera) method.invoke((Object[])null,(Object[]) null);
        return camera;
    }

    
    
    private Camera AllocateGalaxySFrontCamera() {
        Camera camera = Camera.open();
        Camera.Parameters parameters = camera.getParameters();
        parameters.set("camera-id",2);
        camera.setParameters(parameters);
        return camera;
    }

}
