



package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.background.common.log.Logger;








public class ConnectionMonitorThread extends Thread {
  private static final long REAP_INTERVAL = 5000;     
  private static final String LOG_TAG = "ConnectionMonitorThread";

  private volatile boolean stopping;

  @Override
  public void run() {
    try {
      while (!stopping) {
        synchronized (this) {
          wait(REAP_INTERVAL);
          BaseResource.closeExpiredConnections();
        }
      }
    } catch (InterruptedException e) {
      Logger.trace(LOG_TAG, "Interrupted.");
    }
    BaseResource.shutdownConnectionManager();
  }

  public void shutdown() {
    Logger.debug(LOG_TAG, "ConnectionMonitorThread told to shut down.");
    stopping = true;
    synchronized (this) {
      notifyAll();
    }
  }
}
