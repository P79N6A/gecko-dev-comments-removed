




package org.mozilla.gecko;

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.Reader;
import java.nio.charset.Charset;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.util.regex.Pattern;
import java.util.UUID;

public final class ANRReporter extends BroadcastReceiver
{
    private static final boolean DEBUG = false;
    private static final String LOGTAG = "GeckoANRReporter";

    private static final String ANR_ACTION = "android.intent.action.ANR";
    
    private static final int LINES_TO_IDENTIFY_TRACES = 10;
    
    
    
    private static final int TRACES_LINE_SIZE = 100;
    
    private static final int TRACES_BLOCK_SIZE = 2000;
    
    
    
    
    private static final Charset TRACES_CHARSET = Charset.forName("us-ascii");
    private static final Charset PING_CHARSET = Charset.forName("us-ascii");

    private static final ANRReporter sInstance = new ANRReporter();
    private static int sRegisteredCount;
    private Handler mHandler;

    public static void register(Context context) {
        if (sRegisteredCount++ != 0) {
            
            return;
        }
        sInstance.start(context);
    }

    public static void unregister() {
        if (sRegisteredCount == 0) {
            Log.w(LOGTAG, "register/unregister mismatch");
            return;
        }
        if (--sRegisteredCount != 0) {
            
            return;
        }
        sInstance.stop();
    }

    private void start(final Context context) {

        Thread receiverThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                synchronized (ANRReporter.this) {
                    mHandler = new Handler();
                    ANRReporter.this.notify();
                }
                if (DEBUG) {
                    Log.d(LOGTAG, "registering receiver");
                }
                context.registerReceiver(ANRReporter.this,
                                         new IntentFilter(ANR_ACTION),
                                         null,
                                         mHandler);
                Looper.loop();

                if (DEBUG) {
                    Log.d(LOGTAG, "unregistering receiver");
                }
                context.unregisterReceiver(ANRReporter.this);
                mHandler = null;
            }
        }, LOGTAG);

        receiverThread.setDaemon(true);
        receiverThread.start();
    }

    private void stop() {
        synchronized (this) {
            while (mHandler == null) {
                try {
                    wait(1000);
                    if (mHandler == null) {
                        
                        
                        Log.w(LOGTAG, "timed out waiting for handler");
                        return;
                    }
                } catch (InterruptedException e) {
                }
            }
        }
        Looper looper = mHandler.getLooper();
        looper.quit();
        try {
            looper.getThread().join();
        } catch (InterruptedException e) {
        }
    }

    private ANRReporter() {
    }

    
    private static File getTracesFile() {
        try {
            
            Process propProc = (new ProcessBuilder())
                .command("/system/bin/getprop", "dalvik.vm.stack-trace-file")
                .redirectErrorStream(true)
                .start();
            try {
                BufferedReader buf = new BufferedReader(
                    new InputStreamReader(propProc.getInputStream()), TRACES_LINE_SIZE);
                String propVal = buf.readLine();
                if (DEBUG) {
                    Log.d(LOGTAG, "getprop returned " + String.valueOf(propVal));
                }
                
                
                if (propVal != null && propVal.length() != 0) {
                    File tracesFile = new File(propVal);
                    if (tracesFile.isFile() && tracesFile.canRead()) {
                        return tracesFile;
                    } else if (DEBUG) {
                        Log.d(LOGTAG, "cannot access traces file");
                    }
                } else if (DEBUG) {
                    Log.d(LOGTAG, "empty getprop result");
                }
            } finally {
                propProc.destroy();
            }
        } catch (IOException e) {
            Log.w(LOGTAG, e);
        }
        
        File tracesFile = new File("/data/anr/traces.txt");
        if (tracesFile.isFile() && tracesFile.canRead()) {
            return tracesFile;
        }
        return null;
    }

    private static File getPingFile() {
        if (GeckoApp.mAppContext == null) {
            return null;
        }
        GeckoProfile profile = GeckoApp.mAppContext.getProfile();
        if (profile == null) {
            return null;
        }
        File profDir = profile.getDir();
        if (profDir == null) {
            return null;
        }
        File pingDir = new File(profDir, "saved-telemetry-pings");
        pingDir.mkdirs();
        if (!(pingDir.exists() && pingDir.isDirectory())) {
            return null;
        }
        return new File(pingDir, UUID.randomUUID().toString());
    }

    
    private static boolean isGeckoTraces(String pkgName, File tracesFile) {
        try {
            
            Pattern pkgPattern = Pattern.compile(Pattern.quote(pkgName) + "([^a-zA-Z0-9_]|$)");
            if (DEBUG) {
                Log.d(LOGTAG, "trying to match package: " + pkgName);
            }
            BufferedReader traces = new BufferedReader(
                new FileReader(tracesFile), TRACES_BLOCK_SIZE);
            try {
                for (int count = 0; count < LINES_TO_IDENTIFY_TRACES; count++) {
                    String line = traces.readLine();
                    if (DEBUG) {
                        Log.d(LOGTAG, "identifying line: " + String.valueOf(line));
                    }
                    if (pkgPattern.matcher(line).find()) {
                        
                        return true;
                    }
                }
            } finally {
                traces.close();
            }
        } catch (IOException e) {
            
        }
        return false;
    }

    private static long getUptimeMins() {

        long uptimeMins = (new File("/proc/self/stat")).lastModified();
        if (uptimeMins != 0L) {
            uptimeMins = (System.currentTimeMillis() - uptimeMins) / 1000L / 60L;
            if (DEBUG) {
                Log.d(LOGTAG, "uptime " + String.valueOf(uptimeMins));
            }
            return uptimeMins;
        } else if (DEBUG) {
            Log.d(LOGTAG, "could not get uptime");
        }
        return 0L;
    }

    private static long getTotalMem() {

        if (Build.VERSION.SDK_INT >= 16 && GeckoApp.mAppContext != null) {
            ActivityManager am = (ActivityManager)
                GeckoApp.mAppContext.getSystemService(Context.ACTIVITY_SERVICE);
            ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
            am.getMemoryInfo(mi);
            mi.totalMem /= 1024L * 1024L;
            if (DEBUG) {
                Log.d(LOGTAG, "totalMem " + String.valueOf(mi.totalMem));
            }
            return mi.totalMem;
        } else if (DEBUG) {
            Log.d(LOGTAG, "totalMem unavailable");
        }
        return 0L;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DEBUG) {
            Log.d(LOGTAG, "receiving " + String.valueOf(intent));
        }
        if (!ANR_ACTION.equals(intent.getAction())) {
            return;
        }
        Log.i(LOGTAG, "processing Gecko ANR");
    }
}
