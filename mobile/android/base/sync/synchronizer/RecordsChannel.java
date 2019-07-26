



package org.mozilla.gecko.sync.synchronizer;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;









































public class RecordsChannel implements
  RepositorySessionFetchRecordsDelegate,
  RepositorySessionStoreDelegate,
  RecordsConsumerDelegate,
  RepositorySessionBeginDelegate {

  private static final String LOG_TAG = "RecordsChannel";
  public RepositorySession source;
  public RepositorySession sink;
  private RecordsChannelDelegate delegate;
  private long timestamp;
  private long fetchEnd = -1;

  protected final AtomicInteger numFetched = new AtomicInteger();
  protected final AtomicInteger numFetchFailed = new AtomicInteger();
  protected final AtomicInteger numStored = new AtomicInteger();
  protected final AtomicInteger numStoreFailed = new AtomicInteger();

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

  




  public int getFetchCount() {
    return numFetched.get();
  }

  




  public int getFetchFailureCount() {
    return numFetchFailed.get();
  }

  




  public int getStoreCount() {
    return numStored.get();
  }

  




  public int getStoreFailureCount() {
    return numStoreFailed.get();
  }

  


  public void flow() {
    if (!isReady()) {
      RepositorySession failed = source;
      if (source.isActive()) {
        failed = sink;
      }
      this.delegate.onFlowBeginFailed(this, new SessionNotBegunException(failed));
      return;
    }
    sink.setStoreDelegate(this);
    numFetched.set(0);
    numFetchFailed.set(0);
    numStored.set(0);
    numStoreFailed.set(0);
    
    this.consumer = new ConcurrentRecordConsumer(this);
    ThreadPool.run(this.consumer);
    waitingForQueueDone = true;
    source.fetchSince(timestamp, this);
  }

  



  public void beginAndFlow() throws InvalidSessionTransitionException {
    Logger.trace(LOG_TAG, "Beginning source.");
    source.begin(this);
  }

  @Override
  public void store(Record record) {
    numStored.incrementAndGet();
    try {
      sink.store(record);
    } catch (NoStoreDelegateException e) {
      Logger.error(LOG_TAG, "Got NoStoreDelegateException in RecordsChannel.store(). This should not occur. Aborting.", e);
      delegate.onFlowStoreFailed(this, e, record.guid);
    }
  }

  @Override
  public void onFetchFailed(Exception ex, Record record) {
    Logger.warn(LOG_TAG, "onFetchFailed. Calling for immediate stop.", ex);
    numFetchFailed.incrementAndGet();
    this.consumer.halt();
    delegate.onFlowFetchFailed(this, ex);
  }

  @Override
  public void onFetchedRecord(Record record) {
    numFetched.incrementAndGet();
    this.toProcess.add(record);
    this.consumer.doNotify();
  }

  @Override
  public void onFetchCompleted(final long fetchEnd) {
    Logger.trace(LOG_TAG, "onFetchCompleted. Stopping consumer once stores are done.");
    Logger.trace(LOG_TAG, "Fetch timestamp is " + fetchEnd);
    this.fetchEnd = fetchEnd;
    this.consumer.queueFilled();
  }

  @Override
  public void onRecordStoreFailed(Exception ex, String recordGuid) {
    Logger.trace(LOG_TAG, "Failed to store record with guid " + recordGuid);
    numStoreFailed.incrementAndGet();
    this.consumer.stored();
    delegate.onFlowStoreFailed(this, ex, recordGuid);
    
  }

  @Override
  public void onRecordStoreSucceeded(String guid) {
    Logger.trace(LOG_TAG, "Stored record with guid " + guid);
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
  public void onStoreCompleted(long storeEnd) {
    Logger.trace(LOG_TAG, "onStoreCompleted. Notifying delegate of onFlowCompleted. " +
                          "Fetch end is " + fetchEnd + ", store end is " + storeEnd);
    
    delegate.onFlowCompleted(this, fetchEnd, storeEnd);
  }

  @Override
  public void onBeginFailed(Exception ex) {
    delegate.onFlowBeginFailed(this, ex);
  }

  @Override
  public void onBeginSucceeded(RepositorySession session) {
    if (session == source) {
      Logger.trace(LOG_TAG, "Source session began. Beginning sink session.");
      try {
        sink.begin(this);
      } catch (InvalidSessionTransitionException e) {
        onBeginFailed(e);
        return;
      }
    }
    if (session == sink) {
      Logger.trace(LOG_TAG, "Sink session began. Beginning flow.");
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
