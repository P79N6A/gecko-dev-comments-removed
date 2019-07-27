




package org.mozilla.gecko;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PanZoomController;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.mozglue.JNITarget;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.mozglue.generatorannotations.OptionalGeneratedParameter;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
import org.mozilla.gecko.overlays.ui.ShareDialog;
import org.mozilla.gecko.prompts.PromptService;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.GeckoRequest;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSContainer;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.ProxySelector;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.Signature;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;
import android.os.SystemClock;
import android.os.Vibrator;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ContextThemeWrapper;
import android.view.HapticFeedbackConstants;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.webkit.MimeTypeMap;
import android.webkit.URLUtil;
import android.widget.AbsoluteLayout;
import android.widget.Toast;

public class GeckoAppShell
{
    private static final String LOGTAG = "GeckoAppShell";
    private static final boolean LOGGING = false;

    
    private GeckoAppShell() { }

    private static GeckoEditableListener editableListener;

    private static final CrashHandler CRASH_HANDLER = new CrashHandler() {
        @Override
        protected String getAppPackageName() {
            return AppConstants.ANDROID_PACKAGE_NAME;
        }

        @Override
        protected Context getAppContext() {
            return sContextGetter != null ? getContext() : null;
        }

        @Override
        protected Bundle getCrashExtras(final Thread thread, final Throwable exc) {
            final Bundle extras = super.getCrashExtras(thread, exc);

            extras.putString("ProductName", AppConstants.MOZ_APP_BASENAME);
            extras.putString("ProductID", AppConstants.MOZ_APP_ID);
            extras.putString("Version", AppConstants.MOZ_APP_VERSION);
            extras.putString("BuildID", AppConstants.MOZ_APP_BUILDID);
            extras.putString("Vendor", AppConstants.MOZ_APP_VENDOR);
            extras.putString("ReleaseChannel", AppConstants.MOZ_UPDATE_CHANNEL);
            return extras;
        }

        @Override
        public void uncaughtException(final Thread thread, final Throwable exc) {
            if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoExited)) {
                
                
                return;
            }

