




































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


class GeckoAppShell
{
    static {
        sGeckoRunning = false;
    }

    
    private GeckoAppShell() { }

    static boolean sGeckoRunning;

    static private boolean gRestartScheduled = false;

    static protected Timer mSoftKBTimer;

    

    
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
        String tmpdir = System.getProperty("java.io.tmpdir");
        if (tmpdir == null) {
          try {
            File f = Environment.getDownloadCacheDirectory();
            dalvik.system.TemporaryDirectory.setUpDirectory(f);
            tmpdir = f.getPath();
          } catch (Exception e) {
            Log.e("GeckoApp", "error setting up tmp dir" + e);
          }
        }
        GeckoAppShell.putenv("TMPDIR=" + tmpdir);
        
        
        System.loadLibrary("nspr4");
        System.loadLibrary("plc4");
        System.loadLibrary("plds4");

        
        System.loadLibrary("mozsqlite3");

        
        System.loadLibrary("nssutil3");
        System.loadLibrary("nss3");
        System.loadLibrary("ssl3");
        System.loadLibrary("smime3");

        
        System.loadLibrary("mozjs");

        
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

        
        String combinedArgs = apkPath;
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

    public static void showIME(int state) {
        GeckoApp.surfaceView.mIMEState = state;

        if (mSoftKBTimer == null) {
            mSoftKBTimer = new Timer();
            mSoftKBTimer.schedule(new TimerTask() {
                public void run() {
                    InputMethodManager imm = (InputMethodManager) 
                        GeckoApp.surfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);

                    if (GeckoApp.surfaceView.mIMEState != 0)
                        imm.showSoftInput(GeckoApp.surfaceView, 0);
                    else
                        imm.hideSoftInputFromWindow(GeckoApp.surfaceView.getWindowToken(), 0);
                    mSoftKBTimer = null;
                    
                }
            }, 200);
        }
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

    public static void returnIMEQueryResult(String result, int selectionStart, int selectionEnd) {
        GeckoApp.surfaceView.inputConnection.mSelectionStart = selectionStart;
        GeckoApp.surfaceView.inputConnection.mSelectionEnd = selectionEnd;
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
        int numAttr = 2;
        String[] ret = new String[list.size() * numAttr];
        for (int i = 0; i < list.size(); i++) {
          ret[i * numAttr] = list.get(i).loadLabel(pm).toString();
          if (list.get(i).isDefault)
              ret[i * numAttr + 1] = "default";
          else
              ret[i * numAttr + 1] = "";
        }
        return ret;
    }

    static String getMimeTypeFromExtension(String aFileExt) {
        return android.webkit.MimeTypeMap.getSingleton().getMimeTypeFromExtension(aFileExt);
    }

    static boolean openUriExternal(String aUriSpec, String aMimeType) {
        
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(android.net.Uri.parse(aUriSpec), aMimeType);
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        try {
            GeckoApp.surfaceView.getContext().startActivity(intent);
            return true;
        } catch(ActivityNotFoundException e) {
            return false;
        }
    }
}
