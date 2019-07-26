




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.GeckoLayerClient;
import org.mozilla.gecko.gfx.GfxInfoThread;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PanZoomController;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.ActivityManager;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationManager;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;
import android.os.SystemClock;
import android.os.Vibrator;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ContextThemeWrapper;
import android.view.HapticFeedbackConstants;
import android.view.Surface;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.webkit.MimeTypeMap;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.URI;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;
import java.util.concurrent.SynchronousQueue;

public class GeckoAppShell
{
    private static final String LOGTAG = "GeckoAppShell";

    
    private GeckoAppShell() { }

    static private LinkedList<GeckoEvent> gPendingEvents =
        new LinkedList<GeckoEvent>();

    static private boolean gRestartScheduled = false;

    static private GeckoEditableListener mEditableListener = null;

    static private final HashMap<String, String>
        mAlertCookies = new HashMap<String, String>();

    


    static public final int WPL_STATE_START = 0x00000001;
    static public final int WPL_STATE_STOP = 0x00000010;
    static public final int WPL_STATE_IS_DOCUMENT = 0x00020000;
    static public final int WPL_STATE_IS_NETWORK = 0x00040000;

    public static final String SHORTCUT_TYPE_WEBAPP = "webapp";
    public static final String SHORTCUT_TYPE_BOOKMARK = "bookmark";

    static private final boolean LOGGING = false;

    static private int sDensityDpi = 0;

    private static final EventDispatcher sEventDispatcher = new EventDispatcher();

    
    private static final float[] DEFAULT_LAUNCHER_ICON_HSV = { 32.0f, 1.0f, 1.0f };

    
    private static boolean sVibrationMaybePlaying = false;

    


    private static long sVibrationEndTime = 0;

    
    private static int sDefaultSensorHint = 100;

    private static Sensor gAccelerometerSensor = null;
    private static Sensor gLinearAccelerometerSensor = null;
    private static Sensor gGyroscopeSensor = null;
    private static Sensor gOrientationSensor = null;
    private static Sensor gProximitySensor = null;
    private static Sensor gLightSensor = null;

    private static boolean mLocationHighAccuracy = false;

    public static ActivityHandlerHelper sActivityHelper = new ActivityHandlerHelper();
    static NotificationClient sNotificationClient;

    

    
    public static native void nativeInit();

    
    
    public static native void setLayerClient(GeckoLayerClient client);
    public static native void onResume();
    public static native void onLowMemory();
    public static native void callObserver(String observerKey, String topic, String data);
    public static native void removeObserver(String observerKey);
    public static native void onChangeNetworkLinkStatus(String status);
    public static native Message getNextMessageFromQueue(MessageQueue queue);
    public static native void onSurfaceTextureFrameAvailable(Object surfaceTexture, int id);