            super.uncaughtException(thread, exc);
        }

        @Override
        public boolean reportException(final Thread thread, final Throwable exc) {
            try {
                if (exc instanceof OutOfMemoryError) {
                    SharedPreferences prefs = getSharedPreferences();
                    SharedPreferences.Editor editor = prefs.edit();
                    editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, true);

                    
                    
                    editor.commit();
                }

                reportJavaCrash(getExceptionStackTrace(exc));

            } catch (final Throwable e) {
            }

            
            
            if (AppConstants.MOZ_CRASHREPORTER && AppConstants.MOZILLA_OFFICIAL) {
                
                return super.reportException(thread, exc);
            }
            return false;
        }
    };

    public static CrashHandler ensureCrashHandling() {
        
        return CRASH_HANDLER;
    }

    private static final Map<String, String> ALERT_COOKIES = new ConcurrentHashMap<String, String>();

    private static volatile boolean locationHighAccuracyEnabled;

    
     static NotificationClient notificationClient;

    
    private static final int HIGH_MEMORY_DEVICE_THRESHOLD_MB = 768;

    static private int sDensityDpi;
    static private int sScreenDepth;

    
    private static final float[] DEFAULT_LAUNCHER_ICON_HSV = { 32.0f, 1.0f, 1.0f };

    
    private static boolean sVibrationMaybePlaying;

    


    private static long sVibrationEndTime;

    private static Sensor gAccelerometerSensor;
    private static Sensor gLinearAccelerometerSensor;
    private static Sensor gGyroscopeSensor;
    private static Sensor gOrientationSensor;
    private static Sensor gProximitySensor;
    private static Sensor gLightSensor;
    private static Sensor gRotationVectorSensor;
    private static Sensor gGameRotationVectorSensor;

    private static final String GECKOREQUEST_RESPONSE_KEY = "response";
    private static final String GECKOREQUEST_ERROR_KEY = "error";

    



    static public final int WPL_STATE_START = 0x00000001;
    static public final int WPL_STATE_STOP = 0x00000010;
    static public final int WPL_STATE_IS_DOCUMENT = 0x00020000;
    static public final int WPL_STATE_IS_NETWORK = 0x00040000;

    


    static public final int LINK_TYPE_UNKNOWN = 0;
    static public final int LINK_TYPE_ETHERNET = 1;
    static public final int LINK_TYPE_USB = 2;
    static public final int LINK_TYPE_WIFI = 3;
    static public final int LINK_TYPE_WIMAX = 4;
    static public final int LINK_TYPE_2G = 5;
    static public final int LINK_TYPE_3G = 6;
    static public final int LINK_TYPE_4G = 7;

    

    
    public static native void registerJavaUiThread();
    public static native void nativeInit(ClassLoader clsLoader);

    
    public static native void onResume();
    public static void callObserver(String observerKey, String topic, String data) {
        sendEventToGecko(GeckoEvent.createCallObserverEvent(observerKey, topic, data));
    }
    public static void removeObserver(String observerKey) {
        sendEventToGecko(GeckoEvent.createRemoveObserverEvent(observerKey));
    }
    public static native Message getNextMessageFromQueue(MessageQueue queue);
    public static native void onSurfaceTextureFrameAvailable(Object surfaceTexture, int id);
    public static native void dispatchMemoryPressure();

    private static native void reportJavaCrash(String stackTrace);

    public static void notifyUriVisited(String uri) {
        sendEventToGecko(GeckoEvent.createVisitedEvent(uri));
    }

    public static native void processNextNativeEvent(boolean mayWait);

    public static native void notifyBatteryChange(double aLevel, boolean aCharging, double aRemainingTime);

    public static native void scheduleComposite();

    
    
    
    
    public static native void scheduleResumeComposition(int width, int height);

    public static native float computeRenderIntegrity();

    public static native SurfaceBits getSurfaceBits(Surface surface);

    public static native void addPresentationSurface(Surface surface);
    public static native void removePresentationSurface(Surface surface);

    public static native void onFullScreenPluginHidden(View view);

    public static class CreateShortcutFaviconLoadedListener implements OnFaviconLoadedListener {
        private final String title;
        private final String url;

        public CreateShortcutFaviconLoadedListener(final String url, final String title) {
            this.url = url;
            this.title = title;
        }

        @Override
        public void onFaviconLoaded(String pageUrl, String faviconURL, Bitmap favicon) {
            GeckoAppShell.createShortcut(title, url, favicon);
        }
    }

    private static LayerView sLayerView;

    public static void setLayerView(LayerView lv) {
        if (sLayerView == lv) {
            return;
        }
        sLayerView = lv;

        
        
        
        if (editableListener == null) {
            
            editableListener = new GeckoEditable();
        } else {
            
            GeckoAppShell.notifyIMEContext(GeckoEditableListener.IME_STATE_DISABLED, "", "", "");
        }
    }

    @RobocopTarget
    public static LayerView getLayerView() {
        return sLayerView;
    }

    public static void runGecko(String apkPath, String args, String url, String type) {
        
        MessageQueue.IdleHandler idleHandler = new MessageQueue.IdleHandler() {
            @Override public boolean queueIdle() {
                final Handler geckoHandler = ThreadUtils.sGeckoHandler;
                Message idleMsg = Message.obtain(geckoHandler);
                
                idleMsg.obj = geckoHandler;
                geckoHandler.sendMessageAtFrontOfQueue(idleMsg);
                
                return true;
            }
        };
        Looper.myQueue().addIdleHandler(idleHandler);

        
        nativeInit(GeckoAppShell.class.getClassLoader());

        
        String combinedArgs = apkPath + " -greomni " + apkPath;
        if (args != null)
            combinedArgs += " " + args;
        if (url != null)
            combinedArgs += " -url " + url;
        if (type != null)
            combinedArgs += " " + type;

        
        
        
        
        if (!AppConstants.MOZILLA_OFFICIAL) {
            Log.w(LOGTAG, "STARTUP PERFORMANCE WARNING: un-official build: purging the " +
                          "startup (JavaScript) caches.");
            combinedArgs += " -purgecaches";
        }

        DisplayMetrics metrics = getContext().getResources().getDisplayMetrics();
        combinedArgs += " -width " + metrics.widthPixels + " -height " + metrics.heightPixels;

        if (!AppConstants.MOZILLA_OFFICIAL) {
            Log.d(LOGTAG, "GeckoLoader.nativeRun " + combinedArgs);
        }
        
        GeckoLoader.nativeRun(combinedArgs);

        
        Looper.myQueue().removeIdleHandler(idleHandler);
    }

    














    @RobocopTarget
    public static void sendEventToGecko(GeckoEvent e) {
        if (e == null) {
            throw new IllegalArgumentException("e cannot be null.");
        }

        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            notifyGeckoOfEvent(e);
            
            
            e.recycle();
            return;
        }

        GeckoThread.addPendingEvent(e);
    }

    










    @RobocopTarget
    public static void sendRequestToGecko(final GeckoRequest request) {
        final String responseMessage = "Gecko:Request" + request.getId();

        EventDispatcher.getInstance().registerGeckoThreadListener(new NativeEventListener() {
            @Override
            public void handleMessage(String event, NativeJSObject message, EventCallback callback) {
                EventDispatcher.getInstance().unregisterGeckoThreadListener(this, event);
                if (!message.has(GECKOREQUEST_RESPONSE_KEY)) {
                    request.onError(message.getObject(GECKOREQUEST_ERROR_KEY));
                    return;
                }
                request.onResponse(message.getObject(GECKOREQUEST_RESPONSE_KEY));
            }
        }, responseMessage);

        sendEventToGecko(GeckoEvent.createBroadcastEvent(request.getName(), request.getData()));
    }

    
    public static native void notifyGeckoOfEvent(GeckoEvent event);

    
    public static native void notifyGeckoObservers(String subject, String data);

    



    @WrapElementForJNI(allowMultithread = true, noThrow = true)
    public static void handleUncaughtException(Thread thread, Throwable e) {
        CRASH_HANDLER.uncaughtException(thread, e);
    }

    @WrapElementForJNI
    public static void notifyIME(int type) {
        if (editableListener != null) {
            editableListener.notifyIME(type);
        }
    }

    @WrapElementForJNI
    public static void notifyIMEContext(int state, String typeHint,
                                        String modeHint, String actionHint) {
        if (editableListener != null) {
            editableListener.notifyIMEContext(state, typeHint,
                                               modeHint, actionHint);
        }
    }

    @WrapElementForJNI
    public static void notifyIMEChange(String text, int start, int end, int newEnd) {
        if (newEnd < 0) { 
            editableListener.onSelectionChange(start, end);
        } else { 
            editableListener.onTextChange(text, start, end, newEnd);
        }
    }

    private static final Object sEventAckLock = new Object();
    private static boolean sWaitingForEventAck;

    
    public static void sendEventToGeckoSync(GeckoEvent e) {
        e.setAckNeeded(true);

        long time = SystemClock.uptimeMillis();
        boolean isUiThread = ThreadUtils.isOnUiThread();

        synchronized (sEventAckLock) {
            if (sWaitingForEventAck) {
                
                Log.e(LOGTAG, "geckoEventSync() may have been called twice concurrently!", new Exception());
                
            }

            sendEventToGecko(e);
            sWaitingForEventAck = true;
            while (true) {
                if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoExiting) ||
                        GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoExited)) {
                    
                    Log.d(LOGTAG, "Skipping Gecko event sync during exit");
                    sWaitingForEventAck = false;
                    return;
                }

                try {
                    sEventAckLock.wait(1000);
                } catch (InterruptedException ie) {
                }
                if (!sWaitingForEventAck) {
                    
                    break;
                }
                long waited = SystemClock.uptimeMillis() - time;
                Log.d(LOGTAG, "Gecko event sync taking too long: " + waited + "ms");
            }
        }
    }

    
    @WrapElementForJNI
    public static void acknowledgeEvent() {
        synchronized (sEventAckLock) {
            sWaitingForEventAck = false;
            sEventAckLock.notifyAll();
        }
    }

    private static final Runnable sCallbackRunnable = new Runnable() {
        @Override
        public void run() {
            ThreadUtils.assertOnUiThread();
            long nextDelay = runUiThreadCallback();
            if (nextDelay >= 0) {
                ThreadUtils.getUiHandler().postDelayed(this, nextDelay);
            }
        }
    };

    private static native long runUiThreadCallback();

    @WrapElementForJNI(allowMultithread = true)
    private static void requestUiThreadCallback(long delay) {
        ThreadUtils.getUiHandler().postDelayed(sCallbackRunnable, delay);
    }

    private static float getLocationAccuracy(Location location) {
        float radius = location.getAccuracy();
        return (location.hasAccuracy() && radius > 0) ? radius : 1001;
    }

    private static Location getLastKnownLocation(LocationManager lm) {
        Location lastKnownLocation = null;
        List<String> providers = lm.getAllProviders();

        for (String provider : providers) {
            Location location = lm.getLastKnownLocation(provider);
            if (location == null) {
                continue;
            }

            if (lastKnownLocation == null) {
                lastKnownLocation = location;
                continue;
            }

            long timeDiff = location.getTime() - lastKnownLocation.getTime();
            if (timeDiff > 0 ||
                (timeDiff == 0 &&
                 getLocationAccuracy(location) < getLocationAccuracy(lastKnownLocation))) {
                lastKnownLocation = location;
            }
        }

        return lastKnownLocation;
    }

    @WrapElementForJNI
    public static void enableLocation(final boolean enable) {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    LocationManager lm = getLocationManager(getContext());
                    if (lm == null) {
                        return;
                    }

                    if (enable) {
                        Location lastKnownLocation = getLastKnownLocation(lm);
                        if (lastKnownLocation != null) {
                            getGeckoInterface().getLocationListener().onLocationChanged(lastKnownLocation);
                        }

                        Criteria criteria = new Criteria();
                        criteria.setSpeedRequired(false);
                        criteria.setBearingRequired(false);
                        criteria.setAltitudeRequired(false);
                        if (locationHighAccuracyEnabled) {
                            criteria.setAccuracy(Criteria.ACCURACY_FINE);
                            criteria.setCostAllowed(true);
                            criteria.setPowerRequirement(Criteria.POWER_HIGH);
                        } else {
                            criteria.setAccuracy(Criteria.ACCURACY_COARSE);
                            criteria.setCostAllowed(false);
                            criteria.setPowerRequirement(Criteria.POWER_LOW);
                        }

                        String provider = lm.getBestProvider(criteria, true);
                        if (provider == null)
                            return;

                        Looper l = Looper.getMainLooper();
                        lm.requestLocationUpdates(provider, 100, (float).5, getGeckoInterface().getLocationListener(), l);
                    } else {
                        lm.removeUpdates(getGeckoInterface().getLocationListener());
                    }
                }
            });
    }

    private static LocationManager getLocationManager(Context context) {
        try {
            return (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        } catch (NoSuchFieldError e) {
            
            
            
            
            Log.e(LOGTAG, "LOCATION_SERVICE not found?!", e);
            return null;
        }
    }

    @WrapElementForJNI
    public static void enableLocationHighAccuracy(final boolean enable) {
        locationHighAccuracyEnabled = enable;
    }

    @WrapElementForJNI
    public static void enableSensor(int aSensortype) {
        GeckoInterface gi = getGeckoInterface();
        if (gi == null)
            return;
        SensorManager sm = (SensorManager)
            getContext().getSystemService(Context.SENSOR_SERVICE);

        switch(aSensortype) {
        case GeckoHalDefines.SENSOR_ORIENTATION:
            if (gOrientationSensor == null)
                gOrientationSensor = sm.getDefaultSensor(Sensor.TYPE_ORIENTATION);
            if (gOrientationSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gOrientationSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        case GeckoHalDefines.SENSOR_ACCELERATION:
            if (gAccelerometerSensor == null)
                gAccelerometerSensor = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (gAccelerometerSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gAccelerometerSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        case GeckoHalDefines.SENSOR_PROXIMITY:
            if (gProximitySensor == null)
                gProximitySensor = sm.getDefaultSensor(Sensor.TYPE_PROXIMITY);
            if (gProximitySensor != null)
                sm.registerListener(gi.getSensorEventListener(), gProximitySensor, SensorManager.SENSOR_DELAY_NORMAL);
            break;

        case GeckoHalDefines.SENSOR_LIGHT:
            if (gLightSensor == null)
                gLightSensor = sm.getDefaultSensor(Sensor.TYPE_LIGHT);
            if (gLightSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gLightSensor, SensorManager.SENSOR_DELAY_NORMAL);
            break;

        case GeckoHalDefines.SENSOR_LINEAR_ACCELERATION:
            if (gLinearAccelerometerSensor == null)
                gLinearAccelerometerSensor = sm.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
            if (gLinearAccelerometerSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gLinearAccelerometerSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        case GeckoHalDefines.SENSOR_GYROSCOPE:
            if (gGyroscopeSensor == null)
                gGyroscopeSensor = sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            if (gGyroscopeSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gGyroscopeSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        case GeckoHalDefines.SENSOR_ROTATION_VECTOR:
            if (gRotationVectorSensor == null)
                gRotationVectorSensor = sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
            if (gRotationVectorSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gRotationVectorSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        case GeckoHalDefines.SENSOR_GAME_ROTATION_VECTOR:
            if (gGameRotationVectorSensor == null)
                gGameRotationVectorSensor = sm.getDefaultSensor(15 ); 
            if (gGameRotationVectorSensor != null)
                sm.registerListener(gi.getSensorEventListener(), gGameRotationVectorSensor, SensorManager.SENSOR_DELAY_GAME);
            break;

        default:
            Log.w(LOGTAG, "Error! Can't enable unknown SENSOR type " + aSensortype);
        }
    }

    @WrapElementForJNI
    public static void disableSensor(int aSensortype) {
        GeckoInterface gi = getGeckoInterface();
        if (gi == null)
            return;

        SensorManager sm = (SensorManager)
            getContext().getSystemService(Context.SENSOR_SERVICE);

        switch (aSensortype) {
        case GeckoHalDefines.SENSOR_ORIENTATION:
            if (gOrientationSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gOrientationSensor);
            break;

        case GeckoHalDefines.SENSOR_ACCELERATION:
            if (gAccelerometerSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gAccelerometerSensor);
            break;

        case GeckoHalDefines.SENSOR_PROXIMITY:
            if (gProximitySensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gProximitySensor);
            break;

        case GeckoHalDefines.SENSOR_LIGHT:
            if (gLightSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gLightSensor);
            break;

        case GeckoHalDefines.SENSOR_LINEAR_ACCELERATION:
            if (gLinearAccelerometerSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gLinearAccelerometerSensor);
            break;

        case GeckoHalDefines.SENSOR_ROTATION_VECTOR:
            if (gRotationVectorSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gRotationVectorSensor);
            break;

        case GeckoHalDefines.SENSOR_GAME_ROTATION_VECTOR:
            if (gGameRotationVectorSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gGameRotationVectorSensor);
            break;

        case GeckoHalDefines.SENSOR_GYROSCOPE:
            if (gGyroscopeSensor != null)
                sm.unregisterListener(gi.getSensorEventListener(), gGyroscopeSensor);
            break;
        default:
            Log.w(LOGTAG, "Error! Can't disable unknown SENSOR type " + aSensortype);
        }
    }

    @WrapElementForJNI
    public static void startMonitoringGamepad() {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    AndroidGamepadManager.startup();
                }
            });
    }

    @WrapElementForJNI
    public static void stopMonitoringGamepad() {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    AndroidGamepadManager.shutdown();
                }
            });
    }

    @WrapElementForJNI
    public static void gamepadAdded(final int device_id, final int service_id) {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    AndroidGamepadManager.gamepadAdded(device_id, service_id);
                }
            });
    }

    @WrapElementForJNI
    public static void moveTaskToBack() {
        if (getGeckoInterface() != null)
            getGeckoInterface().getActivity().moveTaskToBack(true);
    }

    @WrapElementForJNI
    static void scheduleRestart() {
        getGeckoInterface().doRestart();
    }

    
    
    @WrapElementForJNI
    static void createShortcut(final String aTitle, final String aURI, final String aIconData) {
        
        
        
        Favicons.getSizedFavicon(getContext(), aURI, aIconData, Integer.MAX_VALUE, 0,
            new OnFaviconLoadedListener() {
                @Override
                public void onFaviconLoaded(String url, String faviconURL, Bitmap favicon) {
                    createShortcut(aTitle, url, favicon);
                }
            }
        );
    }

    public static void createShortcut(final String aTitle, final String aURI, final Bitmap aBitmap) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                doCreateShortcut(aTitle, aURI, aBitmap);
            }
        });
    }

    


    private static void doCreateShortcut(final String aTitle, final String aURI, final Bitmap aIcon) {
        
        Intent shortcutIntent = new Intent();
        shortcutIntent.setAction(GeckoApp.ACTION_HOMESCREEN_SHORTCUT);
        shortcutIntent.setData(Uri.parse(aURI));
        shortcutIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                    AppConstants.BROWSER_INTENT_CLASS_NAME);

        Intent intent = new Intent();
        intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
        intent.putExtra(Intent.EXTRA_SHORTCUT_ICON, getLauncherIcon(aIcon));

        if (aTitle != null) {
            intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aTitle);
        } else {
            intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aURI);
        }

        
        intent.putExtra("duplicate", false);

        intent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
        getContext().sendBroadcast(intent);
    }

    @JNITarget
    static public int getPreferredIconSize() {
        if (Versions.feature11Plus) {
            ActivityManager am = (ActivityManager)getContext().getSystemService(Context.ACTIVITY_SERVICE);
            return am.getLauncherLargeIconSize();
        } else {
            switch (getDpi()) {
                case DisplayMetrics.DENSITY_MEDIUM:
                    return 48;
                case DisplayMetrics.DENSITY_XHIGH:
                    return 96;
                case DisplayMetrics.DENSITY_HIGH:
                default:
                    return 72;
            }
        }
    }

    static private Bitmap getLauncherIcon(Bitmap aSource) {
        final int kOffset = 6;
        final int kRadius = 5;
        int size = getPreferredIconSize();
        int insetSize = aSource != null ? size * 2 / 3 : size;

        Bitmap bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);


        
        Paint paint = new Paint();
        if (aSource == null) {
            
            paint.setColor(Color.HSVToColor(DEFAULT_LAUNCHER_ICON_HSV));
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
        } else if (aSource.getWidth() >= insetSize || aSource.getHeight() >= insetSize) {
            
            Rect iconBounds = new Rect(0, 0, size, size);
            canvas.drawBitmap(aSource, null, iconBounds, null);
            return bitmap;
        } else {
            
            int color = BitmapUtils.getDominantColor(aSource);
            paint.setColor(color);
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
            paint.setColor(Color.argb(100, 255, 255, 255));
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
        }

        
        Bitmap overlay = BitmapUtils.decodeResource(getContext(), R.drawable.home_bg);
        canvas.drawBitmap(overlay, null, new Rect(0, 0, size, size), null);

        
        if (aSource == null)
            aSource = BitmapUtils.decodeResource(getContext(), R.drawable.home_star);

        
        int sWidth = insetSize / 2;
        int sHeight = sWidth;

        int halfSize = size / 2;
        canvas.drawBitmap(aSource,
                          null,
                          new Rect(halfSize - sWidth,
                                   halfSize - sHeight,
                                   halfSize + sWidth,
                                   halfSize + sHeight),
                          null);

        return bitmap;
    }

    @WrapElementForJNI(stubName = "GetHandlersForMimeTypeWrapper")
    static String[] getHandlersForMimeType(String aMimeType, String aAction) {
        Intent intent = getIntentForActionString(aAction);
        if (aMimeType != null && aMimeType.length() > 0)
            intent.setType(aMimeType);
        return getHandlersForIntent(intent);
    }

    @WrapElementForJNI(stubName = "GetHandlersForURLWrapper")
    static String[] getHandlersForURL(String aURL, String aAction) {
        
        Uri uri = aURL.indexOf(':') >= 0 ? Uri.parse(aURL) : new Uri.Builder().scheme(aURL).build();

        Intent intent = getOpenURIIntent(getContext(), uri.toString(), "",
            TextUtils.isEmpty(aAction) ? Intent.ACTION_VIEW : aAction, "");

        return getHandlersForIntent(intent);
    }

    static boolean hasHandlersForIntent(Intent intent) {
        try {
            PackageManager pm = getContext().getPackageManager();
            List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
            return !list.isEmpty();
        } catch (Exception ex) {
            Log.e(LOGTAG, "Exception in GeckoAppShell.hasHandlersForIntent");
            return false;
        }
    }

    static String[] getHandlersForIntent(Intent intent) {
        try {
            PackageManager pm = getContext().getPackageManager();
            List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
            int numAttr = 4;
            String[] ret = new String[list.size() * numAttr];
            for (int i = 0; i < list.size(); i++) {
                ResolveInfo resolveInfo = list.get(i);
                ret[i * numAttr] = resolveInfo.loadLabel(pm).toString();
                if (resolveInfo.isDefault)
                    ret[i * numAttr + 1] = "default";
                else
                    ret[i * numAttr + 1] = "";
                ret[i * numAttr + 2] = resolveInfo.activityInfo.applicationInfo.packageName;
                ret[i * numAttr + 3] = resolveInfo.activityInfo.name;
            }
            return ret;
        } catch (Exception ex) {
            Log.e(LOGTAG, "Exception in GeckoAppShell.getHandlersForIntent");
            return new String[0];
        }
    }

    static Intent getIntentForActionString(String aAction) {
        
        if (TextUtils.isEmpty(aAction)) {
            return new Intent(Intent.ACTION_VIEW);
        }
        return new Intent(aAction);
    }

    @WrapElementForJNI(stubName = "GetExtensionFromMimeTypeWrapper")
    static String getExtensionFromMimeType(String aMimeType) {
        return MimeTypeMap.getSingleton().getExtensionFromMimeType(aMimeType);
    }

    @WrapElementForJNI(stubName = "GetMimeTypeFromExtensionsWrapper")
    static String getMimeTypeFromExtensions(String aFileExt) {
        StringTokenizer st = new StringTokenizer(aFileExt, ".,; ");
        String type = null;
        String subType = null;
        while (st.hasMoreElements()) {
            String ext = st.nextToken();
            String mt = getMimeTypeFromExtension(ext);
            if (mt == null)
                continue;
            int slash = mt.indexOf('/');
            String tmpType = mt.substring(0, slash);
            if (!tmpType.equalsIgnoreCase(type))
                type = type == null ? tmpType : "*";
            String tmpSubType = mt.substring(slash + 1);
            if (!tmpSubType.equalsIgnoreCase(subType))
                subType = subType == null ? tmpSubType : "*";
        }
        if (type == null)
            type = "*";
        if (subType == null)
            subType = "*";
        return type + "/" + subType;
    }

    static void safeStreamClose(Closeable stream) {
        try {
            if (stream != null)
                stream.close();
        } catch (IOException e) {}
    }

    static boolean isUriSafeForScheme(Uri aUri) {
        
        
        
        final String scheme = aUri.getScheme();
        if ("tel".equals(scheme) || "sms".equals(scheme)) {
            final String number = aUri.getSchemeSpecificPart();
            if (number.contains("#") || number.contains("*") || aUri.getFragment() != null) {
                return false;
            }
        }
        return true;
    }

    














    @WrapElementForJNI
    public static boolean openUriExternal(String targetURI,
                                          String mimeType,
              @OptionalGeneratedParameter String packageName,
              @OptionalGeneratedParameter String className,
              @OptionalGeneratedParameter String action,
              @OptionalGeneratedParameter String title) {
        final Context context = getContext();
        final Intent intent = getOpenURIIntent(context, targetURI,
                                               mimeType, action, title);

        if (intent == null) {
            return false;
        }

        if (!TextUtils.isEmpty(className)) {
            if (!TextUtils.isEmpty(packageName)) {
                intent.setClassName(packageName, className);
            } else {
                
                intent.setClassName(context, className);
            }
        }

        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        try {
            context.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            return false;
        }
    }

    







    static Uri normalizeUriScheme(final Uri u) {
        final String scheme = u.getScheme();
        final String lower  = scheme.toLowerCase(Locale.US);
        if (lower.equals(scheme)) {
            return u;
        }

        
        return u.buildUpon().scheme(lower).build();
    }

    











    public static Intent getShareIntent(final Context context,
                                        final String targetURI,
                                        final String mimeType,
                                        final String title) {
        Intent shareIntent = getIntentForActionString(Intent.ACTION_SEND);
        shareIntent.putExtra(Intent.EXTRA_TEXT, targetURI);
        shareIntent.putExtra(Intent.EXTRA_SUBJECT, title);
        shareIntent.putExtra(ShareDialog.INTENT_EXTRA_DEVICES_ONLY, true);

        
        
        
        shareIntent.putExtra(Intent.EXTRA_TITLE, title);

        if (mimeType != null && mimeType.length() > 0) {
            shareIntent.setType(mimeType);
        }

        return shareIntent;
    }

    













    static Intent getOpenURIIntent(final Context context,
                                   final String targetURI,
                                   final String mimeType,
                                   final String action,
                                   final String title) {

        if (action.equalsIgnoreCase(Intent.ACTION_SEND)) {
            Intent shareIntent = getShareIntent(context, targetURI, mimeType, title);
            return Intent.createChooser(shareIntent,
                                        context.getResources().getString(R.string.share_title)); 
        }

        final Uri uri = normalizeUriScheme(targetURI.indexOf(':') >= 0 ? Uri.parse(targetURI) : new Uri.Builder().scheme(targetURI).build());
        if (!TextUtils.isEmpty(mimeType)) {
            Intent intent = getIntentForActionString(action);
            intent.setDataAndType(uri, mimeType);
            return intent;
        }

        if (!isUriSafeForScheme(uri)) {
            return null;
        }

        final String scheme = uri.getScheme();

        
        
        
        
        final String extension = MimeTypeMap.getFileExtensionFromUrl(targetURI);
        final String mimeType2 = getMimeTypeFromExtension(extension);
        final Intent intent = getIntentForActionString(action);
        intent.setDataAndType(uri, mimeType2);

        if ("vnd.youtube".equals(scheme) &&
            !hasHandlersForIntent(intent) &&
            !TextUtils.isEmpty(uri.getSchemeSpecificPart())) {

            
            
            final Class<?> c;
            final String browserClassName = AppConstants.BROWSER_INTENT_CLASS_NAME;
            try {
                c = Class.forName(browserClassName);
            } catch (ClassNotFoundException e) {
                
                Log.wtf(LOGTAG, "Class " + browserClassName + " not found!");
                return null;
            }

            final Uri youtubeURI = getYouTubeHTML5URI(uri);
            if (youtubeURI != null) {
                
                
                final Intent view = new Intent(GeckoApp.ACTION_LOAD, youtubeURI, context, c);
                return view;
            }
        }

        
        
        if (!"sms".equals(scheme)) {
            return intent;
        }

        final String query = uri.getEncodedQuery();
        if (TextUtils.isEmpty(query)) {
            return intent;
        }

        final String[] fields = query.split("&");
        boolean foundBody = false;
        String resultQuery = "";
        for (String field : fields) {
            if (foundBody || !field.startsWith("body=")) {
                resultQuery = resultQuery.concat(resultQuery.length() > 0 ? "&" + field : field);
                continue;
            }

            
            final String body = Uri.decode(field.substring(5));
            intent.putExtra("sms_body", body);
            foundBody = true;
        }

        if (!foundBody) {
            
            return intent;
        }

        
        
        final String newQuery = resultQuery.length() > 0 ? "?" + resultQuery : "";
        final Uri pruned = uri.buildUpon().encodedQuery(newQuery).build();
        intent.setData(pruned);

        return intent;
    }

    










    private static Uri getYouTubeHTML5URI(final Uri uri) {
        if (uri == null) {
            return null;
        }

        final String ssp = uri.getSchemeSpecificPart();
        if (TextUtils.isEmpty(ssp)) {
            return null;
        }

        return Uri.parse("https://www.youtube.com/embed/" + ssp);
    }

    


    public static void setNotificationClient(NotificationClient client) {
        if (notificationClient == null) {
            notificationClient = client;
        } else {
            Log.d(LOGTAG, "Notification client already set");
        }
    }

    @WrapElementForJNI(stubName = "ShowAlertNotificationWrapper")
    public static void showAlertNotification(String aImageUrl, String aAlertTitle, String aAlertText,
                                             String aAlertCookie, String aAlertName) {
        
        String app = getContext().getClass().getName();
        Intent notificationIntent = new Intent(GeckoApp.ACTION_ALERT_CALLBACK);
        notificationIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, app);
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        int notificationID = aAlertName.hashCode();

        
        Uri.Builder b = new Uri.Builder();
        Uri dataUri = b.scheme("alert").path(Integer.toString(notificationID))
                                       .appendQueryParameter("name", aAlertName)
                                       .appendQueryParameter("cookie", aAlertCookie)
                                       .build();
        notificationIntent.setData(dataUri);
        PendingIntent contentIntent = PendingIntent.getActivity(
                getContext(), 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        ALERT_COOKIES.put(aAlertName, aAlertCookie);
        callObserver(aAlertName, "alertshow", aAlertCookie);

        notificationClient.add(notificationID, aImageUrl, aAlertTitle, aAlertText, contentIntent);
    }

    @WrapElementForJNI
    public static void alertsProgressListener_OnProgress(String aAlertName, long aProgress, long aProgressMax, String aAlertText) {
        int notificationID = aAlertName.hashCode();
        notificationClient.update(notificationID, aProgress, aProgressMax, aAlertText);
    }

    @WrapElementForJNI
    public static void closeNotification(String aAlertName) {
        String alertCookie = ALERT_COOKIES.get(aAlertName);
        if (alertCookie != null) {
            callObserver(aAlertName, "alertfinished", alertCookie);
            ALERT_COOKIES.remove(aAlertName);
        }

        removeObserver(aAlertName);

        int notificationID = aAlertName.hashCode();
        notificationClient.remove(notificationID);
    }

    public static void handleNotification(String aAction, String aAlertName, String aAlertCookie) {
        int notificationID = aAlertName.hashCode();

        if (GeckoApp.ACTION_ALERT_CALLBACK.equals(aAction)) {
            callObserver(aAlertName, "alertclickcallback", aAlertCookie);

            if (notificationClient.isOngoing(notificationID)) {
                
                return;
            }
        }
        closeNotification(aAlertName);
    }

    @WrapElementForJNI(stubName = "GetDpiWrapper")
    public static int getDpi() {
        if (sDensityDpi == 0) {
            sDensityDpi = getContext().getResources().getDisplayMetrics().densityDpi;
        }

        return sDensityDpi;
    }

    @WrapElementForJNI
    public static float getDensity() {
        return getContext().getResources().getDisplayMetrics().density;
    }

    private static boolean isHighMemoryDevice() {
        return HardwareUtils.getMemSize() > HIGH_MEMORY_DEVICE_THRESHOLD_MB;
    }

    



    @WrapElementForJNI(stubName = "GetScreenDepthWrapper")
    public static synchronized int getScreenDepth() {
        if (sScreenDepth == 0) {
            sScreenDepth = 16;
            PixelFormat info = new PixelFormat();
            PixelFormat.getPixelFormatInfo(getGeckoInterface().getActivity().getWindowManager().getDefaultDisplay().getPixelFormat(), info);
            if (info.bitsPerPixel >= 24 && isHighMemoryDevice()) {
                sScreenDepth = 24;
            }
        }

        return sScreenDepth;
    }

    public static synchronized void setScreenDepthOverride(int aScreenDepth) {
        if (sScreenDepth != 0) {
            Log.e(LOGTAG, "Tried to override screen depth after it's already been set");
            return;
        }

        sScreenDepth = aScreenDepth;
    }

    @WrapElementForJNI
    public static void setFullScreen(boolean fullscreen) {
        if (getGeckoInterface() != null)
            getGeckoInterface().setFullScreen(fullscreen);
    }

    @WrapElementForJNI
    public static void performHapticFeedback(boolean aIsLongPress) {
        
        
        if (!sVibrationMaybePlaying || System.nanoTime() >= sVibrationEndTime) {
            LayerView layerView = getLayerView();
            layerView.performHapticFeedback(aIsLongPress ?
                                            HapticFeedbackConstants.LONG_PRESS :
                                            HapticFeedbackConstants.VIRTUAL_KEY);
        }
    }

    private static Vibrator vibrator() {
        LayerView layerView = getLayerView();
        return (Vibrator) layerView.getContext().getSystemService(Context.VIBRATOR_SERVICE);
    }

    
    private static long[] convertIntToLongArray(int[] input) {
        long[] output = new long[input.length];
        for (int i = 0; i < input.length; i++) {
            output[i] = input[i];
        }
        return output;
    }

    
    public static void vibrateOnHapticFeedbackEnabled(int[] milliseconds) {
        if (Settings.System.getInt(getContext().getContentResolver(), Settings.System.HAPTIC_FEEDBACK_ENABLED, 0) > 0) {
            vibrate(convertIntToLongArray(milliseconds), -1);
        }
    }

    @WrapElementForJNI(stubName = "Vibrate1")
    public static void vibrate(long milliseconds) {
        sVibrationEndTime = System.nanoTime() + milliseconds * 1000000;
        sVibrationMaybePlaying = true;
        vibrator().vibrate(milliseconds);
    }

    @WrapElementForJNI(stubName = "VibrateA")
    public static void vibrate(long[] pattern, int repeat) {
        
        
        long vibrationDuration = 0;
        int iterLen = pattern.length - (pattern.length % 2 == 0 ? 1 : 0);
        for (int i = 0; i < iterLen; i++) {
          vibrationDuration += pattern[i];
        }

        sVibrationEndTime = System.nanoTime() + vibrationDuration * 1000000;
        sVibrationMaybePlaying = true;
        vibrator().vibrate(pattern, repeat);
    }

    @WrapElementForJNI
    public static void cancelVibrate() {
        sVibrationMaybePlaying = false;
        sVibrationEndTime = 0;
        vibrator().cancel();
    }

    @WrapElementForJNI
    public static void showInputMethodPicker() {
        InputMethodManager imm = (InputMethodManager)
            getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showInputMethodPicker();
    }

    @WrapElementForJNI
    public static void setKeepScreenOn(final boolean on) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
            }
        });
    }

    @WrapElementForJNI
    public static void notifyDefaultPrevented(final boolean defaultPrevented) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                LayerView view = getLayerView();
                PanZoomController controller = (view == null ? null : view.getPanZoomController());
                if (controller != null) {
                    controller.notifyDefaultActionPrevented(defaultPrevented);
                }
            }
        });
    }

    @WrapElementForJNI
    public static boolean isNetworkLinkUp() {
        ConnectivityManager cm = (ConnectivityManager)
           getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        try {
            NetworkInfo info = cm.getActiveNetworkInfo();
            if (info == null || !info.isConnected())
                return false;
        } catch (SecurityException se) {
            return false;
        }
        return true;
    }

    @WrapElementForJNI
    public static boolean isNetworkLinkKnown() {
        ConnectivityManager cm = (ConnectivityManager)
            getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        try {
            if (cm.getActiveNetworkInfo() == null)
                return false;
        } catch (SecurityException se) {
            return false;
        }
        return true;
    }

    @WrapElementForJNI
    public static int networkLinkType() {
        ConnectivityManager cm = (ConnectivityManager)
            getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();
        if (info == null) {
            return LINK_TYPE_UNKNOWN;
        }

        switch (info.getType()) {
            case ConnectivityManager.TYPE_ETHERNET:
                return LINK_TYPE_ETHERNET;
            case ConnectivityManager.TYPE_WIFI:
                return LINK_TYPE_WIFI;
            case ConnectivityManager.TYPE_WIMAX:
                return LINK_TYPE_WIMAX;
            case ConnectivityManager.TYPE_MOBILE:
                break; 
            default:
                Log.w(LOGTAG, "Ignoring the current network type.");
                return LINK_TYPE_UNKNOWN;
        }

        TelephonyManager tm = (TelephonyManager)
            getContext().getSystemService(Context.TELEPHONY_SERVICE);
        if (tm == null) {
            Log.e(LOGTAG, "Telephony service does not exist");
            return LINK_TYPE_UNKNOWN;
        }

        switch (tm.getNetworkType()) {
            case TelephonyManager.NETWORK_TYPE_IDEN:
            case TelephonyManager.NETWORK_TYPE_CDMA:
            case TelephonyManager.NETWORK_TYPE_GPRS:
                return LINK_TYPE_2G;
            case TelephonyManager.NETWORK_TYPE_1xRTT:
            case TelephonyManager.NETWORK_TYPE_EDGE:
                return LINK_TYPE_2G; 
            case TelephonyManager.NETWORK_TYPE_UMTS:
            case TelephonyManager.NETWORK_TYPE_EVDO_0:
                return LINK_TYPE_3G;
            case TelephonyManager.NETWORK_TYPE_HSPA:
            case TelephonyManager.NETWORK_TYPE_HSDPA:
            case TelephonyManager.NETWORK_TYPE_HSUPA:
            case TelephonyManager.NETWORK_TYPE_EVDO_A:
            case TelephonyManager.NETWORK_TYPE_EVDO_B:
            case TelephonyManager.NETWORK_TYPE_EHRPD:
                return LINK_TYPE_3G; 
            case TelephonyManager.NETWORK_TYPE_HSPAP:
                return LINK_TYPE_3G; 
            case TelephonyManager.NETWORK_TYPE_LTE:
                return LINK_TYPE_4G; 
            case TelephonyManager.NETWORK_TYPE_UNKNOWN:
            default:
                Log.w(LOGTAG, "Connected to an unknown mobile network!");
                return LINK_TYPE_UNKNOWN;
        }
    }

    @WrapElementForJNI(stubName = "GetSystemColoursWrapper")
    public static int[] getSystemColors() {
        
        final int[] attrsAppearance = {
            android.R.attr.textColor,
            android.R.attr.textColorPrimary,
            android.R.attr.textColorPrimaryInverse,
            android.R.attr.textColorSecondary,
            android.R.attr.textColorSecondaryInverse,
            android.R.attr.textColorTertiary,
            android.R.attr.textColorTertiaryInverse,
            android.R.attr.textColorHighlight,
            android.R.attr.colorForeground,
            android.R.attr.colorBackground,
            android.R.attr.panelColorForeground,
            android.R.attr.panelColorBackground
        };

        int[] result = new int[attrsAppearance.length];

        final ContextThemeWrapper contextThemeWrapper =
            new ContextThemeWrapper(getContext(), android.R.style.TextAppearance);

        final TypedArray appearance = contextThemeWrapper.getTheme().obtainStyledAttributes(attrsAppearance);

        if (appearance != null) {
            for (int i = 0; i < appearance.getIndexCount(); i++) {
                int idx = appearance.getIndex(i);
                int color = appearance.getColor(idx, 0);
                result[idx] = color;
            }
            appearance.recycle();
        }

        return result;
    }

    @WrapElementForJNI
    public static void killAnyZombies() {
        GeckoProcessesVisitor visitor = new GeckoProcessesVisitor() {
            @Override
            public boolean callback(int pid) {
                if (pid != android.os.Process.myPid())
                    android.os.Process.killProcess(pid);
                return true;
            }
        };
            
        EnumerateGeckoProcesses(visitor);
    }

    interface GeckoProcessesVisitor{
        boolean callback(int pid);
    }

    private static void EnumerateGeckoProcesses(GeckoProcessesVisitor visiter) {
        int pidColumn = -1;
        int userColumn = -1;

        try {
            
            java.lang.Process ps = Runtime.getRuntime().exec("ps");
            BufferedReader in = new BufferedReader(new InputStreamReader(ps.getInputStream()),
                                                   2048);

            String headerOutput = in.readLine();

            
            StringTokenizer st = new StringTokenizer(headerOutput);
            
            int tokenSoFar = 0;
            while (st.hasMoreTokens()) {
                String next = st.nextToken();
                if (next.equalsIgnoreCase("PID"))
                    pidColumn = tokenSoFar;
                else if (next.equalsIgnoreCase("USER"))
                    userColumn = tokenSoFar;
                tokenSoFar++;
            }

            
            String psOutput = null;
            while ((psOutput = in.readLine()) != null) {
                String[] split = psOutput.split("\\s+");
                if (split.length <= pidColumn || split.length <= userColumn)
                    continue;
                int uid = android.os.Process.getUidForName(split[userColumn]);
                if (uid == android.os.Process.myUid() &&
                    !split[split.length - 1].equalsIgnoreCase("ps")) {
                    int pid = Integer.parseInt(split[pidColumn]);
                    boolean keepGoing = visiter.callback(pid);
                    if (keepGoing == false)
                        break;
                }
            }
            in.close();
        }
        catch (Exception e) {
            Log.w(LOGTAG, "Failed to enumerate Gecko processes.",  e);
        }
    }

    public static String getAppNameByPID(int pid) {
        BufferedReader cmdlineReader = null;
        String path = "/proc/" + pid + "/cmdline";
        try {
            File cmdlineFile = new File(path);
            if (!cmdlineFile.exists())
                return "";
            cmdlineReader = new BufferedReader(new FileReader(cmdlineFile));
            return cmdlineReader.readLine().trim();
        } catch (Exception ex) {
            return "";
        } finally {
            if (null != cmdlineReader) {
                try {
                    cmdlineReader.close();
                } catch (Exception e) {}
            }
        }
    }

    public static void listOfOpenFiles() {
        int pidColumn = -1;
        int nameColumn = -1;

        try {
            String filter = GeckoProfile.get(getContext()).getDir().toString();
            Log.d(LOGTAG, "[OPENFILE] Filter: " + filter);

            
            java.lang.Process lsof = Runtime.getRuntime().exec("lsof");
            BufferedReader in = new BufferedReader(new InputStreamReader(lsof.getInputStream()), 2048);

            String headerOutput = in.readLine();
            StringTokenizer st = new StringTokenizer(headerOutput);
            int token = 0;
            while (st.hasMoreTokens()) {
                String next = st.nextToken();
                if (next.equalsIgnoreCase("PID"))
                    pidColumn = token;
                else if (next.equalsIgnoreCase("NAME"))
                    nameColumn = token;
                token++;
            }

            
            Map<Integer, String> pidNameMap = new TreeMap<Integer, String>();
            String output = null;
            while ((output = in.readLine()) != null) {
                String[] split = output.split("\\s+");
                if (split.length <= pidColumn || split.length <= nameColumn)
                    continue;
                Integer pid = new Integer(split[pidColumn]);
                String name = pidNameMap.get(pid);
                if (name == null) {
                    name = getAppNameByPID(pid.intValue());
                    pidNameMap.put(pid, name);
                }
                String file = split[nameColumn];
                if (!TextUtils.isEmpty(name) && !TextUtils.isEmpty(file) && file.startsWith(filter))
                    Log.d(LOGTAG, "[OPENFILE] " + name + "(" + split[pidColumn] + ") : " + file);
            }
            in.close();
        } catch (Exception e) { }
    }

    @WrapElementForJNI(stubName = "GetIconForExtensionWrapper")
    public static byte[] getIconForExtension(String aExt, int iconSize) {
        try {
            if (iconSize <= 0)
                iconSize = 16;

            if (aExt != null && aExt.length() > 1 && aExt.charAt(0) == '.')
                aExt = aExt.substring(1);

            PackageManager pm = getContext().getPackageManager();
            Drawable icon = getDrawableForExtension(pm, aExt);
            if (icon == null) {
                
                icon = pm.getDefaultActivityIcon();
            }

            Bitmap bitmap = ((BitmapDrawable)icon).getBitmap();
            if (bitmap.getWidth() != iconSize || bitmap.getHeight() != iconSize)
                bitmap = Bitmap.createScaledBitmap(bitmap, iconSize, iconSize, true);

            ByteBuffer buf = ByteBuffer.allocate(iconSize * iconSize * 4);
            bitmap.copyPixelsToBuffer(buf);

            return buf.array();
        }
        catch (Exception e) {
            Log.w(LOGTAG, "getIconForExtension failed.",  e);
            return null;
        }
    }

    public static String getMimeTypeFromExtension(String ext) {
        final MimeTypeMap mtm = MimeTypeMap.getSingleton();
        return mtm.getMimeTypeFromExtension(ext);
    }
    
    private static Drawable getDrawableForExtension(PackageManager pm, String aExt) {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        final String mimeType = getMimeTypeFromExtension(aExt);
        if (mimeType != null && mimeType.length() > 0)
            intent.setType(mimeType);
        else
            return null;

        List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
        if (list.size() == 0)
            return null;

        ResolveInfo resolveInfo = list.get(0);

        if (resolveInfo == null)
            return null;

        ActivityInfo activityInfo = resolveInfo.activityInfo;

        return activityInfo.loadIcon(pm);
    }

    @WrapElementForJNI
    public static boolean getShowPasswordSetting() {
        try {
            int showPassword =
                Settings.System.getInt(getContext().getContentResolver(),
                                       Settings.System.TEXT_SHOW_PASSWORD, 1);
            return (showPassword > 0);
        }
        catch (Exception e) {
            return true;
        }
    }

    @WrapElementForJNI(stubName = "AddPluginViewWrapper")
    public static void addPluginView(View view,
                                     float x, float y,
                                     float w, float h,
                                     boolean isFullScreen) {
        if (getGeckoInterface() != null)
             getGeckoInterface().addPluginView(view, new RectF(x, y, x + w, y + h), isFullScreen);
    }

    @WrapElementForJNI
    public static void removePluginView(View view, boolean isFullScreen) {
        if (getGeckoInterface() != null)
            getGeckoInterface().removePluginView(view, isFullScreen);
    }

    



    public static final String PLUGIN_ACTION = "android.webkit.PLUGIN";
    public static final String PLUGIN_PERMISSION = "android.webkit.permission.PLUGIN";

    private static final String PLUGIN_SYSTEM_LIB = "/system/lib/plugins/";

    private static final String PLUGIN_TYPE = "type";
    private static final String TYPE_NATIVE = "native";
    public static final ArrayList<PackageInfo> mPackageInfoCache = new ArrayList<>();

    
    static String[] getPluginDirectories() {

        
        boolean isTegra = (new File("/system/lib/hw/gralloc.tegra.so")).exists() ||
                          (new File("/system/lib/hw/gralloc.tegra3.so")).exists();
        if (isTegra) {
            
            if (Versions.feature19Plus) {
                Log.w(LOGTAG, "Blocking plugins because of Tegra (bug 957694)");
                return null;
            }

            
            final File vfile = new File("/proc/version");
            try {
                if (vfile.canRead()) {
                    final BufferedReader reader = new BufferedReader(new FileReader(vfile));
                    try {
                        final String version = reader.readLine();
                        if (version.indexOf("CM9") != -1 ||
                            version.indexOf("cyanogen") != -1 ||
                            version.indexOf("Nova") != -1) {
                            Log.w(LOGTAG, "Blocking plugins because of Tegra 2 + unofficial ICS bug (bug 736421)");
                            return null;
                        }
                    } finally {
                      reader.close();
                    }
                }
            } catch (IOException ex) {
                
            }
        }

        ArrayList<String> directories = new ArrayList<String>();
        PackageManager pm = getContext().getPackageManager();
        List<ResolveInfo> plugins = pm.queryIntentServices(new Intent(PLUGIN_ACTION),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        synchronized(mPackageInfoCache) {

            
            mPackageInfoCache.clear();


            for (ResolveInfo info : plugins) {

                
                ServiceInfo serviceInfo = info.serviceInfo;
                if (serviceInfo == null) {
                    Log.w(LOGTAG, "Ignoring bad plugin.");
                    continue;
                }

                
                
                
                if (serviceInfo.packageName.equals("com.htc.flashliteplugin")) {
                    Log.w(LOGTAG, "Skipping HTC's flash lite plugin");
                    continue;
                }


                
                PackageInfo pkgInfo;
                try {
                    pkgInfo = pm.getPackageInfo(serviceInfo.packageName,
                                    PackageManager.GET_PERMISSIONS
                                    | PackageManager.GET_SIGNATURES);
                } catch (Exception e) {
                    Log.w(LOGTAG, "Can't find plugin: " + serviceInfo.packageName);
                    continue;
                }

                if (pkgInfo == null) {
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Could not load package information.");
                    continue;
                }

                





                String directory = pkgInfo.applicationInfo.dataDir + "/lib";
                final int appFlags = pkgInfo.applicationInfo.flags;
                final int updatedSystemFlags = ApplicationInfo.FLAG_SYSTEM |
                                               ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;

                
                if ((appFlags & updatedSystemFlags) == ApplicationInfo.FLAG_SYSTEM) {
                    directory = PLUGIN_SYSTEM_LIB + pkgInfo.packageName;
                }

                
                String permissions[] = pkgInfo.requestedPermissions;
                if (permissions == null) {
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Does not have required permission.");
                    continue;
                }
                boolean permissionOk = false;
                for (String permit : permissions) {
                    if (PLUGIN_PERMISSION.equals(permit)) {
                        permissionOk = true;
                        break;
                    }
                }
                if (!permissionOk) {
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Does not have required permission (2).");
                    continue;
                }

                
                Signature signatures[] = pkgInfo.signatures;
                if (signatures == null) {
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Not signed.");
                    continue;
                }

                
                if (serviceInfo.metaData == null) {
                    Log.e(LOGTAG, "The plugin '" + serviceInfo.name + "' has no defined type.");
                    continue;
                }

                String pluginType = serviceInfo.metaData.getString(PLUGIN_TYPE);
                if (!TYPE_NATIVE.equals(pluginType)) {
                    Log.e(LOGTAG, "Unrecognized plugin type: " + pluginType);
                    continue;
                }

                try {
                    Class<?> cls = getPluginClass(serviceInfo.packageName, serviceInfo.name);

                    
                    boolean classFound = true;

                    if (!classFound) {
                        Log.e(LOGTAG, "The plugin's class' " + serviceInfo.name + "' does not extend the appropriate class.");
                        continue;
                    }

                } catch (NameNotFoundException e) {
                    Log.e(LOGTAG, "Can't find plugin: " + serviceInfo.packageName);
                    continue;
                } catch (ClassNotFoundException e) {
                    Log.e(LOGTAG, "Can't find plugin's class: " + serviceInfo.name);
                    continue;
                }

                
                mPackageInfoCache.add(pkgInfo);
                directories.add(directory);
            }
        }

        return directories.toArray(new String[directories.size()]);
    }

    static String getPluginPackage(String pluginLib) {

        if (pluginLib == null || pluginLib.length() == 0) {
            return null;
        }

        synchronized(mPackageInfoCache) {
            for (PackageInfo pkgInfo : mPackageInfoCache) {
                if (pluginLib.contains(pkgInfo.packageName)) {
                    return pkgInfo.packageName;
                }
            }
        }

        return null;
    }

    static Class<?> getPluginClass(String packageName, String className)
            throws NameNotFoundException, ClassNotFoundException {
        Context pluginContext = getContext().createPackageContext(packageName,
                Context.CONTEXT_INCLUDE_CODE |
                Context.CONTEXT_IGNORE_SECURITY);
        ClassLoader pluginCL = pluginContext.getClassLoader();
        return pluginCL.loadClass(className);
    }

    @WrapElementForJNI(allowMultithread = true)
    public static Class<?> loadPluginClass(String className, String libName) {
        if (getGeckoInterface() == null)
            return null;
        try {
            final String packageName = getPluginPackage(libName);
            final int contextFlags = Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY;
            final Context pluginContext = getContext().createPackageContext(packageName, contextFlags);
            return pluginContext.getClassLoader().loadClass(className);
        } catch (java.lang.ClassNotFoundException cnfe) {
            Log.w(LOGTAG, "Couldn't find plugin class " + className, cnfe);
            return null;
        } catch (android.content.pm.PackageManager.NameNotFoundException nnfe) {
            Log.w(LOGTAG, "Couldn't find package.", nnfe);
            return null;
        }
    }

    private static ContextGetter sContextGetter;

    @WrapElementForJNI(allowMultithread = true)
    public static Context getContext() {
        return sContextGetter.getContext();
    }

    public static void setContextGetter(ContextGetter cg) {
        sContextGetter = cg;
    }

    public static SharedPreferences getSharedPreferences() {
        if (sContextGetter == null) {
            throw new IllegalStateException("No ContextGetter; cannot fetch prefs.");
        }
        return sContextGetter.getSharedPreferences();
    }

    public interface AppStateListener {
        public void onPause();
        public void onResume();
        public void onOrientationChanged();
    }

    public interface GeckoInterface {
        public GeckoProfile getProfile();
        public PromptService getPromptService();
        public Activity getActivity();
        public String getDefaultUAString();
        public LocationListener getLocationListener();
        public SensorEventListener getSensorEventListener();
        public void doRestart();
        public void setFullScreen(boolean fullscreen);
        public void addPluginView(View view, final RectF rect, final boolean isFullScreen);
        public void removePluginView(final View view, final boolean isFullScreen);
        public void enableCameraView();
        public void disableCameraView();
        public void addAppStateListener(AppStateListener listener);
        public void removeAppStateListener(AppStateListener listener);
        public View getCameraView();
        public void notifyWakeLockChanged(String topic, String state);
        public FormAssistPopup getFormAssistPopup();
        public boolean areTabsShown();
        public AbsoluteLayout getPluginContainer();
        public void notifyCheckUpdateResult(String result);
        public void invalidateOptionsMenu();
    };

    private static GeckoInterface sGeckoInterface;

    public static GeckoInterface getGeckoInterface() {
        return sGeckoInterface;
    }

    public static void setGeckoInterface(GeckoInterface aGeckoInterface) {
        sGeckoInterface = aGeckoInterface;
    }

    public static android.hardware.Camera sCamera;

    static native void cameraCallbackBridge(byte[] data);

    static final int kPreferredFPS = 25;
    static byte[] sCameraBuffer;


    @WrapElementForJNI(stubName = "InitCameraWrapper")
    static int[] initCamera(String aContentType, int aCamera, int aWidth, int aHeight) {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (getGeckoInterface() != null)
                            getGeckoInterface().enableCameraView();
                    } catch (Exception e) {}
                }
            });

        
        
        
        
        int[] result = new int[4];
        result[0] = 0;

        if (android.hardware.Camera.getNumberOfCameras() == 0) {
            return result;
        }

        try {
            sCamera = android.hardware.Camera.open(aCamera);

            android.hardware.Camera.Parameters params = sCamera.getParameters();
            params.setPreviewFormat(ImageFormat.NV21);

            
            int fpsDelta = 1000;
            try {
                Iterator<Integer> it = params.getSupportedPreviewFrameRates().iterator();
                while (it.hasNext()) {
                    int nFps = it.next();
                    if (Math.abs(nFps - kPreferredFPS) < fpsDelta) {
                        fpsDelta = Math.abs(nFps - kPreferredFPS);
                        params.setPreviewFrameRate(nFps);
                    }
                }
            } catch(Exception e) {
                params.setPreviewFrameRate(kPreferredFPS);
            }

            
            Iterator<android.hardware.Camera.Size> sit = params.getSupportedPreviewSizes().iterator();
            int sizeDelta = 10000000;
            int bufferSize = 0;
            while (sit.hasNext()) {
                android.hardware.Camera.Size size = sit.next();
                if (Math.abs(size.width * size.height - aWidth * aHeight) < sizeDelta) {
                    sizeDelta = Math.abs(size.width * size.height - aWidth * aHeight);
                    params.setPreviewSize(size.width, size.height);
                    bufferSize = size.width * size.height;
                }
            }

            try {
                if (getGeckoInterface() != null) {
                    View cameraView = getGeckoInterface().getCameraView();
                    if (cameraView instanceof SurfaceView) {
                        sCamera.setPreviewDisplay(((SurfaceView)cameraView).getHolder());
                    } else if (cameraView instanceof TextureView) {
                        sCamera.setPreviewTexture(((TextureView)cameraView).getSurfaceTexture());
                    }
                }
            } catch (IOException | RuntimeException e) {
                Log.w(LOGTAG, "Error setPreviewXXX:", e);
            }

            sCamera.setParameters(params);
            sCameraBuffer = new byte[(bufferSize * 12) / 8];
            sCamera.addCallbackBuffer(sCameraBuffer);
            sCamera.setPreviewCallbackWithBuffer(new android.hardware.Camera.PreviewCallback() {
                @Override
                public void onPreviewFrame(byte[] data, android.hardware.Camera camera) {
                    cameraCallbackBridge(data);
                    if (sCamera != null)
                        sCamera.addCallbackBuffer(sCameraBuffer);
                }
            });
            sCamera.startPreview();
            params = sCamera.getParameters();
            result[0] = 1;
            result[1] = params.getPreviewSize().width;
            result[2] = params.getPreviewSize().height;
            result[3] = params.getPreviewFrameRate();
        } catch(RuntimeException e) {
            Log.w(LOGTAG, "initCamera RuntimeException.", e);
            result[0] = result[1] = result[2] = result[3] = 0;
        }
        return result;
    }

    @WrapElementForJNI
    static synchronized void closeCamera() {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (getGeckoInterface() != null)
                            getGeckoInterface().disableCameraView();
                    } catch (Exception e) {}
                }
            });
        if (sCamera != null) {
            sCamera.stopPreview();
            sCamera.release();
            sCamera = null;
            sCameraBuffer = null;
        }
    }

    


    @WrapElementForJNI
    public static void enableBatteryNotifications() {
        GeckoBatteryManager.enableNotifications();
    }

    @WrapElementForJNI(stubName = "HandleGeckoMessageWrapper")
    public static void handleGeckoMessage(final NativeJSContainer message) {
        EventDispatcher.getInstance().dispatchEvent(message);
        message.dispose();
    }

    @WrapElementForJNI
    public static void disableBatteryNotifications() {
        GeckoBatteryManager.disableNotifications();
    }

    @WrapElementForJNI(stubName = "GetCurrentBatteryInformationWrapper")
    public static double[] getCurrentBatteryInformation() {
        return GeckoBatteryManager.getCurrentInformation();
    }

    @WrapElementForJNI(stubName = "CheckURIVisited")
    static void checkUriVisited(String uri) {
        GlobalHistory.getInstance().checkUriVisited(uri);
    }

    @WrapElementForJNI(stubName = "MarkURIVisited")
    static void markUriVisited(final String uri) {
        final Context context = getContext();
        final BrowserDB db = GeckoProfile.get(context).getDB();
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                GlobalHistory.getInstance().add(context, db, uri);
            }
        });
    }

    @WrapElementForJNI(stubName = "SetURITitle")
    static void setUriTitle(final String uri, final String title) {
        final Context context = getContext();
        final BrowserDB db = GeckoProfile.get(context).getDB();
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                GlobalHistory.getInstance().update(context.getContentResolver(), db, uri, title);
            }
        });
    }

    @WrapElementForJNI
    static void hideProgressDialog() {
        
    }

    


    @WrapElementForJNI(stubName = "SendMessageWrapper")
    public static void sendMessage(String aNumber, String aMessage, int aRequestId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().send(aNumber, aMessage, aRequestId);
    }

    @WrapElementForJNI(stubName = "GetMessageWrapper")
    public static void getMessage(int aMessageId, int aRequestId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().getMessage(aMessageId, aRequestId);
    }

    @WrapElementForJNI(stubName = "DeleteMessageWrapper")
    public static void deleteMessage(int aMessageId, int aRequestId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().deleteMessage(aMessageId, aRequestId);
    }

    @WrapElementForJNI(stubName = "CreateMessageListWrapper")
    public static void createMessageList(long aStartDate, long aEndDate, String[] aNumbers, int aNumbersCount, String aDelivery, boolean aHasRead, boolean aRead, long aThreadId, boolean aReverse, int aRequestId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().createMessageList(aStartDate, aEndDate, aNumbers, aNumbersCount, aDelivery, aHasRead, aRead, aThreadId, aReverse, aRequestId);
    }

    @WrapElementForJNI(stubName = "GetNextMessageInListWrapper")
    public static void getNextMessageInList(int aListId, int aRequestId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().getNextMessageInList(aListId, aRequestId);
    }

    @WrapElementForJNI
    public static void clearMessageList(int aListId) {
        if (!SmsManager.isEnabled()) {
            return;
        }

        SmsManager.getInstance().clearMessageList(aListId);
    }

    
    @WrapElementForJNI
    @RobocopTarget
    public static boolean isTablet() {
        return HardwareUtils.isTablet();
    }

    public static void viewSizeChanged() {
        LayerView v = getLayerView();
        if (v != null && v.isIMEEnabled()) {
            sendEventToGecko(GeckoEvent.createBroadcastEvent(
                    "ScrollTo:FocusedInput", ""));
        }
    }

    @WrapElementForJNI(stubName = "GetCurrentNetworkInformationWrapper")
    public static double[] getCurrentNetworkInformation() {
        return GeckoNetworkManager.getInstance().getCurrentInformation();
    }

    @WrapElementForJNI
    public static void enableNetworkNotifications() {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                GeckoNetworkManager.getInstance().enableNotifications();
            }
        });
    }

    @WrapElementForJNI
    public static void disableNetworkNotifications() {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                GeckoNetworkManager.getInstance().disableNotifications();
            }
        });
    }

    






    public static byte[] decodeBase64(String s, int flags) {
        return Base64.decode(s.getBytes(), flags);
    }

    @WrapElementForJNI(stubName = "GetScreenOrientationWrapper")
    public static short getScreenOrientation() {
        return GeckoScreenOrientation.getInstance().getScreenOrientation().value;
    }

    @WrapElementForJNI
    public static void enableScreenOrientationNotifications() {
        GeckoScreenOrientation.getInstance().enableNotifications();
    }

    @WrapElementForJNI
    public static void disableScreenOrientationNotifications() {
        GeckoScreenOrientation.getInstance().disableNotifications();
    }

    @WrapElementForJNI
    public static void lockScreenOrientation(int aOrientation) {
        GeckoScreenOrientation.getInstance().lock(aOrientation);
    }

    @WrapElementForJNI
    public static void unlockScreenOrientation() {
        GeckoScreenOrientation.getInstance().unlock();
    }

    @WrapElementForJNI
    public static boolean pumpMessageLoop() {
        Handler geckoHandler = ThreadUtils.sGeckoHandler;
        Message msg = getNextMessageFromQueue(ThreadUtils.sGeckoQueue);

        if (msg == null) {
            return false;
        }

        if (msg.obj == geckoHandler && msg.getTarget() == geckoHandler) {
            
            return false;
        }

        if (msg.getTarget() == null) {
            Looper.myLooper().quit();
        } else {
            msg.getTarget().dispatchMessage(msg);
        }

        return true;
    }

    @WrapElementForJNI
    public static void notifyWakeLockChanged(String topic, String state) {
        if (getGeckoInterface() != null)
            getGeckoInterface().notifyWakeLockChanged(topic, state);
    }

    @WrapElementForJNI(allowMultithread = true)
    public static void registerSurfaceTextureFrameListener(Object surfaceTexture, final int id) {
        ((SurfaceTexture)surfaceTexture).setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                GeckoAppShell.onSurfaceTextureFrameAvailable(surfaceTexture, id);
            }
        });
    }

    @WrapElementForJNI(allowMultithread = true)
    public static void unregisterSurfaceTextureFrameListener(Object surfaceTexture) {
        ((SurfaceTexture)surfaceTexture).setOnFrameAvailableListener(null);
    }

    @WrapElementForJNI
    public static boolean unlockProfile() {
        
        GeckoAppShell.killAnyZombies();

        
        if (getGeckoInterface() != null) {
            GeckoProfile profile = getGeckoInterface().getProfile();
            File lock = profile.getFile(".parentlock");
            return lock.exists() && lock.delete();
        }
        return false;
    }

    @WrapElementForJNI(stubName = "GetProxyForURIWrapper")
    public static String getProxyForURI(String spec, String scheme, String host, int port) {
        final ProxySelector ps = new ProxySelector();

        Proxy proxy = ps.select(scheme, host);
        if (Proxy.NO_PROXY.equals(proxy)) {
            return "DIRECT";
        }
        
        switch (proxy.type()) {
            case HTTP:
                return "PROXY " + proxy.address().toString();
            case SOCKS:
                return "SOCKS " + proxy.address().toString();
        }

        return "DIRECT";
    }

    

    public static void downloadImageForIntent(final Intent intent) {
        final String src = ContextUtils.getStringExtra(intent, Intent.EXTRA_TEXT);
        if (src == null) {
            showImageShareFailureToast();
            return;
        }

        final File dir = GeckoApp.getTempDirectory();

        if (dir == null) {
            showImageShareFailureToast();
            return;
        }

        GeckoApp.deleteTempFiles();

        String type = intent.getType();
        OutputStream os = null;
        try {
            
            if (src.startsWith("data:")) {
                final int dataStart = src.indexOf(",");

                String extension = MimeTypeMap.getSingleton().getExtensionFromMimeType(type);

                
                if (TextUtils.isEmpty(extension) && dataStart > 5) {
                    type = src.substring(5, dataStart).replace(";base64", "");
                    extension = MimeTypeMap.getSingleton().getExtensionFromMimeType(type);
                }

                final File imageFile = File.createTempFile("image", "." + extension, dir);
                os = new FileOutputStream(imageFile);

                byte[] buf = Base64.decode(src.substring(dataStart + 1), Base64.DEFAULT);
                os.write(buf);

                
                intent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(imageFile));
            } else {
                InputStream is = null;
                try {
                    final byte[] buf = new byte[2048];
                    final URL url = new URL(src);
                    final String filename = URLUtil.guessFileName(src, null, type);
                    is = url.openStream();

                    final File imageFile = new File(dir, filename);
                    os = new FileOutputStream(imageFile);

                    int length;
                    while ((length = is.read(buf)) != -1) {
                        os.write(buf, 0, length);
                    }

                    
                    intent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(imageFile));
                } finally {
                    safeStreamClose(is);
                }
            }
        } catch(IOException ex) {
            
        } finally {
            safeStreamClose(os);
        }
    }

    
    private static final void showImageShareFailureToast() {
        Toast toast = Toast.makeText(getContext(),
                                     getContext().getResources().getString(R.string.share_image_failed),
                                     Toast.LENGTH_SHORT);
        toast.show();
    }

    @WrapElementForJNI(allowMultithread = true)
    static InputStream createInputStream(URLConnection connection) throws IOException {
        return connection.getInputStream();
    }

    @WrapElementForJNI(allowMultithread = true, narrowChars = true)
    static URLConnection getConnection(String url) {
        try {
            String spec;
            if (url.startsWith("android://")) {
                spec = url.substring(10);
            } else {
                spec = url.substring(8);
            }

            
            int colon = spec.indexOf(':');
            if (colon == -1 || colon > spec.indexOf('/')) {
                spec = spec.replaceFirst("/", ":/");
            }
        } catch(Exception ex) {
            return null;
        }
        return null;
    }

    @WrapElementForJNI(allowMultithread = true, narrowChars = true)
    static String connectionGetMimeType(URLConnection connection) {
        return connection.getContentType();
    }

    





    @WrapElementForJNI
    static String getExternalPublicDirectory(final String type) {
        final String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state) &&
            !Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            
            return null;
        }

        if ("sdcard".equals(type)) {
            
            return Environment.getExternalStorageDirectory().getAbsolutePath();
        }

        final String systemType;
        if ("downloads".equals(type)) {
            systemType = Environment.DIRECTORY_DOWNLOADS;
        } else if ("pictures".equals(type)) {
            systemType = Environment.DIRECTORY_PICTURES;
        } else if ("videos".equals(type)) {
            systemType = Environment.DIRECTORY_MOVIES;
        } else if ("music".equals(type)) {
            systemType = Environment.DIRECTORY_MUSIC;
        } else {
            return null;
        }
        return Environment.getExternalStoragePublicDirectory(systemType).getAbsolutePath();
    }

    @WrapElementForJNI
    static int getMaxTouchPoints() {
        PackageManager pm = getContext().getPackageManager();
        if (pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH_JAZZHAND)) {
            
            return 5;
        } else if (pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH_DISTINCT)) {
            
            return 2;
        } else if (pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH)) {
            
            return 2;
        } else if (pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN)) {
            
            return 1;
        }
        return 0;
    }
}
