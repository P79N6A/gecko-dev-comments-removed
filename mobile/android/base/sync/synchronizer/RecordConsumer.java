




































package org.mozilla.gecko.sync.synchronizer;

public abstract class RecordConsumer implements Runnable {

  public abstract void stored();

  



  public abstract void queueFilled();
  public abstract void halt();

  public abstract void doNotify();

  protected boolean stopImmediately = false;
  protected RecordsConsumerDelegate delegate;

  public RecordConsumer() {
    super();
  }

}