    public static void registerGlobalExceptionHandler() {
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable e) {
                
                
                Throwable cause;
                while ((cause = e.getCause()) != null) {
                    e = cause;
                }

                try {
                    Log.e(LOGTAG, ">>> REPORTING UNCAUGHT EXCEPTION FROM THREAD "
                                  + thread.getId() + " (\"" + thread.getName() + "\")", e);

                    if (e instanceof OutOfMemoryError) {
                        SharedPreferences prefs =
                            GeckoApp.mAppContext.getSharedPreferences(GeckoApp.PREFS_NAME, 0);
                        SharedPreferences.Editor editor = prefs.edit();
                        editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, true);
                        editor.commit();
                    }
                } finally {
                    reportJavaCrash(getStackTraceString(e));
                }
            }
        });
    }

    private static String getStackTraceString(Throwable e) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        e.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }

    private static native void reportJavaCrash(String stackTrace);

    public static void notifyUriVisited(String uri) {
        sendEventToGecko(GeckoEvent.createVisitedEvent(uri));
    }

    public static native void processNextNativeEvent();

    public static native void notifyBatteryChange(double aLevel, boolean aCharging, double aRemainingTime);

    public static native void scheduleComposite();

    
    
    
    
    public static native void scheduleResumeComposition(int width, int height);

    public static native float computeRenderIntegrity();

    public static native SurfaceBits getSurfaceBits(Surface surface);

    public static native void onFullScreenPluginHidden(View view);

    private static final class GeckoMediaScannerClient implements MediaScannerConnectionClient {
        private final String mFile;
        private final String mMimeType;
        private MediaScannerConnection mScanner;

        public static void startScan(Context context, String file, String mimeType) {
            new GeckoMediaScannerClient(context, file, mimeType);
        }

        private GeckoMediaScannerClient(Context context, String file, String mimeType) {
            mFile = file;
            mMimeType = mimeType;
            mScanner = new MediaScannerConnection(context, this);
            mScanner.connect();
        }

        @Override
        public void onMediaScannerConnected() {
            mScanner.scanFile(mFile, mMimeType);
        }

        @Override
        public void onScanCompleted(String path, Uri uri) {
            if(path.equals(mFile)) {
                mScanner.disconnect();
                mScanner = null;
            }
        }
    }

    public static void runGecko(String apkPath, String args, String url, String type) {
        Looper.prepare();

        
        GeckoAppShell.nativeInit();

        
        GeckoAppShell.setLayerClient(GeckoApp.mAppContext.getLayerView().getLayerClient());

        
        String combinedArgs = apkPath + " -greomni " + apkPath;
        if (args != null)
            combinedArgs += " " + args;
        if (url != null)
            combinedArgs += " -url " + url;
        if (type != null)
            combinedArgs += " " + type;

        DisplayMetrics metrics = GeckoApp.mAppContext.getResources().getDisplayMetrics();
        combinedArgs += " -width " + metrics.widthPixels + " -height " + metrics.heightPixels;

        GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    geckoLoaded();
                }
            });

        
        GeckoLoader.nativeRun(combinedArgs);
    }

    
    private static void geckoLoaded() {
        GeckoEditable editable = new GeckoEditable();
        
        mEditableListener = editable;
    }

    static void sendPendingEventsToGecko() {
        try {
            while (!gPendingEvents.isEmpty()) {
                GeckoEvent e = gPendingEvents.removeFirst();
                notifyGeckoOfEvent(e);
            }
        } catch (NoSuchElementException e) {}
    }

    
    public static void sendEventToGecko(GeckoEvent e) {
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            notifyGeckoOfEvent(e);
        } else {
            gPendingEvents.addLast(e);
        }
    }

    
    public static native void notifyGeckoOfEvent(GeckoEvent event);

    


    public static void notifyIME(int type) {
        if (mEditableListener != null) {
            mEditableListener.notifyIME(type);
        }
    }

    public static void notifyIMEContext(int state, String typeHint,
                                        String modeHint, String actionHint) {
        if (mEditableListener != null) {
            mEditableListener.notifyIMEContext(state, typeHint,
                                               modeHint, actionHint);
        }
    }

    public static void notifyIMEChange(String text, int start, int end, int newEnd) {
        if (newEnd < 0) { 
            mEditableListener.onSelectionChange(start, end);
        } else { 
            mEditableListener.onTextChange(text, start, end, newEnd);
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
                try {
                    sEventAckLock.wait(100);
                } catch (InterruptedException ie) {
                }
                if (!sWaitingForEventAck) {
                    
                    break;
                }
                long waited = SystemClock.uptimeMillis() - time;
                Log.d(LOGTAG, "Gecko event sync taking too long: " + waited + "ms");
                if (isUiThread && waited >= 4000) {
                    Log.w(LOGTAG, "Gecko event sync took too long, aborting!", new Exception());
                    sWaitingForEventAck = false;
                    break;
                }
            }
        }
    }

    
    public static void acknowledgeEvent() {
        synchronized (sEventAckLock) {
            sWaitingForEventAck = false;
            sEventAckLock.notifyAll();
        }
    }

    public static void enableLocation(final boolean enable) {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    LocationManager lm = (LocationManager)
                        GeckoApp.mAppContext.getSystemService(Context.LOCATION_SERVICE);

                    if (enable) {
                        Criteria criteria = new Criteria();
                        criteria.setSpeedRequired(false);
                        criteria.setBearingRequired(false);
                        criteria.setAltitudeRequired(false);
                        if (mLocationHighAccuracy) {
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
                        Location loc = lm.getLastKnownLocation(provider);
                        if (loc != null) {
                            GeckoApp.mAppContext.onLocationChanged(loc);
                        }
                        lm.requestLocationUpdates(provider, 100, (float).5, GeckoApp.mAppContext, l);
                    } else {
                        lm.removeUpdates(GeckoApp.mAppContext);
                    }
                }
            });
    }

    public static void enableLocationHighAccuracy(final boolean enable) {
        mLocationHighAccuracy = enable;
    }

    public static void enableSensor(int aSensortype) {
        SensorManager sm = (SensorManager)
            GeckoApp.mAppContext.getSystemService(Context.SENSOR_SERVICE);

        switch(aSensortype) {
        case GeckoHalDefines.SENSOR_ORIENTATION:
            if(gOrientationSensor == null)
                gOrientationSensor = sm.getDefaultSensor(Sensor.TYPE_ORIENTATION);
            if (gOrientationSensor != null)
                sm.registerListener(GeckoApp.mAppContext, gOrientationSensor, sDefaultSensorHint);
            break;

        case GeckoHalDefines.SENSOR_ACCELERATION:
            if(gAccelerometerSensor == null)
                gAccelerometerSensor = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (gAccelerometerSensor != null)
                sm.registerListener(GeckoApp.mAppContext, gAccelerometerSensor, sDefaultSensorHint);
            break;

        case GeckoHalDefines.SENSOR_PROXIMITY:
            if(gProximitySensor == null)
                gProximitySensor = sm.getDefaultSensor(Sensor.TYPE_PROXIMITY);
            if (gProximitySensor != null)
                sm.registerListener(GeckoApp.mAppContext, gProximitySensor, SensorManager.SENSOR_DELAY_NORMAL);
            break;

        case GeckoHalDefines.SENSOR_LIGHT:
            if(gLightSensor == null)
                gLightSensor = sm.getDefaultSensor(Sensor.TYPE_LIGHT);
            if (gLightSensor != null)
                sm.registerListener(GeckoApp.mAppContext, gLightSensor, SensorManager.SENSOR_DELAY_NORMAL);
            break;

        case GeckoHalDefines.SENSOR_LINEAR_ACCELERATION:
            if(gLinearAccelerometerSensor == null)
                gLinearAccelerometerSensor = sm.getDefaultSensor(10 );
            if (gLinearAccelerometerSensor != null)
                sm.registerListener(GeckoApp.mAppContext, gLinearAccelerometerSensor, sDefaultSensorHint);
            break;

        case GeckoHalDefines.SENSOR_GYROSCOPE:
            if(gGyroscopeSensor == null)
                gGyroscopeSensor = sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            if (gGyroscopeSensor != null)
                sm.registerListener(GeckoApp.mAppContext, gGyroscopeSensor, sDefaultSensorHint);
            break;
        default:
            Log.w(LOGTAG, "Error! Can't enable unknown SENSOR type " + aSensortype);
        }
    }

    public static void disableSensor(int aSensortype) {
        SensorManager sm = (SensorManager)
            GeckoApp.mAppContext.getSystemService(Context.SENSOR_SERVICE);

        switch (aSensortype) {
        case GeckoHalDefines.SENSOR_ORIENTATION:
            if (gOrientationSensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gOrientationSensor);
            break;

        case GeckoHalDefines.SENSOR_ACCELERATION:
            if (gAccelerometerSensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gAccelerometerSensor);
            break;

        case GeckoHalDefines.SENSOR_PROXIMITY:
            if (gProximitySensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gProximitySensor);
            break;

        case GeckoHalDefines.SENSOR_LIGHT:
            if (gLightSensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gLightSensor);
            break;

        case GeckoHalDefines.SENSOR_LINEAR_ACCELERATION:
            if (gLinearAccelerometerSensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gLinearAccelerometerSensor);
            break;

        case GeckoHalDefines.SENSOR_GYROSCOPE:
            if (gGyroscopeSensor != null)
                sm.unregisterListener(GeckoApp.mAppContext, gGyroscopeSensor);
            break;
        default:
            Log.w(LOGTAG, "Error! Can't disable unknown SENSOR type " + aSensortype);
        }
    }

    public static void moveTaskToBack() {
        GeckoApp.mAppContext.moveTaskToBack(true);
    }

    public static void returnIMEQueryResult(String result, int selectionStart, int selectionLength) {
        
        
    }

    static void onXreExit() {
        
        GeckoThread.setLaunchState(GeckoThread.LaunchState.GeckoExiting);
        if (gRestartScheduled) {
            GeckoApp.mAppContext.doRestart();
        } else {
            GeckoApp.mAppContext.finish();
        }

        Log.d(LOGTAG, "Killing via System.exit()");
        System.exit(0);
    }

    static void scheduleRestart() {
        gRestartScheduled = true;
    }

    public static File preInstallWebApp(String aTitle, String aURI, String aUniqueURI) {
        int index = WebAppAllocator.getInstance(GeckoApp.mAppContext).findAndAllocateIndex(aUniqueURI, aTitle, (String) null);
        GeckoProfile profile = GeckoProfile.get(GeckoApp.mAppContext, "webapp" + index);
        return profile.getDir();
    }

    public static void postInstallWebApp(String aTitle, String aURI, String aUniqueURI, String aIconURL) {
    	WebAppAllocator allocator = WebAppAllocator.getInstance(GeckoApp.mAppContext);
		int index = allocator.getIndexForApp(aUniqueURI);
    	assert index != -1 && aIconURL != null;
    	allocator.updateAppAllocation(aUniqueURI, index, BitmapUtils.getBitmapFromDataURI(aIconURL));
    	createShortcut(aTitle, aURI, aUniqueURI, aIconURL, "webapp");
    }

    public static Intent getWebAppIntent(String aURI, String aUniqueURI, String aTitle, Bitmap aIcon) {
        int index;
        if (aIcon != null && !TextUtils.isEmpty(aTitle))
            index = WebAppAllocator.getInstance(GeckoApp.mAppContext).findAndAllocateIndex(aUniqueURI, aTitle, aIcon);
        else
            index = WebAppAllocator.getInstance(GeckoApp.mAppContext).getIndexForApp(aUniqueURI);

        if (index == -1)
            return null;

        return getWebAppIntent(index, aURI);
    }

    public static Intent getWebAppIntent(int aIndex, String aURI) {
        Intent intent = new Intent();
        intent.setAction(GeckoApp.ACTION_WEBAPP_PREFIX + aIndex);
        intent.setData(Uri.parse(aURI));
        intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                            AppConstants.ANDROID_PACKAGE_NAME + ".WebApps$WebApp" + aIndex);
        return intent;
    }

    
    
    static void createShortcut(String aTitle, String aURI, String aIconData, String aType) {
        if ("webapp".equals(aType)) {
            Log.w(LOGTAG, "createShortcut with no unique URI should not be used for aType = webapp!");
        }

        createShortcut(aTitle, aURI, aURI, aIconData, aType);
    }

    
    static void createShortcut(String aTitle, String aURI, Bitmap aBitmap, String aType) {
        createShortcut(aTitle, aURI, aURI, aBitmap, aType);
    }

    
    static void createShortcut(String aTitle, String aURI, String aUniqueURI, String aIconData, String aType) {
        createShortcut(aTitle, aURI, aUniqueURI, BitmapUtils.getBitmapFromDataURI(aIconData), aType);
    }

    public static void createShortcut(final String aTitle, final String aURI, final String aUniqueURI,
                                      final Bitmap aIcon, final String aType)
    {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                Intent shortcutIntent;
                if (aType.equalsIgnoreCase(SHORTCUT_TYPE_WEBAPP)) {
                    shortcutIntent = getWebAppIntent(aURI, aUniqueURI, aTitle, aIcon);
                } else {
                    shortcutIntent = new Intent();
                    shortcutIntent.setAction(GeckoApp.ACTION_BOOKMARK);
                    shortcutIntent.setData(Uri.parse(aURI));
                    shortcutIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                                AppConstants.BROWSER_INTENT_CLASS);
                }
        
                Intent intent = new Intent();
                intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
                if (aTitle != null)
                    intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aTitle);
                else
                    intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aURI);
                intent.putExtra(Intent.EXTRA_SHORTCUT_ICON, getLauncherIcon(aIcon, aType));

                
                intent.putExtra("duplicate", false);

                intent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
                GeckoApp.mAppContext.sendBroadcast(intent);
            }
        });
    }

    public static void removeShortcut(final String aTitle, final String aURI, final String aType) {
        removeShortcut(aTitle, aURI, null, aType);
    }

    public static void removeShortcut(final String aTitle, final String aURI, final String aUniqueURI, final String aType) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                Intent shortcutIntent;
                if (aType.equalsIgnoreCase(SHORTCUT_TYPE_WEBAPP)) {
                    int index = WebAppAllocator.getInstance(GeckoApp.mAppContext).getIndexForApp(aUniqueURI);
                    shortcutIntent = getWebAppIntent(aURI, aUniqueURI, "", null);
                    if (shortcutIntent == null)
                        return;
                } else {
                    shortcutIntent = new Intent();
                    shortcutIntent.setAction(GeckoApp.ACTION_BOOKMARK);
                    shortcutIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                                AppConstants.BROWSER_INTENT_CLASS);
                    shortcutIntent.setData(Uri.parse(aURI));
                }
        
                Intent intent = new Intent();
                intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
                if (aTitle != null)
                    intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aTitle);
                else
                    intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, aURI);

                intent.setAction("com.android.launcher.action.UNINSTALL_SHORTCUT");
                GeckoApp.mAppContext.sendBroadcast(intent);
            }
        });
    }

    public static void uninstallWebApp(final String uniqueURI) {
        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                int index = WebAppAllocator.getInstance(GeckoApp.mAppContext).releaseIndexForApp(uniqueURI);

                
                if (index == -1)
                    return;

                
                String targetProcessName = GeckoApp.mAppContext.getPackageName();
                targetProcessName = targetProcessName + ":" + targetProcessName + ".WebApp" + index;

                ActivityManager am = (ActivityManager) GeckoApp.mAppContext.getSystemService(Context.ACTIVITY_SERVICE);
                List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
                if (procs != null) {
                    for (ActivityManager.RunningAppProcessInfo proc : procs) {
                        if (proc.processName.equals(targetProcessName)) {
                            android.os.Process.killProcess(proc.pid);
                            break;
                        }
                    }
                }

                
                GeckoProfile.removeProfile(GeckoApp.mAppContext, "webapp" + index);
            }
        });
    }

    static public int getPreferredIconSize() {
        if (android.os.Build.VERSION.SDK_INT >= 11) {
            ActivityManager am = (ActivityManager)GeckoApp.mAppContext.getSystemService(Context.ACTIVITY_SERVICE);
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

    static private Bitmap getLauncherIcon(Bitmap aSource, String aType) {
        final int kOffset = 6;
        final int kRadius = 5;
        int size = getPreferredIconSize();
        int insetSize = aSource != null ? size * 2 / 3 : size;

        Bitmap bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);

        if (aType.equalsIgnoreCase(SHORTCUT_TYPE_WEBAPP)) {
            Rect iconBounds = new Rect(0, 0, size, size);
            canvas.drawBitmap(aSource, null, iconBounds, null);
            return bitmap;
        }

        
        Paint paint = new Paint();
        if (aSource == null) {
            
            paint.setColor(Color.HSVToColor(DEFAULT_LAUNCHER_ICON_HSV));
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
        } else {
            
            int color = BitmapUtils.getDominantColor(aSource);
            paint.setColor(color);
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
            paint.setColor(Color.argb(100, 255, 255, 255));
            canvas.drawRoundRect(new RectF(kOffset, kOffset, size - kOffset, size - kOffset), kRadius, kRadius, paint);
        }

        
        Bitmap overlay = BitmapFactory.decodeResource(GeckoApp.mAppContext.getResources(), R.drawable.home_bg);
        canvas.drawBitmap(overlay, null, new Rect(0, 0, size, size), null);

        
        if (aSource == null)
            aSource = BitmapFactory.decodeResource(GeckoApp.mAppContext.getResources(), R.drawable.home_star);

        
        int sWidth = insetSize / 2;
        int sHeight = sWidth;

        if (aSource.getWidth() > insetSize || aSource.getHeight() > insetSize) {
            
            
            sWidth = Math.min(size / 3, aSource.getWidth() / 2);
            sHeight = Math.min(size / 3, aSource.getHeight() / 2);
        }

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

    static String[] getHandlersForMimeType(String aMimeType, String aAction) {
        Intent intent = getIntentForActionString(aAction);
        if (aMimeType != null && aMimeType.length() > 0)
            intent.setType(aMimeType);
        return getHandlersForIntent(intent);
    }

    static String[] getHandlersForURL(String aURL, String aAction) {
        
        Uri uri = aURL.indexOf(':') >= 0 ? Uri.parse(aURL) : new Uri.Builder().scheme(aURL).build();

        Intent intent = getOpenURIIntent(GeckoApp.mAppContext, uri.toString(), "",
            TextUtils.isEmpty(aAction) ? Intent.ACTION_VIEW : aAction, "");

        return getHandlersForIntent(intent);
    }

    static boolean hasHandlersForIntent(Intent intent) {
        PackageManager pm = GeckoApp.mAppContext.getPackageManager();
        List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
        return !list.isEmpty();
    }

    static String[] getHandlersForIntent(Intent intent) {
        PackageManager pm = GeckoApp.mAppContext.getPackageManager();
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
    }

    static Intent getIntentForActionString(String aAction) {
        
        if (TextUtils.isEmpty(aAction)) {
            return new Intent(Intent.ACTION_VIEW);
        }
        return new Intent(aAction);
    }

    static String getExtensionFromMimeType(String aMimeType) {
        return MimeTypeMap.getSingleton().getExtensionFromMimeType(aMimeType);
    }

    static String getMimeTypeFromExtensions(String aFileExt) {
        MimeTypeMap mtm = MimeTypeMap.getSingleton();
        StringTokenizer st = new StringTokenizer(aFileExt, ".,; ");
        String type = null;
        String subType = null;
        while (st.hasMoreElements()) {
            String ext = st.nextToken();
            String mt = mtm.getMimeTypeFromExtension(ext);
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

    static void shareImage(String aSrc, String aType) {

        Intent intent = new Intent(Intent.ACTION_SEND);
        boolean isDataURI = aSrc.startsWith("data:");
        OutputStream os = null;
        File dir = GeckoApp.getTempDirectory();

        if (dir == null) {
            showImageShareFailureToast();
            return;
        }

        GeckoApp.deleteTempFiles();

        try {
            
            File imageFile = File.createTempFile("image",
                                                 "." + aType.replace("image/",""),
                                                 dir);
            os = new FileOutputStream(imageFile);

            if (isDataURI) {
                
                int dataStart = aSrc.indexOf(',');
                byte[] buf = Base64.decode(aSrc.substring(dataStart+1), Base64.DEFAULT);
                os.write(buf);
            } else {
                
                InputStream is = null;
                try {
                    URL url = new URL(aSrc);
                    is = url.openStream();
                    byte[] buf = new byte[2048];
                    int length;

                    while ((length = is.read(buf)) != -1) {
                        os.write(buf, 0, length);
                    }
                } finally {
                    safeStreamClose(is);
                }
            }
            intent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(imageFile));

            
            
            if (aType.startsWith("image/")) {
                intent.setType(aType);
            } else {
                intent.setType("image/*");
            }
        } catch (IOException e) {
            if (!isDataURI) {
               
               intent.putExtra(Intent.EXTRA_TEXT, aSrc);
               intent.setType("text/plain");
            } else {
               showImageShareFailureToast();
               return;
            }
        } finally {
            safeStreamClose(os);
        }
        GeckoApp.mAppContext.startActivity(Intent.createChooser(intent,
                GeckoApp.mAppContext.getResources().getString(R.string.share_title)));
    }

    
    private static final void showImageShareFailureToast() {
        Toast toast = Toast.makeText(GeckoApp.mAppContext,
                                     GeckoApp.mAppContext.getResources().getString(R.string.share_image_failed),
                                     Toast.LENGTH_SHORT);
        toast.show();
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

    











    static boolean openUriExternal(String targetURI,
                                   String mimeType,
                                   String packageName,
                                   String className,
                                   String action,
                                   String title) {
        final Context context = GeckoApp.mAppContext;
        final Intent intent = getOpenURIIntent(context, targetURI,
                                               mimeType, action, title);

        if (intent == null) {
            return false;
        }

        if (packageName.length() > 0 && className.length() > 0) {
            intent.setClassName(packageName, className);
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

    













    static Intent getOpenURIIntent(final Context context,
                                   final String targetURI,
                                   final String mimeType,
                                   final String action,
                                   final String title) {

        if (action.equalsIgnoreCase(Intent.ACTION_SEND)) {
            Intent shareIntent = getIntentForActionString(action);
            shareIntent.putExtra(Intent.EXTRA_TEXT, targetURI);
            shareIntent.putExtra(Intent.EXTRA_SUBJECT, title);

            
            
            
            shareIntent.putExtra(Intent.EXTRA_TITLE, title);

            if (mimeType != null && mimeType.length() > 0) {
                shareIntent.setType(mimeType);
            }

            return Intent.createChooser(shareIntent,
                                        context.getResources().getString(R.string.share_title)); 
        }

        final Uri uri = normalizeUriScheme(Uri.parse(targetURI));
        if (mimeType.length() > 0) {
            Intent intent = getIntentForActionString(action);
            intent.setDataAndType(uri, mimeType);
            return intent;
        }

        if (!isUriSafeForScheme(uri)) {
            return null;
        }

        final String scheme = uri.getScheme();

        final Intent intent;

        
        
        
        
        final Intent likelyIntent = getIntentForActionString(action);
        likelyIntent.setData(uri);

        if ("vnd.youtube".equals(scheme) && !hasHandlersForIntent(likelyIntent)) {
            
            
            intent = new Intent(VideoPlayer.VIDEO_ACTION);
            intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                "org.mozilla.gecko.VideoPlayer");
            intent.setData(uri);
        } else {
            intent = likelyIntent;
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

    


    @SuppressWarnings("deprecation")
    private static String getClipboardTextImpl() {
        Context context = GeckoApp.mAppContext;
        if (android.os.Build.VERSION.SDK_INT >= 11) {
            android.content.ClipboardManager cm = (android.content.ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm.hasPrimaryClip()) {
                ClipData clip = cm.getPrimaryClip();
                if (clip != null) {
                    ClipData.Item item = clip.getItemAt(0);
                    return item.coerceToText(context).toString();
                }
            }
        } else {
            android.text.ClipboardManager cm = (android.text.ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm.hasText()) {
                return cm.getText().toString();
            }
        }
        return null;
    }

    private static SynchronousQueue<String> sClipboardQueue = new SynchronousQueue<String>();
    private static String EMPTY_STRING = new String();

    static String getClipboardText() {
        
        
        

        if (ThreadUtils.isOnUiThread() || ThreadUtils.isOnBackgroundThread()) {
            return getClipboardTextImpl();
        }

        ThreadUtils.postToBackgroundThread(new Runnable() { 
            @Override
            public void run() {
                String text = getClipboardTextImpl();
                try {
                    sClipboardQueue.put(text != null ? text : EMPTY_STRING);
                } catch (InterruptedException ie) {}
            }
        });
        try {
            String ret = sClipboardQueue.take();
            return (EMPTY_STRING.equals(ret) ? null : ret);
        } catch (InterruptedException ie) {}
        return null;
    }

    static void setClipboardText(final String text) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            @SuppressWarnings("deprecation")
            public void run() {
                Context context = GeckoApp.mAppContext;
                if (android.os.Build.VERSION.SDK_INT >= 11) {
                    android.content.ClipboardManager cm = (android.content.ClipboardManager)
                        context.getSystemService(Context.CLIPBOARD_SERVICE);
                    cm.setPrimaryClip(ClipData.newPlainText("Text", text));
                } else {
                    android.text.ClipboardManager cm = (android.text.ClipboardManager)
                        context.getSystemService(Context.CLIPBOARD_SERVICE);
                    cm.setText(text);
                }
            }
        });
    }

    public static void setNotificationClient(NotificationClient client) {
        if (sNotificationClient == null) {
            sNotificationClient = client;
        } else {
            Log.d(LOGTAG, "Notification client already set");
        }
    }

    public static void showAlertNotification(String aImageUrl, String aAlertTitle, String aAlertText,
                                             String aAlertCookie, String aAlertName) {
        Log.d(LOGTAG, "GeckoAppShell.showAlertNotification\n" +
            "- image = '" + aImageUrl + "'\n" +
            "- title = '" + aAlertTitle + "'\n" +
            "- text = '" + aAlertText +"'\n" +
            "- cookie = '" + aAlertCookie +"'\n" +
            "- name = '" + aAlertName + "'");

        
        String app = GeckoApp.mAppContext.getClass().getName();
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
                GeckoApp.mAppContext, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        mAlertCookies.put(aAlertName, aAlertCookie);
        callObserver(aAlertName, "alertshow", aAlertCookie);

        sNotificationClient.add(notificationID, aImageUrl, aAlertTitle, aAlertText, contentIntent);
    }

    public static void alertsProgressListener_OnProgress(String aAlertName, long aProgress, long aProgressMax, String aAlertText) {
        int notificationID = aAlertName.hashCode();
        sNotificationClient.update(notificationID, aProgress, aProgressMax, aAlertText);

        if (aProgress == aProgressMax) {
            
            removeObserver(aAlertName);
        }
    }

    public static void closeNotification(String aAlertName) {
        String alertCookie = mAlertCookies.get(aAlertName);
        if (alertCookie != null) {
            callObserver(aAlertName, "alertfinished", alertCookie);
            mAlertCookies.remove(aAlertName);
        }

        removeObserver(aAlertName);

        int notificationID = aAlertName.hashCode();
        sNotificationClient.remove(notificationID);
    }

    public static void handleNotification(String aAction, String aAlertName, String aAlertCookie) {
        int notificationID = aAlertName.hashCode();

        if (GeckoApp.ACTION_ALERT_CALLBACK.equals(aAction)) {
            callObserver(aAlertName, "alertclickcallback", aAlertCookie);

            if (sNotificationClient.isProgressStyle(notificationID)) {
                
                return;
            }
        }

        
        
        
        
        sendEventToGecko(GeckoEvent.createBroadcastEvent("Notification:Clicked", aAlertCookie));
        closeNotification(aAlertName);
    }

    public static int getDpi() {
        if (sDensityDpi == 0) {
            sDensityDpi = GeckoApp.mAppContext.getResources().getDisplayMetrics().densityDpi;
        }

        return sDensityDpi;
    }

    public static void setFullScreen(boolean fullscreen) {
        GeckoApp.mAppContext.setFullScreen(fullscreen);
    }

    public static String showFilePickerForExtensions(String aExtensions) {
        return sActivityHelper.showFilePicker(GeckoApp.mAppContext, getMimeTypeFromExtensions(aExtensions));
    }

    public static String showFilePickerForMimeType(String aMimeType) {
        return sActivityHelper.showFilePicker(GeckoApp.mAppContext, aMimeType);
    }

    public static void performHapticFeedback(boolean aIsLongPress) {
        
        
        if (!sVibrationMaybePlaying || System.nanoTime() >= sVibrationEndTime) {
            LayerView layerView = GeckoApp.mAppContext.getLayerView();
            layerView.performHapticFeedback(aIsLongPress ?
                                            HapticFeedbackConstants.LONG_PRESS :
                                            HapticFeedbackConstants.VIRTUAL_KEY);
        }
    }

    private static Vibrator vibrator() {
        LayerView layerView = GeckoApp.mAppContext.getLayerView();
        return (Vibrator) layerView.getContext().getSystemService(Context.VIBRATOR_SERVICE);
    }

    public static void vibrate(long milliseconds) {
        sVibrationEndTime = System.nanoTime() + milliseconds * 1000000;
        sVibrationMaybePlaying = true;
        vibrator().vibrate(milliseconds);
    }

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

    public static void cancelVibrate() {
        sVibrationMaybePlaying = false;
        sVibrationEndTime = 0;
        vibrator().cancel();
    }

    public static void showInputMethodPicker() {
        InputMethodManager imm = (InputMethodManager)
            GeckoApp.mAppContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showInputMethodPicker();
    }

    public static void setKeepScreenOn(final boolean on) {
        GeckoApp.mAppContext.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                
            }
        });
    }

    public static void notifyDefaultPrevented(final boolean defaultPrevented) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                LayerView view = GeckoApp.mAppContext.getLayerView();
                PanZoomController controller = (view == null ? null : view.getPanZoomController());
                if (controller != null) {
                    controller.notifyDefaultActionPrevented(defaultPrevented);
                }
            }
        });
    }

    public static boolean isNetworkLinkUp() {
        ConnectivityManager cm = (ConnectivityManager)
            GeckoApp.mAppContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();
        if (info == null || !info.isConnected())
            return false;
        return true;
    }

    public static boolean isNetworkLinkKnown() {
        ConnectivityManager cm = (ConnectivityManager)
            GeckoApp.mAppContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm.getActiveNetworkInfo() == null)
            return false;
        return true;
    }

    public static void setSelectedLocale(String localeCode) {
        



























    }

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
            new ContextThemeWrapper(GeckoApp.mAppContext, android.R.style.TextAppearance);

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

    public static boolean checkForGeckoProcs() {

        class GeckoPidCallback implements GeckoProcessesVisitor {
            public boolean otherPidExist = false;
            @Override
            public boolean callback(int pid) {
                if (pid != android.os.Process.myPid()) {
                    otherPidExist = true;
                    return false;
                }
                return true;
            }            
        }
        GeckoPidCallback visitor = new GeckoPidCallback();            
        EnumerateGeckoProcesses(visitor);
        return visitor.otherPidExist;
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

    public static void waitForAnotherGeckoProc(){
        int countdown = 40;
        while (!checkForGeckoProcs() &&  --countdown > 0) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException ie) {}
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
            String filter = GeckoProfile.get(GeckoApp.mAppContext).getDir().toString();
            Log.i(LOGTAG, "[OPENFILE] Filter: " + filter);

            
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
                    Log.i(LOGTAG, "[OPENFILE] " + name + "(" + split[pidColumn] + ") : " + file);
            }
            in.close();
        } catch (Exception e) { }
    }

    public static void scanMedia(String aFile, String aMimeType) {
        Context context = GeckoApp.mAppContext;
        GeckoMediaScannerClient.startScan(context, aFile, aMimeType);
    }

    public static byte[] getIconForExtension(String aExt, int iconSize) {
        try {
            if (iconSize <= 0)
                iconSize = 16;

            if (aExt != null && aExt.length() > 1 && aExt.charAt(0) == '.')
                aExt = aExt.substring(1);

            PackageManager pm = GeckoApp.mAppContext.getPackageManager();
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

    private static Drawable getDrawableForExtension(PackageManager pm, String aExt) {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        MimeTypeMap mtm = MimeTypeMap.getSingleton();
        String mimeType = mtm.getMimeTypeFromExtension(aExt);
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

    public static boolean getShowPasswordSetting() {
        try {
            int showPassword =
                Settings.System.getInt(GeckoApp.mAppContext.getContentResolver(),
                                       Settings.System.TEXT_SHOW_PASSWORD, 1);
            return (showPassword > 0);
        }
        catch (Exception e) {
            return true;
        }
    }

    public static void addPluginView(View view,
                                     int x, int y,
                                     int w, int h,
                                     boolean isFullScreen) {
        GeckoApp.mAppContext.addPluginView(view, new Rect(x, y, x + w, y + h), isFullScreen);
    }

    public static void removePluginView(View view, boolean isFullScreen) {
        GeckoApp.mAppContext.removePluginView(view, isFullScreen);
    }

    public static Class<?> loadPluginClass(String className, String libName) {
        try {
            final String packageName = GeckoApp.mAppContext.getPluginPackage(libName);
            final int contextFlags = Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY;
            final Context pluginContext = GeckoApp.mAppContext.createPackageContext(packageName, contextFlags);
            return pluginContext.getClassLoader().loadClass(className);
        } catch (java.lang.ClassNotFoundException cnfe) {
            Log.w(LOGTAG, "Couldn't find plugin class " + className, cnfe);
            return null;
        } catch (android.content.pm.PackageManager.NameNotFoundException nnfe) {
            Log.w(LOGTAG, "Couldn't find package.", nnfe);
            return null;
        }
    }

    public static Context getContext() {
        return GeckoApp.mAppContext;
    }

    public static android.hardware.Camera sCamera = null;

    static native void cameraCallbackBridge(byte[] data);

    static int kPreferedFps = 25;
    static byte[] sCameraBuffer = null;

    static int[] initCamera(String aContentType, int aCamera, int aWidth, int aHeight) {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        GeckoApp.mAppContext.enableCameraView();
                    } catch (Exception e) {}
                }
            });

        
        
        
        
        int[] result = new int[4];
        result[0] = 0;

        if (Build.VERSION.SDK_INT >= 9) {
            if (android.hardware.Camera.getNumberOfCameras() == 0)
                return result;
        }

        try {
            
            if (Build.VERSION.SDK_INT >= 9)
                sCamera = android.hardware.Camera.open(aCamera);
            else
                sCamera = android.hardware.Camera.open();

            android.hardware.Camera.Parameters params = sCamera.getParameters();
            params.setPreviewFormat(ImageFormat.NV21);

            
            int fpsDelta = 1000;
            try {
                Iterator<Integer> it = params.getSupportedPreviewFrameRates().iterator();
                while (it.hasNext()) {
                    int nFps = it.next();
                    if (Math.abs(nFps - kPreferedFps) < fpsDelta) {
                        fpsDelta = Math.abs(nFps - kPreferedFps);
                        params.setPreviewFrameRate(nFps);
                    }
                }
            } catch(Exception e) {
                params.setPreviewFrameRate(kPreferedFps);
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
                sCamera.setPreviewDisplay(GeckoApp.mAppContext.cameraView.getHolder());
            } catch(IOException e) {
                Log.w(LOGTAG, "Error setPreviewDisplay:", e);
            } catch(RuntimeException e) {
                Log.w(LOGTAG, "Error setPreviewDisplay:", e);
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

    static synchronized void closeCamera() {
        ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        GeckoApp.mAppContext.disableCameraView();
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

    








    public static void registerEventListener(String event, GeckoEventListener listener) {
        sEventDispatcher.registerEventListener(event, listener);
    }

    static EventDispatcher getEventDispatcher() {
        return sEventDispatcher;
    }

    








    public static void unregisterEventListener(String event, GeckoEventListener listener) {
        sEventDispatcher.unregisterEventListener(event, listener);
    }

    


    public static void enableBatteryNotifications() {
        GeckoBatteryManager.enableNotifications();
    }

    public static String handleGeckoMessage(String message) {
        return sEventDispatcher.dispatchEvent(message);
    }

    public static void disableBatteryNotifications() {
        GeckoBatteryManager.disableNotifications();
    }

    public static double[] getCurrentBatteryInformation() {
        return GeckoBatteryManager.getCurrentInformation();
    }

    static void checkUriVisited(String uri) {   
        GlobalHistory.getInstance().checkUriVisited(uri);
    }

    static void markUriVisited(final String uri) {    
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                GlobalHistory.getInstance().add(uri);
            }
        });
    }

    static void setUriTitle(final String uri, final String title) {    
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                GlobalHistory.getInstance().update(uri, title);
            }
        });
    }

    static void hideProgressDialog() {
        
    }

    


    public static void sendMessage(String aNumber, String aMessage, int aRequestId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().send(aNumber, aMessage, aRequestId);
    }

    public static void getMessage(int aMessageId, int aRequestId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().getMessage(aMessageId, aRequestId);
    }

    public static void deleteMessage(int aMessageId, int aRequestId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().deleteMessage(aMessageId, aRequestId);
    }

    public static void createMessageList(long aStartDate, long aEndDate, String[] aNumbers, int aNumbersCount, int aDeliveryState, boolean aReverse, int aRequestId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().createMessageList(aStartDate, aEndDate, aNumbers, aNumbersCount, aDeliveryState, aReverse, aRequestId);
    }

    public static void getNextMessageInList(int aListId, int aRequestId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().getNextMessageInList(aListId, aRequestId);
    }

    public static void clearMessageList(int aListId) {
        if (SmsManager.getInstance() == null) {
            return;
        }

        SmsManager.getInstance().clearMessageList(aListId);
    }

    
    public static boolean isTablet() {
        return HardwareUtils.isTablet();
    }

    public static void viewSizeChanged() {
        LayerView v = GeckoApp.mAppContext.getLayerView();
        if (v != null && v.isIMEEnabled()) {
            sendEventToGecko(GeckoEvent.createBroadcastEvent(
                    "ScrollTo:FocusedInput", ""));
        }
    }

    public static double[] getCurrentNetworkInformation() {
        return GeckoNetworkManager.getInstance().getCurrentInformation();
    }

    public static void enableNetworkNotifications() {
        GeckoNetworkManager.getInstance().enableNotifications();
    }

    public static void disableNetworkNotifications() {
        GeckoNetworkManager.getInstance().disableNotifications();
    }

    
    public static final int BASE64_DEFAULT = 0;
    public static final int BASE64_URL_SAFE = 8;

    


    
    private static final byte[] map1 = new byte[64];
    private static final byte[] map1_urlsafe;
    static {
      int i=0;
      for (byte c='A'; c<='Z'; c++) map1[i++] = c;
      for (byte c='a'; c<='z'; c++) map1[i++] = c;
      for (byte c='0'; c<='9'; c++) map1[i++] = c;
      map1[i++] = '+'; map1[i++] = '/';
      map1_urlsafe = map1.clone();
      map1_urlsafe[62] = '-'; map1_urlsafe[63] = '_'; 
    }

    
    private static final byte[] map2 = new byte[128];
    static {
        for (int i=0; i<map2.length; i++) map2[i] = -1;
        for (int i=0; i<64; i++) map2[map1[i]] = (byte)i;
        map2['-'] = (byte)62; map2['_'] = (byte)63;
    }

    final static byte EQUALS_ASCII = (byte) '=';

    





    public static byte[] encodeBase64(byte[] in, int flags) {
        if (Build.VERSION.SDK_INT >=Build.VERSION_CODES.FROYO)
            return Base64.encode(in, flags | Base64.NO_WRAP);
        int oDataLen = (in.length*4+2)/3;       
        int oLen = ((in.length+2)/3)*4;         
        byte[] out = new byte[oLen];
        int ip = 0;
        int iEnd = in.length;
        int op = 0;
        byte[] toMap = ((flags & BASE64_URL_SAFE) == 0 ? map1 : map1_urlsafe);
        while (ip < iEnd) {
            int i0 = in[ip++] & 0xff;
            int i1 = ip < iEnd ? in[ip++] & 0xff : 0;
            int i2 = ip < iEnd ? in[ip++] & 0xff : 0;
            int o0 = i0 >>> 2;
            int o1 = ((i0 &   3) << 4) | (i1 >>> 4);
            int o2 = ((i1 & 0xf) << 2) | (i2 >>> 6);
            int o3 = i2 & 0x3F;
            out[op++] = toMap[o0];
            out[op++] = toMap[o1];
            out[op] = op < oDataLen ? toMap[o2] : EQUALS_ASCII; op++;
            out[op] = op < oDataLen ? toMap[o3] : EQUALS_ASCII; op++;
        }
        return out; 
    }

    








    public static byte[] decodeBase64(byte[] in, int flags) {
        if (Build.VERSION.SDK_INT >=Build.VERSION_CODES.FROYO)
            return Base64.decode(in, flags);
        int iOff = 0;
        int iLen = in.length;
        if (iLen%4 != 0) throw new IllegalArgumentException ("Length of Base64 encoded input string is not a multiple of 4.");
        while (iLen > 0 && in[iOff+iLen-1] == '=') iLen--;
        int oLen = (iLen*3) / 4;
        byte[] out = new byte[oLen];
        int ip = iOff;
        int iEnd = iOff + iLen;
        int op = 0;
        while (ip < iEnd) {
            int i0 = in[ip++];
            int i1 = in[ip++];
            int i2 = ip < iEnd ? in[ip++] : 'A';
            int i3 = ip < iEnd ? in[ip++] : 'A';
            if (i0 > 127 || i1 > 127 || i2 > 127 || i3 > 127)
                throw new IllegalArgumentException ("Illegal character in Base64 encoded data.");
            int b0 = map2[i0];
            int b1 = map2[i1];
            int b2 = map2[i2];
            int b3 = map2[i3];
            if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0)
                throw new IllegalArgumentException ("Illegal character in Base64 encoded data.");
            int o0 = ( b0       <<2) | (b1>>>4);
            int o1 = ((b1 & 0xf)<<4) | (b2>>>2);
            int o2 = ((b2 &   3)<<6) |  b3;
            out[op++] = (byte)o0;
            if (op<oLen) out[op++] = (byte)o1;
            if (op<oLen) out[op++] = (byte)o2; }
        return out; 
    }

    public static byte[] decodeBase64(String s, int flags) {
        return decodeBase64(s.getBytes(), flags);
    }

    public static short getScreenOrientation() {
        return GeckoScreenOrientationListener.getInstance().getScreenOrientation();
    }

    public static void enableScreenOrientationNotifications() {
        GeckoScreenOrientationListener.getInstance().enableNotifications();
    }

    public static void disableScreenOrientationNotifications() {
        GeckoScreenOrientationListener.getInstance().disableNotifications();
    }

    public static void lockScreenOrientation(int aOrientation) {
        GeckoScreenOrientationListener.getInstance().lockScreenOrientation(aOrientation);
    }

    public static void unlockScreenOrientation() {
        GeckoScreenOrientationListener.getInstance().unlockScreenOrientation();
    }

    public static boolean pumpMessageLoop() {
        MessageQueue mq = Looper.myQueue();
        Message msg = getNextMessageFromQueue(mq); 
        if (msg == null)
            return false;
        if (msg.getTarget() == null) 
            Looper.myLooper().quit();
        else
            msg.getTarget().dispatchMessage(msg);
        msg.recycle();
        return true;
    }

    static class AsyncResultHandler extends FilePickerResultHandler {
        private long mId;
        AsyncResultHandler(long id) {
            super(null);
            mId = id;
        }

        @Override
        public void onActivityResult(int resultCode, Intent data) {
            GeckoAppShell.notifyFilePickerResult(handleActivityResult(resultCode, data), mId);
        }
        
    }

    static native void notifyFilePickerResult(String filePath, long id);

    
    public static void showFilePickerAsync(String aMimeType, long id) {
        if (!sActivityHelper.showFilePicker(GeckoApp.mAppContext, aMimeType, new AsyncResultHandler(id))) {
            GeckoAppShell.notifyFilePickerResult("", id);
        }
    }

    public static void notifyWakeLockChanged(String topic, String state) {
        GeckoApp.mAppContext.notifyWakeLockChanged(topic, state);
    }

    public static String getGfxInfoData() {
        return GfxInfoThread.getData();
    }

    public static void registerSurfaceTextureFrameListener(Object surfaceTexture, final int id) {
        ((SurfaceTexture)surfaceTexture).setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                GeckoAppShell.onSurfaceTextureFrameAvailable(surfaceTexture, id);
            }
        });
    }

    public static void unregisterSurfaceTextureFrameListener(Object surfaceTexture) {
        ((SurfaceTexture)surfaceTexture).setOnFrameAvailableListener(null);
    }

    public static boolean unlockProfile() {
        
        GeckoAppShell.killAnyZombies();

        
        GeckoProfile profile = GeckoApp.mAppContext.getProfile();
        File lock = profile.getFile(".parentlock");
        return lock.exists() && lock.delete();
    }

    public static String getProxyForURI(String spec, String scheme, String host, int port) {
        URI uri = null;
        try {
            uri = new URI(spec);
        } catch(java.net.URISyntaxException uriEx) {
            try {
                uri = new URI(scheme, null, host, port, null, null, null);
            } catch(java.net.URISyntaxException uriEx2) {
                Log.d("GeckoProxy", "Failed to create uri from spec", uriEx);
                Log.d("GeckoProxy", "Failed to create uri from parts", uriEx2);
            }
        }
        if (uri != null) {
            ProxySelector ps = ProxySelector.getDefault();
            if (ps != null) {
                List<Proxy> proxies = ps.select(uri);
                if (proxies != null && !proxies.isEmpty()) {
                    Proxy proxy = proxies.get(0);
                    if (!Proxy.NO_PROXY.equals(proxy)) {
                        final String proxyStr;
                        switch (proxy.type()) {
                        case HTTP:
                            proxyStr = "PROXY " + proxy.address().toString();
                            break;
                        case SOCKS:
                            proxyStr = "SOCKS " + proxy.address().toString();
                            break;
                        case DIRECT:
                        default:
                            proxyStr = "DIRECT";
                            break;
                        }
                        return proxyStr;
                    }
                }
            }
        }
        return "DIRECT";
    }
}
