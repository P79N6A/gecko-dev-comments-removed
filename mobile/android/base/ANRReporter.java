




package org.mozilla.gecko;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.util.UUID;
import java.util.regex.Pattern;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public final class ANRReporter extends BroadcastReceiver
{
    private static final boolean DEBUG = false;
    private static final String LOGTAG = "GeckoANRReporter";

    private static final String ANR_ACTION = "android.intent.action.ANR";
    
    private static final int LINES_TO_IDENTIFY_TRACES = 10;
    
    
    
    private static final int TRACES_LINE_SIZE = 100;
    
    private static final int TRACES_BLOCK_SIZE = 2000;
    private static final String TRACES_CHARSET = "utf-8";
    private static final String PING_CHARSET = "utf-8";

    private static final ANRReporter sInstance = new ANRReporter();
    private static int sRegisteredCount;
    private Handler mHandler;
    private volatile boolean mPendingANR;

    private static native boolean requestNativeStack(boolean unwind);
    private static native String getNativeStack();
    private static native void releaseNativeStack();

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
        
        File tracesFile = new File("/data/anr/traces.txt");
        if (tracesFile.isFile() && tracesFile.canRead()) {
            return tracesFile;
        }

        
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
                    tracesFile = new File(propVal);
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
        } catch (ClassCastException e) {
            Log.w(LOGTAG, e); 
        }
        return null;
    }

    private static File getPingFile() {
        if (GeckoAppShell.getContext() == null) {
            return null;
        }
        GeckoProfile profile = GeckoAppShell.getGeckoInterface().getProfile();
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
            final String END_OF_PACKAGE_NAME = "([^a-zA-Z0-9_]|$)";
            
            Pattern pkgPattern = Pattern.compile(Pattern.quote(pkgName) + END_OF_PACKAGE_NAME);
            Pattern mangledPattern = null;
            if (!AppConstants.MANGLED_ANDROID_PACKAGE_NAME.equals(pkgName)) {
                mangledPattern = Pattern.compile(Pattern.quote(
                    AppConstants.MANGLED_ANDROID_PACKAGE_NAME) + END_OF_PACKAGE_NAME);
            }
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
                    if (line == null) {
                        if (DEBUG) {
                            Log.d(LOGTAG, "reached end of traces file");
                        }
                        return false;
                    }
                    if (pkgPattern.matcher(line).find()) {
                        
                        return true;
                    }
                    if (mangledPattern != null && mangledPattern.matcher(line).find()) {
                        
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
        }
        if (DEBUG) {
            Log.d(LOGTAG, "could not get uptime");
        }
        return 0L;
    }

    






















    private static int writePingPayload(OutputStream ping,
                                        String payload) throws IOException {
        byte [] data = payload.getBytes(PING_CHARSET);
        ping.write(data);
        return data.length;
    }

    private static void fillPingHeader(OutputStream ping, String slug)
            throws IOException {

        
        byte [] data = ("{" +
            "\"reason\":\"android-anr-report\"," +
            "\"slug\":" + JSONObject.quote(slug) + "," +
            "\"payload\":").getBytes(PING_CHARSET);
        ping.write(data);
        if (DEBUG) {
            Log.d(LOGTAG, "wrote ping header, size = " + String.valueOf(data.length));
        }

        
        int size = writePingPayload(ping, ("{" +
            "\"ver\":1," +
            "\"simpleMeasurements\":{" +
                "\"uptime\":" + String.valueOf(getUptimeMins()) +
            "}," +
            "\"info\":{" +
                "\"reason\":\"android-anr-report\"," +
                "\"OS\":" + JSONObject.quote(SysInfo.getName()) + "," +
                "\"version\":\"" + String.valueOf(SysInfo.getVersion()) + "\"," +
                "\"appID\":" + JSONObject.quote(AppConstants.MOZ_APP_ID) + "," +
                "\"appVersion\":" + JSONObject.quote(AppConstants.MOZ_APP_VERSION)+ "," +
                "\"appName\":" + JSONObject.quote(AppConstants.MOZ_APP_BASENAME) + "," +
                "\"appBuildID\":" + JSONObject.quote(AppConstants.MOZ_APP_BUILDID) + "," +
                "\"appUpdateChannel\":" + JSONObject.quote(AppConstants.MOZ_UPDATE_CHANNEL) + "," +
                
                "\"platformBuildID\":" + JSONObject.quote(AppConstants.MOZ_APP_BUILDID) + "," +
                "\"locale\":" + JSONObject.quote(SysInfo.getLocale()) + "," +
                "\"cpucount\":" + String.valueOf(SysInfo.getCPUCount()) + "," +
                "\"memsize\":" + String.valueOf(SysInfo.getMemSize()) + "," +
                "\"arch\":" + JSONObject.quote(SysInfo.getArchABI()) + "," +
                "\"kernel_version\":" + JSONObject.quote(SysInfo.getKernelVersion()) + "," +
                "\"device\":" + JSONObject.quote(SysInfo.getDevice()) + "," +
                "\"manufacturer\":" + JSONObject.quote(SysInfo.getManufacturer()) + "," +
                "\"hardware\":" + JSONObject.quote(SysInfo.getHardware()) +
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
        if (prevIndex < 0) {
            
            if (block.startsWith(pattern.substring(-prevIndex, pattern.length()))) {
                
                return pattern.length() + prevIndex;
            }
            
        }
        
        int index = block.indexOf(pattern);
        if (index >= 0) {
            
            return index + pattern.length();
        }
        
        
        
        for (index = block.length() - pattern.length() + 1; index < block.length(); index++) {
            
            if (block.charAt(index) == pattern.charAt(0) &&
                block.endsWith(pattern.substring(0, block.length() - index))) {
                
                
                return index - block.length();
            }
        }
        return 0;
    }

    
    
    private static int fillPingBlock(OutputStream ping,
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
            total += writePingPayload(ping, quoted.substring(1, quoted.length() - 1));
            if (endIndex > 0) {
                
                break;
            }
        }
        return total;
    }

    private static void fillLogcat(final OutputStream ping) {
        if (Versions.preJB) {
            
            return;
        }

        try {
            
            Process proc = (new ProcessBuilder())
                .command("/system/bin/logcat", "-v", "threadtime", "-t", "200", "-d", "*:D")
                .redirectErrorStream(true)
                .start();
            try {
                Reader procOut = new InputStreamReader(proc.getInputStream(), TRACES_CHARSET);
                int size = fillPingBlock(ping, procOut, null);
                if (DEBUG) {
                    Log.d(LOGTAG, "wrote logcat, size = " + String.valueOf(size));
                }
            } finally {
                proc.destroy();
            }
        } catch (IOException e) {
            
            Log.w(LOGTAG, e);
        }
    }

    private static void fillPingFooter(OutputStream ping,
                                       boolean haveNativeStack)
            throws IOException {

        

        int total = writePingPayload(ping, ("\"," +
                "\"androidLogcat\":\""));
        fillLogcat(ping);

        if (haveNativeStack) {
            total += writePingPayload(ping, ("\"," +
                    "\"androidNativeStack\":"));

            String nativeStack = String.valueOf(getNativeStack());
            int size = writePingPayload(ping, nativeStack);
            if (DEBUG) {
                Log.d(LOGTAG, "wrote native stack, size = " + String.valueOf(size));
            }
            total += size + writePingPayload(ping, "}");
        } else {
            total += writePingPayload(ping, "\"}");
        }

        byte [] data = (
            "}").getBytes(PING_CHARSET);
        ping.write(data);
        if (DEBUG) {
            Log.d(LOGTAG, "wrote ping footer, size = " + String.valueOf(data.length + total));
        }
    }

    private static void processTraces(Reader traces, File pingFile) {

        
        boolean haveNativeStack = requestNativeStack(
             SysInfo.getMemSize() >= 640);
        try {
            OutputStream ping = new BufferedOutputStream(
                new FileOutputStream(pingFile), TRACES_BLOCK_SIZE);
            try {
                fillPingHeader(ping, pingFile.getName());
                
                
                
                
                
                
                
                
                
                
                
                int size = fillPingBlock(ping, traces, "\n----- end");
                if (DEBUG) {
                    Log.d(LOGTAG, "wrote traces, size = " + String.valueOf(size));
                }
                fillPingFooter(ping, haveNativeStack);
                if (DEBUG) {
                    Log.d(LOGTAG, "finished creating ping file");
                }
                return;
            } finally {
                ping.close();
                if (haveNativeStack) {
                    releaseNativeStack();
                }
            }
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
