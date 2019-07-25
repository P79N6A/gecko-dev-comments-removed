




































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

import android.util.*;
import android.content.DialogInterface; 

class GeckoAppShell
{
    static {
        sGeckoRunning = false;
    }

    
    private GeckoAppShell() { }

    static boolean sGeckoRunning;

    static private boolean gRestartScheduled = false;

    

    
    public static native void nativeInit();
    public static native void nativeRun(String args);

    
    public static native void setInitialSize(int width, int height);
    public static native void setSurfaceView(GeckoSurfaceView sv);
    public static native void putenv(String map);

    
    public static void loadGeckoLibs() {
        
        
        

        
        System.loadLibrary("mozalloc");

        
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
        InputMethodManager imm = (InputMethodManager) 
            GeckoApp.surfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);

        GeckoApp.surfaceView.mIMEState = state;
        if (state != 0)
            imm.showSoftInput(GeckoApp.surfaceView, 0);
        else
            imm.hideSoftInputFromWindow(GeckoApp.surfaceView.getWindowToken(), 0);
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
}
