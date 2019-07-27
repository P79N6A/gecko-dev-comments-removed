



package org.mozilla.gecko.util;

import android.os.Handler;
import android.os.Looper;

import java.util.concurrent.SynchronousQueue;

final class GeckoBackgroundThread extends Thread {
    private static final String LOOPER_NAME = "GeckoBackgroundThread";

    
    private static Handler sHandler;
    private SynchronousQueue<Handler> mHandlerQueue = new SynchronousQueue<Handler>();

    
    private GeckoBackgroundThread() {
        super();
    }

    @Override
    public void run() {
        setName(LOOPER_NAME);
        Looper.prepare();
        try {
            mHandlerQueue.put(new Handler());
        } catch (InterruptedException ie) {}

        Looper.loop();
    }

    
     static synchronized Handler getHandler() {
        if (sHandler == null) {
            GeckoBackgroundThread lt = new GeckoBackgroundThread();
            ThreadUtils.setBackgroundThread(lt);
            lt.start();
            try {
                sHandler = lt.mHandlerQueue.take();
            } catch (InterruptedException ie) {}
        }
        return sHandler;
    }

     static void post(Runnable runnable) {
        Handler handler = getHandler();
        if (handler == null) {
            throw new IllegalStateException("No handler! Must have been interrupted. Not posting.");
        }
        handler.post(runnable);
    }
}
