




package org.mozilla.gecko;

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
