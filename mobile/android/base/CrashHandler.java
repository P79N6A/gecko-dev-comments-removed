




package org.mozilla.gecko;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.UUID;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;

class CrashHandler implements Thread.UncaughtExceptionHandler {

    private static final String LOGTAG = "GeckoCrashHandler";
    private static final Thread MAIN_THREAD = Thread.currentThread();
    private static final String DEFAULT_SERVER_URL =
        "https://crash-reports.mozilla.com/submit?id=%1$s&version=%2$s&buildid=%3$s";

    
    protected final Context appContext;
    
    protected final Thread handlerThread;
    protected final Thread.UncaughtExceptionHandler systemUncaughtHandler;

    protected boolean crashing;
    protected boolean unregistered;

    





    public static Throwable getRootException(Throwable exc) {
        for (Throwable cause = exc; cause != null; cause = cause.getCause()) {
            exc = cause;
        }
        return exc;
    }

    





    public static String getExceptionStackTrace(final Throwable exc) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        exc.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }

    


    public static void terminateProcess() {
        Process.killProcess(Process.myPid());
    }

    


    public CrashHandler() {
        this((Context) null);
    }

    




    public CrashHandler(final Context appContext) {
        this.appContext = appContext;
        this.handlerThread = null;
        this.systemUncaughtHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(this);
    }

    




    public CrashHandler(final Thread thread) {
        this(thread, null);
    }

    





    public CrashHandler(final Thread thread, final Context appContext) {
        this.appContext = appContext;
        this.handlerThread = thread;
        this.systemUncaughtHandler = thread.getUncaughtExceptionHandler();
        thread.setUncaughtExceptionHandler(this);
    }

    


    public void unregister() {
        unregistered = true;

        
        
        

        if (handlerThread != null) {
            if (handlerThread.getUncaughtExceptionHandler() == this) {
                handlerThread.setUncaughtExceptionHandler(systemUncaughtHandler);
            }
        } else {
            if (Thread.getDefaultUncaughtExceptionHandler() == this) {
                Thread.setDefaultUncaughtExceptionHandler(systemUncaughtHandler);
            }
        }
    }

    





    protected void logException(final Thread thread, final Throwable exc) {
        try {
            Log.e(LOGTAG, ">>> REPORTING UNCAUGHT EXCEPTION FROM THREAD "
                          + thread.getId() + " (\"" + thread.getName() + "\")", exc);

            if (MAIN_THREAD != thread) {
                Log.e(LOGTAG, "Main thread (" + MAIN_THREAD.getId() + ") stack:");
                for (StackTraceElement ste : MAIN_THREAD.getStackTrace()) {
                    Log.e(LOGTAG, "    " + ste.toString());
                }
            }
        } catch (final Throwable e) {
            
            
        }
    }

    private static long getCrashTime() {
        return System.currentTimeMillis() / 1000;
    }

    private static long getStartupTime() {
        
        final long uptimeMins = (new File("/proc/self/cmdline")).lastModified();
        if (uptimeMins == 0L) {
            return getCrashTime();
        }
        return uptimeMins / 1000;
    }

    private static String getJavaPackageName() {
        return CrashHandler.class.getPackage().getName();
    }

    protected String getAppPackageName() {
        final Context context = getAppContext();

        if (context != null) {
            return context.getPackageName();
        }

        try {
            
            final FileReader reader = new FileReader("/proc/self/cmdline");
            final char[] buffer = new char[64];
            try {
                if (reader.read(buffer) > 0) {
                    
                    final int nul = Arrays.asList(buffer).indexOf('\0');
                    return (new String(buffer, 0, nul < 0 ? buffer.length : nul)).trim();
                }
            } finally {
                reader.close();
            }

        } catch (final IOException e) {
            Log.i(LOGTAG, "Error reading package name", e);
        }

        
        return getJavaPackageName();
    }

    protected Context getAppContext() {
        return appContext;
    }

    






    protected Bundle getCrashExtras(final Thread thread, final Throwable exc) {
        final Context context = getAppContext();
        final Bundle extras = new Bundle();
        final String pkgName = getAppPackageName();

        extras.putString("ProductName", pkgName);
        extras.putLong("CrashTime", getCrashTime());
        extras.putLong("StartupTime", getStartupTime());

        if (context != null) {
            final PackageManager pkgMgr = context.getPackageManager();
            try {
                final PackageInfo pkgInfo = pkgMgr.getPackageInfo(pkgName, 0);
                extras.putString("Version", pkgInfo.versionName);
                extras.putInt("BuildID", pkgInfo.versionCode);
                extras.putLong("InstallTime", pkgInfo.firstInstallTime / 1000);
            } catch (final PackageManager.NameNotFoundException e) {
                Log.i(LOGTAG, "Error getting package info", e);
            }
        }

        extras.putString("JavaStackTrace", getExceptionStackTrace(exc));
        return extras;
    }

    






    protected byte[] getCrashDump(final Thread thread, final Throwable exc) {
        return new byte[0]; 
    }

    protected static String normalizeUrlString(final String str) {
        if (str == null) {
            return "";
        }
        return Uri.encode(str);
    }

    




    protected String getServerUrl(final Bundle extras) {
        return String.format(DEFAULT_SERVER_URL,
            normalizeUrlString(extras.getString("ProductID")),
            normalizeUrlString(extras.getString("Version")),
            normalizeUrlString(extras.getString("BuildID")));
    }

    






    protected boolean launchCrashReporter(final String dumpFile, final String extraFile) {
        try {
            final Context context = getAppContext();
            final String javaPkg = getJavaPackageName();
            final String pkg = getAppPackageName();
            final String component = javaPkg + ".CrashReporter";
            final String action = javaPkg + ".reportCrash";
            final ProcessBuilder pb;

            if (context != null) {
                final Intent intent = new Intent(action);
                intent.setComponent(new ComponentName(pkg, component));
                intent.putExtra("minidumpPath", dumpFile);
                context.startActivity(intent);
                return true;
            }

            
            
            if (Build.VERSION.SDK_INT < 17) {
                pb = new ProcessBuilder(
                    "/system/bin/am", "start",
                    "-a", action,
                    "-n", pkg + '/' + component,
                    "--es", "minidumpPath", dumpFile);
            } else {
                pb = new ProcessBuilder(
                    "/system/bin/am", "start",
                    "--user",  "-3",
                    "-a", action,
                    "-n", pkg + '/' + component,
                    "--es", "minidumpPath", dumpFile);
            }

            pb.start().waitFor();

        } catch (final IOException e) {
            Log.e(LOGTAG, "Error launching crash reporter", e);
            return false;

        } catch (final InterruptedException e) {
            Log.i(LOGTAG, "Interrupted while waiting to launch crash reporter", e);
            
        }
        return true;
    }

    






    protected boolean reportException(final Thread thread, final Throwable exc) {
        final Context context = getAppContext();
        final String id = UUID.randomUUID().toString();

        
        final File dir;
        if (context != null) {
            dir = context.getCacheDir();
        } else {
            dir = new File("/data/data/" + getAppPackageName() + "/cache");
        }

        dir.mkdirs();
        if (!dir.exists()) {
            return false;
        }

        final File dmpFile = new File(dir, id + ".dmp");
        final File extraFile = new File(dir, id + ".extra");

        try {
            

            final byte[] minidump = getCrashDump(thread, exc);
            final FileOutputStream dmpStream = new FileOutputStream(dmpFile);
            try {
                dmpStream.write(minidump);
            } finally {
                dmpStream.close();
            }

        } catch (final IOException e) {
            Log.e(LOGTAG, "Error writing minidump file", e);
            return false;
        }

        try {
            

            final Bundle extras = getCrashExtras(thread, exc);
            final String url = getServerUrl(extras);
            extras.putString("ServerURL", url);

            final BufferedWriter extraWriter = new BufferedWriter(new FileWriter(extraFile));
            try {
                for (String key : extras.keySet()) {
                    
                    extraWriter.write(key);
                    extraWriter.write('=');
                    extraWriter.write(String.valueOf(extras.get(key)).replace("\n", "\\n"));
                    extraWriter.write('\n');
                }
            } finally {
                extraWriter.close();
            }

        } catch (final IOException e) {
            Log.e(LOGTAG, "Error writing extra file", e);
            return false;
        }

        return launchCrashReporter(dmpFile.getAbsolutePath(), extraFile.getAbsolutePath());
    }

    





    @Override
    public void uncaughtException(Thread thread, Throwable exc) {
        if (this.crashing) {
            
            return;
        }

        if (thread == null) {
            
            thread = Thread.currentThread();
        }

        try {
            if (!this.unregistered) {
                

                this.crashing = true;
                exc = getRootException(exc);
                logException(thread, exc);

                if (reportException(thread, exc)) {
                    
                    return;
                }
            }

            if (systemUncaughtHandler != null) {
                
                systemUncaughtHandler.uncaughtException(thread, exc);
            }
        } finally {
            terminateProcess();
        }
    }
}
