



package org.mozilla.gecko;

import org.mozilla.gecko.util.ThreadUtils;

public class TestGeckoBackgroundThread extends BrowserTestCase {

    private boolean finishedTest;
    private boolean ranFirstRunnable;

    public void testGeckoBackgroundThread() throws InterruptedException {

        final Thread testThread = Thread.currentThread();

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                assertFalse(ThreadUtils.isOnThread(testThread));

                
                assertTrue(ThreadUtils.isOnBackgroundThread());

                ranFirstRunnable = true;
            }
        });

        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                assertTrue(ThreadUtils.isOnBackgroundThread());

                
                assertTrue(ranFirstRunnable);

                synchronized (TestGeckoBackgroundThread.this) {
                    finishedTest = true;
                    TestGeckoBackgroundThread.this.notify();
                }
            }
        });

        synchronized (this) {
            while (!finishedTest) {
                wait();
            }
        }
    }
}
