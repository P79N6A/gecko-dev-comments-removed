



package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.domain.Record;









class ConcurrentRecordConsumer extends RecordConsumer {
  private static final String LOG_TAG = "CRecordConsumer";

  



  protected boolean allRecordsQueued = false;
  private long counter = 0;

  public ConcurrentRecordConsumer(RecordsConsumerDelegate delegate) {
    this.delegate = delegate;
  }

  private Object monitor = new Object();
  @Override
  public void doNotify() {
    synchronized (monitor) {
      monitor.notify();
    }
  }

  @Override
  public void queueFilled() {
    Logger.debug(LOG_TAG, "Queue filled.");
    synchronized (monitor) {
      this.allRecordsQueued = true;
      monitor.notify();
    }
  }

  @Override
  public void halt() {
    synchronized (monitor) {
      this.stopImmediately = true;
      monitor.notify();
    }
  }

  private Object countMonitor = new Object();
  @Override
  public void stored() {
    Logger.trace(LOG_TAG, "Record stored. Notifying.");
    synchronized (countMonitor) {
      counter++;
    }
  }

  private void consumerIsDone() {
    Logger.debug(LOG_TAG, "Consumer is done. Processed " + counter + ((counter == 1) ? " record." : " records."));
    delegate.consumerIsDone(!allRecordsQueued);
  }

  @Override
  public void run() {
    Record record;

    while (true) {
      
      while ((record = delegate.getQueue().poll()) != null) {
        synchronized (monitor) {
          Logger.trace(LOG_TAG, "run() took monitor.");
          if (stopImmediately) {
            Logger.debug(LOG_TAG, "Stopping immediately. Clearing queue.");
            delegate.getQueue().clear();
            Logger.debug(LOG_TAG, "Notifying consumer.");
            consumerIsDone();
            return;
          }
          Logger.debug(LOG_TAG, "run() dropped monitor.");
        }

        Logger.trace(LOG_TAG, "Storing record with guid " + record.guid + ".");
        try {
          delegate.store(record);
        } catch (Exception e) {
          
          Logger.error(LOG_TAG, "Caught error in store.", e);
        }
        Logger.trace(LOG_TAG, "Done with record.");
      }
      synchronized (monitor) {
        Logger.trace(LOG_TAG, "run() took monitor.");

        if (allRecordsQueued) {
          Logger.debug(LOG_TAG, "Done with records and no more to come. Notifying consumerIsDone.");
          consumerIsDone();
          return;
        }
        if (stopImmediately) {
          Logger.debug(LOG_TAG, "Done with records and told to stop immediately. Notifying consumerIsDone.");
          consumerIsDone();
          return;
        }
        try {
          Logger.debug(LOG_TAG, "Not told to stop but no records. Waiting.");
          monitor.wait(10000);
        } catch (InterruptedException e) {
          
        }
        Logger.trace(LOG_TAG, "run() dropped monitor.");
      }
    }
  }
}
