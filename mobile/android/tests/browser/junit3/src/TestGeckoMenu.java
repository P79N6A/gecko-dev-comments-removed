



package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.util.ThreadUtils;

public class TestGeckoMenu extends BrowserTestCase {

    private volatile Exception exception;
    private void setException(Exception e) {
        this.exception = e;
    }

    public void testMenuThreading() throws InterruptedException {
        final GeckoMenu menu = new GeckoMenu(getActivity());
        final Object semaphore = new Object();

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    menu.add("test1");
                } catch (Exception e) {
                    setException(e);
                }

                synchronized (semaphore) {
                    semaphore.notify();
                }
            }
        });
        synchronized (semaphore) {
            semaphore.wait();
        }

        
        assertNull(exception);

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    menu.add("test2");
                } catch (Exception e) {
                    setException(e);
                }

                synchronized (semaphore) {
                    semaphore.notify();
                }
            }
        }).start();

        synchronized (semaphore) {
            semaphore.wait();
        }

        if (AppConstants.RELEASE_BUILD) {
            
            assertNull(exception);
            return;
        }

        assertNotNull(exception);
        assertEquals(exception.getClass(), IllegalThreadStateException.class);
    }
}
