




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.nio.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;
import android.location.*;

import android.util.*;
import android.content.DialogInterface; 
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;

class GeckoAppShell
{
    static {
        sGeckoRunning = false;
    }

    
    private GeckoAppShell() { }

    static boolean sGeckoRunning;

    static private boolean gRestartScheduled = false;

    static private final Timer mIMETimer = new Timer();

    static private final int NOTIFY_IME_RESETINPUTSTATE = 0;
    static private final int NOTIFY_IME_SETOPENSTATE = 1;
    static private final int NOTIFY_IME_SETENABLED = 2;
    static private final int NOTIFY_IME_CANCELCOMPOSITION = 3;
    static private final int NOTIFY_IME_FOCUSCHANGE = 4;

    

    
    public static native void nativeInit();
    public static native void nativeRun(String args);

    
    public static native void setInitialSize(int width, int height);
    public static native void setSurfaceView(GeckoSurfaceView sv);
    public static native void putenv(String map);
    public static native void onResume();

    
    public static void loadGeckoLibs() {
        
        
        

        
        System.loadLibrary("mozalloc");
        System.loadLibrary("mozutils");
                
        Intent i = GeckoApp.mAppContext.getIntent();
        String env = i.getStringExtra("env0");
        Log.i("GeckoApp", "env0: "+ env);
        for (int c = 1; env != null; c++) {
            GeckoAppShell.putenv(env);
            env = i.getStringExtra("env" + c);
            Log.i("GeckoApp", "env"+ c +": "+ env);
        }

        File f = new File("/data/data/org.mozilla." + 
                          GeckoApp.mAppContext.getAppName() +"/tmp");
        if (!f.exists())
            f.mkdirs();
        GeckoAppShell.putenv("TMPDIR=" + f.getPath());

        f = Environment.getDownloadCacheDirectory();
        GeckoAppShell.putenv("EXTERNAL_STORAGE" + f.getPath());

        
        System.loadLibrary("nspr4");
        System.loadLibrary("plc4");
        System.loadLibrary("plds4");

        
        System.loadLibrary("mozsqlite3");

        
        System.loadLibrary("nssutil3");
        System.loadLibrary("nss3");
        System.loadLibrary("ssl3");
        System.loadLibrary("smime3");

        
        System.loadLibrary("xul");

        
        System.loadLibrary("xpcom");                                          

        
        System.loadLibrary("nssckbi");
        System.loadLibrary("freebl3");
        System.loadLibrary("softokn3");
    }

    public static void runGecko(String apkPath, String args, String url) {
        
        GeckoAppShell.nativeInit();

        
        GeckoAppShell.setSurfaceView(GeckoApp.surfaceView);

        sGeckoRunning = true;

        
        String combinedArgs = apkPath + " -omnijar " + apkPath;
        if (args != null)
            combinedArgs += " " + args;
        if (url != null)
            combinedArgs += " " + url;
        
        GeckoAppShell.nativeRun(combinedArgs);
    }

    private static GeckoEvent mLastDrawEvent;

    public static void sendEventToGecko(GeckoEvent e) {
        if (sGeckoRunning)
            notifyGeckoOfEvent(e);
    }

    
    public static native void notifyGeckoOfEvent(GeckoEvent event);

    


    public static void scheduleRedraw() {
        
        scheduleRedraw(0, -1, -1, -1, -1);
    }

    public static void scheduleRedraw(int nativeWindow, int x, int y, int w, int h) {
        GeckoEvent e;

        if (x == -1) {
            e = new GeckoEvent(GeckoEvent.DRAW, null);
        } else {
            e = new GeckoEvent(GeckoEvent.DRAW, new Rect(x, y, w, h));
        }

        e.mNativeWindow = nativeWindow;

        sendEventToGecko(e);
    }

    
    private static final class IMEStateUpdater extends TimerTask
    {
        static private IMEStateUpdater instance;
        private boolean mEnable, mReset;

        static private IMEStateUpdater getInstance() {
            if (instance == null) {
                instance = new IMEStateUpdater();
                mIMETimer.schedule(instance, 200);
            }
            return instance;
        }

        static public synchronized void enableIME() {
            getInstance().mEnable = true;
        }

        static public synchronized void resetIME() {
            getInstance().mReset = true;
        }

        public void run() {
            synchronized(IMEStateUpdater.class) {
                instance = null;
            }

            InputMethodManager imm = (InputMethodManager) 
                GeckoApp.surfaceView.getContext().getSystemService(
                    Context.INPUT_METHOD_SERVICE);
            if (imm == null)
                return;

            if (mReset)
                imm.restartInput(GeckoApp.surfaceView);

            if (!mEnable)
                return;

            if (GeckoApp.surfaceView.mIMEState !=
                    GeckoSurfaceView.IME_STATE_DISABLED)
                imm.showSoftInput(GeckoApp.surfaceView, 0);
            else
                imm.hideSoftInputFromWindow(
                    GeckoApp.surfaceView.getWindowToken(), 0);
        }
    }

