




package org.mozilla.gecko;

import org.mozilla.gecko.util.ThreadUtils;

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
    private volatile boolean mPendingANR;

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

    






















    private static int writePingPayload(OutputStream ping,
                                        MessageDigest checksum,
                                        String payload) throws IOException {

        byte [] data = payload.getBytes(PING_CHARSET);
        checksum.update(data);

        data = JSONObject.quote(payload).getBytes(PING_CHARSET);
        
        ping.write(data, 1, data.length - 2);
        return data.length - 2;
    }

    private static void fillPingHeader(OutputStream ping, MessageDigest checksum, String slug)
            throws IOException {

        
        byte [] data = ("{" +
            "\"slug\":" + JSONObject.quote(slug) + "," +
            "\"payload\":\"").getBytes(PING_CHARSET);
        ping.write(data);
        if (DEBUG) {
            Log.d(LOGTAG, "wrote ping header, size = " + String.valueOf(data.length));
        }

        
        int size = writePingPayload(ping, checksum, ("{" +
            "\"ver\":1," +
            "\"simpleMeasurements\":{" +
                "\"uptime\":" + String.valueOf(getUptimeMins()) +
            "}," +
            "\"info\":{" +
                "\"reason\":\"android-anr-report\"," +
                "\"OS\":" + JSONObject.quote(GeckoAppInfo.getOS()) + "," +
                "\"version\":\"" + String.valueOf(Build.VERSION.SDK_INT) + "\"," +
                "\"appID\":" + JSONObject.quote(GeckoAppInfo.getID()) + "," +
                "\"appVersion\":" + JSONObject.quote(GeckoAppInfo.getVersion()) + "," +
                "\"appName\":" + JSONObject.quote(GeckoAppInfo.getName()) + "," +
                "\"appBuildID\":" + JSONObject.quote(GeckoAppInfo.getBuildID()) + "," +
                "\"appUpdateChannel\":" + JSONObject.quote(GeckoAppInfo.getUpdateChannel()) + "," +
                "\"platformBuildID\":" + JSONObject.quote(GeckoAppInfo.getPlatformBuildID()) + "," +
                "\"locale\":" + JSONObject.quote(GeckoAppInfo.getLocale()) + "," +
                "\"cpucount\":" + String.valueOf(Runtime.getRuntime().availableProcessors()) + "," +
                "\"memsize\":" + String.valueOf(getTotalMem()) + "," +
                "\"arch\":" + JSONObject.quote(System.getProperty("os.arch")) + "," +
                "\"kernel_version\":" + JSONObject.quote(System.getProperty("os.version")) + "," +
                "\"device\":" + JSONObject.quote(Build.MODEL) + "," +
                "\"manufacturer\":" + JSONObject.quote(Build.MANUFACTURER) + "," +
                "\"hardware\":" + JSONObject.quote(Build.VERSION.SDK_INT < 8 ? "" :
                                                   Build.HARDWARE) +
            "}," +
            "\"androidANR\":\""));
        if (DEBUG) {
            Log.d(LOGTAG, "wrote metadata, size = " + String.valueOf(size));
        }

        
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    private static int getEndPatternIndex(String block, String pattern, int prevIndex) {
        if (pattern == null || block.length() < pattern.length()) {
            
            return 0;
        }
        if (prevIndex == 0) {
            
            int index = block.indexOf(pattern);
            if (index >= 0) {
                
                return index + pattern.length();
            }
            
            
            
            
            for (index = block.length() - 1; index > block.length() - pattern.length(); index--) {
                
                if (block.charAt(index) == pattern.charAt(0) &&
                    block.endsWith(pattern.substring(0, block.length() - index))) {
                    
                    
                    return index - block.length();
                }
            }
        } else if (prevIndex < 0) {
            
            if (block.startsWith(pattern.substring(-prevIndex, pattern.length()))) {
                
                return pattern.length() + prevIndex;
            }
        }
        return 0;
    }

    
    
    private static int fillPingBlock(OutputStream ping, MessageDigest checksum,
                                     Reader reader, String endPattern)
            throws IOException {

        int total = 0;
        int endIndex = 0;
        char [] block = new char[TRACES_BLOCK_SIZE];
        for (int size = reader.read(block); size >= 0; size = reader.read(block)) {
            String stringBlock = new String(block, 0, size);
            endIndex = getEndPatternIndex(stringBlock, endPattern, endIndex);
            if (endIndex > 0) {
                
                stringBlock = stringBlock.substring(0, endIndex);
            }
            String quoted = JSONObject.quote(stringBlock);
            total += writePingPayload(ping, checksum, quoted.substring(1, quoted.length() - 1));
            if (endIndex > 0) {
                
                break;
            }
        }
        return total;
    }

    private static void fillPingFooter(OutputStream ping, MessageDigest checksum)
            throws IOException {

        

        int total = writePingPayload(ping, checksum, ("\"," +
                "\"androidLogcat\":\""));

        try {
            
            Process proc = (new ProcessBuilder())
                .command("/system/bin/logcat", "-v", "threadtime", "-t", "200", "-d", "*:D")
                .redirectErrorStream(true)
                .start();
            try {
                Reader procOut = new InputStreamReader(proc.getInputStream(), TRACES_CHARSET);
                int size = fillPingBlock(ping, checksum, procOut, null);
                if (DEBUG) {
                    Log.d(LOGTAG, "wrote logcat, size = " + String.valueOf(size));
                }
            } finally {
                proc.destroy();
            }
        } catch (IOException e) {
            
            Log.w(LOGTAG, e);
        }

        total += writePingPayload(ping, checksum, "\"}");

        String base64Checksum = Base64.encodeToString(checksum.digest(), Base64.NO_WRAP);
        if (DEBUG) {
            Log.d(LOGTAG, "checksum: " + base64Checksum);
        }
        byte [] data = (
                "\"," +
                "\"checksum\":" + JSONObject.quote(base64Checksum) +
            "}").getBytes(PING_CHARSET);
        ping.write(data);
        if (DEBUG) {
            Log.d(LOGTAG, "wrote ping footer, size = " + String.valueOf(data.length + total));
        }
    }

    private static void processTraces(Reader traces, File pingFile) {
        try {
            OutputStream ping = new BufferedOutputStream(
                new FileOutputStream(pingFile), TRACES_BLOCK_SIZE);
            try {
                MessageDigest checksum = MessageDigest.getInstance("SHA-256");
                fillPingHeader(ping, checksum, pingFile.getName());
                
                
                
                
                
                
                
                
                
                
                
                int size = fillPingBlock(ping, checksum, traces, "\n----- end");
                if (DEBUG) {
                    Log.d(LOGTAG, "wrote traces, size = " + String.valueOf(size));
                }
                fillPingFooter(ping, checksum);
                if (DEBUG) {
                    Log.d(LOGTAG, "finished creating ping file");
                }
                return;
            } finally {
                ping.close();
            }
        } catch (GeneralSecurityException e) {
            Log.w(LOGTAG, e);
        } catch (IOException e) {
            Log.w(LOGTAG, e);
        }
        
        if (pingFile.exists()) {
            pingFile.delete();
        }
    }

    private static void processTraces(File tracesFile, File pingFile) {
        try {
            Reader traces = new InputStreamReader(
                new FileInputStream(tracesFile), TRACES_CHARSET);
            try {
                processTraces(traces, pingFile);
            } finally {
                traces.close();
            }
        } catch (IOException e) {
            Log.w(LOGTAG, e);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (mPendingANR) {
            
            if (DEBUG) {
                Log.d(LOGTAG, "skipping duplicate ANR");
            }
            return;
        }
        if (ThreadUtils.getUiHandler() != null) {
            mPendingANR = true;
            
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    
                    mPendingANR = false;
                    if (DEBUG) {
                        Log.d(LOGTAG, "yay we got unstuck!");
                    }
                }
            });
        }
        if (DEBUG) {
            Log.d(LOGTAG, "receiving " + String.valueOf(intent));
        }
        if (!ANR_ACTION.equals(intent.getAction())) {
            return;
        }

        
        File pingFile = getPingFile();
        if (DEBUG) {
            Log.d(LOGTAG, "using ping file: " + String.valueOf(pingFile));
        }
        if (pingFile == null) {
            return;
        }

        File tracesFile = getTracesFile();
        if (DEBUG) {
            Log.d(LOGTAG, "using traces file: " + String.valueOf(tracesFile));
        }
        if (tracesFile == null) {
            return;
        }

        
        if (!isGeckoTraces(context.getPackageName(), tracesFile)) {
            if (DEBUG) {
                Log.d(LOGTAG, "traces is not Gecko ANR");
            }
            return;
        }
        Log.i(LOGTAG, "processing Gecko ANR");
        processTraces(tracesFile, pingFile);
    }
}
