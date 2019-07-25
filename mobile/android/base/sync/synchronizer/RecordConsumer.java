




































package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;






class RecordConsumer implements Runnable {
  private static final String LOG_TAG = "RecordConsumer";
  private boolean stopEventually = false;
  private boolean stopImmediately = false;
  private RecordsConsumerDelegate delegate;
  private long counter = 0;

  public RecordConsumer(RecordsConsumerDelegate delegate) {
    this.delegate = delegate;
  }

  private Object monitor = new Object();
  public void doNotify() {
    synchronized (monitor) {
      monitor.notify();
    }
  }

  private static void info(String message) {
    System.out.println("INFO: " + message);
    Log.i(LOG_TAG, message);
  }

  private static void warn(String message, Exception ex) {
    System.out.println("WARN: " + message);
    Log.w(LOG_TAG, message, ex);
  }

  private static void debug(String message) {
    System.out.println("DEBUG: " + message);
    Log.d(LOG_TAG, message);
  }

  public void stop(boolean immediately) {
    debug("Called stop(" + immediately + ").");
    synchronized (monitor) {
      debug("stop() took monitor.");
      this.stopEventually = true;
      this.stopImmediately = immediately;
      monitor.notify();
      debug("stop() dropped monitor.");
    }
  }

  private Object storeSerializer = new Object();
  public void stored() {
    debug("Record stored. Notifying.");
    synchronized (storeSerializer) {
      debug("stored() took storeSerializer.");
      counter++;
      storeSerializer.notify();
      debug("stored() dropped storeSerializer.");
    }
  }
  private void storeSerially(Record record) {
    debug("New record to store.");
    synchronized (storeSerializer) {
      debug("storeSerially() took storeSerializer.");
      debug("Storing...");
      try {
        this.delegate.store(record);
      } catch (Exception e) {
        warn("Got exception in store. Not waiting.", e);
        return;      
      }
      try {
        storeSerializer.wait();
      } catch (InterruptedException e) {
        
      }
      debug("storeSerially() dropped storeSerializer.");
    }
  }

  private void consumerIsDone() {
    info("Consumer is done. Processed " + counter + ((counter == 1) ? " record." : " records."));
    delegate.consumerIsDone();
  }

  @Override
  public void run() {
    while (true) {
      synchronized (monitor) {
        debug("run() took monitor.");
        if (stopImmediately) {
          debug("Stopping immediately. Clearing queue.");
          delegate.getQueue().clear();
          debug("Notifying consumer.");
          consumerIsDone();
          return;
        }
        debug("run() dropped monitor.");
      }
      
      while (!delegate.getQueue().isEmpty()) {
        debug("Grabbing record...");
        Record record = delegate.getQueue().remove();
        
        
        debug("Invoking storeSerially...");
        this.storeSerially(record);
        debug("Done with record.");
      }
      synchronized (monitor) {
        debug("run() took monitor.");

        if (stopEventually) {
          debug("Done with records and told to stop. Notifying consumer.");
          consumerIsDone();
          return;
        }
        try {
          debug("Not told to stop but no records. Waiting.");
          monitor.wait(10000);
        } catch (InterruptedException e) {
          
        }
        debug("run() dropped monitor.");
      }
    }
  }
}