    public static void notifyIME(int type, int state) {
        if (GeckoApp.surfaceView == null)
            return;

        switch (type) {
        case NOTIFY_IME_RESETINPUTSTATE:
            IMEStateUpdater.resetIME();
            
            IMEStateUpdater.enableIME();
            break;

        case NOTIFY_IME_SETENABLED:
            

            GeckoApp.surfaceView.mIMEState = state;
            IMEStateUpdater.enableIME();
            break;

        case NOTIFY_IME_CANCELCOMPOSITION:
            IMEStateUpdater.resetIME();
            break;

        case NOTIFY_IME_FOCUSCHANGE:
            GeckoApp.surfaceView.mIMEFocus = state != 0;
            break;

        }
    }

    public static void notifyIMEChange(String text, int start, int end, int newEnd) {
        if (GeckoApp.surfaceView == null ||
            GeckoApp.surfaceView.inputConnection == null)
            return;

        InputMethodManager imm = (InputMethodManager) 
            GeckoApp.surfaceView.getContext().getSystemService(
                Context.INPUT_METHOD_SERVICE);
        if (imm == null)
            return;

        if (newEnd < 0)
            GeckoApp.surfaceView.inputConnection.notifySelectionChange(
                imm, start, end);
        else
            GeckoApp.surfaceView.inputConnection.notifyTextChange(
                imm, text, start, end, newEnd);
    }

    public static void enableAccelerometer(boolean enable) {
        SensorManager sm = (SensorManager) 
            GeckoApp.surfaceView.getContext().getSystemService(Context.SENSOR_SERVICE);

        if (enable) {
            Sensor accelSensor = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (accelSensor == null)
                return;

            sm.registerListener(GeckoApp.surfaceView, accelSensor, SensorManager.SENSOR_DELAY_GAME);
        } else {
            sm.unregisterListener(GeckoApp.surfaceView);
        }
    }

    public static void enableLocation(boolean enable) {
        LocationManager lm = (LocationManager)
            GeckoApp.surfaceView.getContext().getSystemService(Context.LOCATION_SERVICE);

        if (enable) {
            Criteria crit = new Criteria();
            crit.setAccuracy(Criteria.ACCURACY_FINE);
            String provider = lm.getBestProvider(crit, true);
            if (provider == null)
                return;

            sendEventToGecko(new GeckoEvent(lm.getLastKnownLocation(provider)));
            lm.requestLocationUpdates(provider, 100, (float).5, GeckoApp.surfaceView, Looper.getMainLooper());
        } else {
            lm.removeUpdates(GeckoApp.surfaceView);
        }
    }

    public static void moveTaskToBack() {
        GeckoApp.mAppContext.moveTaskToBack(true);
    }

    public static void returnIMEQueryResult(String result, int selectionStart, int selectionLength) {
        GeckoApp.surfaceView.inputConnection.mSelectionStart = selectionStart;
        GeckoApp.surfaceView.inputConnection.mSelectionLength = selectionLength;
        try {
            GeckoApp.surfaceView.inputConnection.mQueryResult.put(result);
        } catch (InterruptedException e) {
        }
    }

    static void onXreExit() {
        sGeckoRunning = false;
        Log.i("GeckoAppJava", "XRE exited");
        if (gRestartScheduled) {
            GeckoApp.mAppContext.doRestart();
        } else {
            Log.i("GeckoAppJava", "we're done, good bye");
            System.exit(0);
        }

    }
    static void scheduleRestart() {
        Log.i("GeckoAppJava", "scheduling restart");
        gRestartScheduled = true;        
    }
    
    static String[] getHandlersForMimeType(String aMimeType) {
        PackageManager pm = 
            GeckoApp.surfaceView.getContext().getPackageManager();
        Intent intent = new Intent();
        intent.setType(aMimeType);
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

    static String[] getHandlersForProtocol(String aScheme) {
        PackageManager pm = 
            GeckoApp.surfaceView.getContext().getPackageManager();
        Intent intent = new Intent();
        Uri uri = new Uri.Builder().scheme(aScheme).build();
        intent.setData(uri);
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

    static String getMimeTypeFromExtension(String aFileExt) {
        return android.webkit.MimeTypeMap.getSingleton().getMimeTypeFromExtension(aFileExt);
    }

    static boolean openUriExternal(String aUriSpec, String aMimeType, 
                                   String aPackageName, String aClassName) {
        
        Intent intent = new Intent(Intent.ACTION_VIEW);
        if (aMimeType.length() > 0)
            intent.setDataAndType(Uri.parse(aUriSpec), aMimeType);
        else
            intent.setData(Uri.parse(aUriSpec));
        if (aPackageName.length() > 0 && aClassName.length() > 0)
            intent.setClassName(aPackageName, aClassName);

        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        try {
            GeckoApp.surfaceView.getContext().startActivity(intent);
            return true;
        } catch(ActivityNotFoundException e) {
            return false;
        }
    }
}
