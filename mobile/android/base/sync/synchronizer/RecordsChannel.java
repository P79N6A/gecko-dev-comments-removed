




































package org.mozilla.gecko.sync.synchronizer;

import java.util.concurrent.ConcurrentLinkedQueue;

import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.util.Log;








class RecordsChannel implements RepositorySessionFetchRecordsDelegate, RepositorySessionStoreDelegate, RecordsConsumerDelegate, RepositorySessionBeginDelegate {
  private static final String LOG_TAG = "RecordsChannel";
  public RepositorySession source;
  public RepositorySession sink;
  private RecordsChannelDelegate delegate;
  private long timestamp;
  private long end = -1;                     

  public RecordsChannel(RepositorySession source, RepositorySession sink, RecordsChannelDelegate delegate) {
    this.source = source;
    this.sink   = sink;
    this.delegate = delegate;
    this.timestamp = source.lastSyncTimestamp;
  }

  





  private boolean waitingForQueueDone = false;
  ConcurrentLinkedQueue<Record> toProcess = new ConcurrentLinkedQueue<Record>();
  private RecordConsumer consumer;

  private void enqueue(Record record) {
    toProcess.add(record);
  }

  @Override
  public ConcurrentLinkedQueue<Record> getQueue() {
    return toProcess;
  }

  @Override
  public void consumerIsDone() {
    Log.d(LOG_TAG, "Consumer is done. Are we waiting for it? " + waitingForQueueDone);
    if (waitingForQueueDone) {
      waitingForQueueDone = false;
      delegate.onFlowCompleted(this, end);
    }
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
    
    this.consumer = new RecordConsumer(this);
    ThreadPool.run(this.consumer);
    waitingForQueueDone = true;
    source.fetchSince(timestamp, this);
  }

  


  public void beginAndFlow() {
    source.begin(this);
  }

  @Override
  public void store(Record record) {
    sink.store(record, this);
  }

  @Override
  public void onFetchFailed(Exception ex, Record record) {
    Log.w(LOG_TAG, "onFetchFailed. Calling for immediate stop.", ex);
    this.consumer.stop(true);
  }

  @Override
  public void onFetchedRecord(Record record) {
    this.enqueue(record);
    this.consumer.doNotify();
  }

  @Override
  public void onFetchCompleted(long end) {
    Log.i(LOG_TAG, "onFetchCompleted. Stopping consumer once stores are done.");
    this.end = end;
    this.consumer.stop(false);
  }

  @Override
  public void onStoreFailed(Exception ex) {
    this.consumer.stored();
    delegate.onFlowStoreFailed(this, ex);
    
  }

  @Override
  public void onStoreSucceeded(Record record) {
    this.consumer.stored();
  }

  @Override
  public void onFetchSucceeded(Record[] records, long end) {
    for (Record record : records) {
      this.toProcess.add(record);
    }
    this.consumer.doNotify();
  }

  @Override
  public void onBeginFailed(Exception ex) {
    delegate.onFlowBeginFailed(this, ex);
  }

  @Override
  public void onBeginSucceeded(RepositorySession session) {
    if (session == source) {
      sink.begin(this);
    }
    if (session == sink) {
      this.flow();
      return;
    }

    
  }

  @Override
  public RepositorySessionStoreDelegate deferredStoreDelegate() {
    final RepositorySessionStoreDelegate self = this;
    return new RepositorySessionStoreDelegate() {
      @Override
      public void onStoreSucceeded(final Record record) {
        ThreadPool.run(new Runnable() {
          @Override
          public void run() {
            self.onStoreSucceeded(record);
          }
        });
      }

      @Override
      public void onStoreFailed(final Exception ex) {
        ThreadPool.run(new Runnable() {
          @Override
          public void run() {
            self.onStoreFailed(ex);
          }
        });
      }

      @Override
      public RepositorySessionStoreDelegate deferredStoreDelegate() {
        return this;
      }
    };
  }

  @Override
  public RepositorySessionBeginDelegate deferredBeginDelegate() {
    final RepositorySessionBeginDelegate self = this;
    return new RepositorySessionBeginDelegate() {

      @Override
      public void onBeginSucceeded(final RepositorySession session) {
        ThreadPool.run(new Runnable() {
          @Override
          public void run() {
            self.onBeginSucceeded(session);
          }
        });
      }

      @Override
      public void onBeginFailed(final Exception ex) {
        ThreadPool.run(new Runnable() {
          @Override
          public void run() {
            self.onBeginFailed(ex);
          }
        });
      }

      @Override
      public RepositorySessionBeginDelegate deferredBeginDelegate() {
        return this;
      }
    };
  }
}
