



package org.mozilla.gecko.sync;

import org.mozilla.gecko.background.common.log.Logger;









public class DelayedWorkTracker {
  private static final String LOG_TAG = "DelayedWorkTracker";
  protected Runnable workItem = null;
  protected int outstandingCount = 0;

  public int incrementOutstanding() {
    Logger.trace(LOG_TAG, "Incrementing outstanding.");
    synchronized(this) {
      return ++outstandingCount;
    }
  }
  public int decrementOutstanding() {
    Logger.trace(LOG_TAG, "Decrementing outstanding.");
    Runnable job = null;
    int count;
    synchronized(this) {
      if ((count = --outstandingCount) == 0 &&
          workItem != null) {
        job = workItem;
        workItem = null;
      } else {
        return count;
      }
    }
    job.run();
    
    return getOutstandingOperations();
  }
  public int getOutstandingOperations() {
    synchronized(this) {
      return outstandingCount;
    }
  }
  public void delayWorkItem(Runnable item) {
    Logger.trace(LOG_TAG, "delayWorkItem.");
    boolean runnableNow = false;
    synchronized(this) {
      Logger.trace(LOG_TAG, "outstandingCount: " + outstandingCount);
      if (outstandingCount == 0) {
        runnableNow = true;
      } else {
        if (workItem != null) {
          throw new IllegalStateException("Work item already set!");
        }
        workItem = item;
      }
    }
    if (runnableNow) {
      Logger.trace(LOG_TAG, "Running item now.");
      item.run();
    }
  }
}