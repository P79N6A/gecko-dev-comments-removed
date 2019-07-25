




































package org.mozilla.gecko.sync.synchronizer;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;









































class RecordsChannel implements
  RepositorySessionFetchRecordsDelegate,
  RepositorySessionStoreDelegate,
  RecordsConsumerDelegate,
  RepositorySessionBeginDelegate {

  private static final String LOG_TAG = "RecordsChannel";
  public RepositorySession source;
  public RepositorySession sink;
  private RecordsChannelDelegate delegate;
  private long timestamp;
  private long end = -1;                     

  public RecordsChannel(RepositorySession source, RepositorySession sink, RecordsChannelDelegate delegate) {
    this.source    = source;
    this.sink      = sink;
    this.delegate  = delegate;
    this.timestamp = source.lastSyncTimestamp;
  }

  








  private RecordConsumer consumer;
  private boolean waitingForQueueDone = false;
  private ConcurrentLinkedQueue<Record> toProcess = new ConcurrentLinkedQueue<Record>();

  @Override
  public ConcurrentLinkedQueue<Record> getQueue() {
    return toProcess;
  }

  protected boolean isReady() {
    return source.isActive() && sink.isActive();
  }

  


  public void abort() {
    if (source.isActive()) {
      source.abort();
    }
    if (sink.isActive()) {
      sink.abort();
    }
  }

  


  public void flow() {
    if (!isReady()) {
      RepositorySession failed = source;
      if (source.isActive()) {
        failed = sink;
      }
      this.delegate.onFlowBeginFailed(this, new SessionNotBegunException(failed));
    }
    sink.setStoreDelegate(this);
    
    this.consumer = new ConcurrentRecordConsumer(this);
    ThreadPool.run(this.consumer);
    waitingForQueueDone = true;
    source.fetchSince(timestamp, this);
  }

  


  public void beginAndFlow() {
    Logger.info(LOG_TAG, "Beginning source.");
    source.begin(this);
  }

  @Override
  public void store(Record record) {
    try {
      sink.store(record);
    } catch (NoStoreDelegateException e) {
      Logger.error(LOG_TAG, "Got NoStoreDelegateException in RecordsChannel.store(). This should not occur. Aborting.", e);
      delegate.onFlowStoreFailed(this, e);
      this.abort();
    }
  }

  @Override
  public void onFetchFailed(Exception ex, Record record) {
    Logger.warn(LOG_TAG, "onFetchFailed. Calling for immediate stop.", ex);
    this.consumer.halt();
  }

  @Override
  public void onFetchedRecord(Record record) {
    this.toProcess.add(record);
    this.consumer.doNotify();
  }

  @Override
  public void onFetchSucceeded(Record[] records, long end) {
    for (Record record : records) {
      this.toProcess.add(record);
    }
    this.consumer.doNotify();
    this.onFetchCompleted(end);
  }

  @Override
  public void onFetchCompleted(long end) {
    Logger.info(LOG_TAG, "onFetchCompleted. Stopping consumer once stores are done.");
    Logger.info(LOG_TAG, "Fetch timestamp is " + end);
    this.end = end;
    this.consumer.queueFilled();
  }

  @Override
  public void onRecordStoreFailed(Exception ex) {
    this.consumer.stored();
    delegate.onFlowStoreFailed(this, ex);
    
  }

  @Override
  public void onRecordStoreSucceeded(Record record) {
    this.consumer.stored();
  }


  @Override
  public void consumerIsDone(boolean allRecordsQueued) {
    Logger.trace(LOG_TAG, "Consumer is done. Are we waiting for it? " + waitingForQueueDone);
    if (waitingForQueueDone) {
      waitingForQueueDone = false;
      this.sink.storeDone();                 
    }
  }

  @Override
  public void onStoreCompleted() {
    Logger.info(LOG_TAG, "onStoreCompleted. Notifying delegate of onFlowCompleted. End is " + end);
    
    delegate.onFlowCompleted(this, end);
  }

  @Override
  public void onBeginFailed(Exception ex) {
    delegate.onFlowBeginFailed(this, ex);
  }

  @Override
  public void onBeginSucceeded(RepositorySession session) {
    if (session == source) {
      Logger.info(LOG_TAG, "Source session began. Beginning sink session.");
      sink.begin(this);
    }
    if (session == sink) {
      Logger.info(LOG_TAG, "Sink session began. Beginning flow.");
      this.flow();
      return;
    }

    
  }

  @Override
  public RepositorySessionStoreDelegate deferredStoreDelegate(final ExecutorService executor) {
    return new DeferredRepositorySessionStoreDelegate(this, executor);
  }

  @Override
  public RepositorySessionBeginDelegate deferredBeginDelegate(final ExecutorService executor) {
    return new DeferredRepositorySessionBeginDelegate(this, executor);
  }

  @Override
  public RepositorySessionFetchRecordsDelegate deferredFetchDelegate(ExecutorService executor) {
    
    return this;
  }
}
