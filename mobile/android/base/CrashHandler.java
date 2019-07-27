




package org.mozilla.gecko;

import java.io.PrintWriter;
import java.io.StringWriter;

import android.os.Process;
import android.util.Log;

class CrashHandler implements Thread.UncaughtExceptionHandler {

    private static final String LOGTAG = "GeckoCrashHandler";
    private static final Thread MAIN_THREAD = Thread.currentThread();

    
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

    






    protected boolean reportException(final Thread thread, final Throwable exc) {
        
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
