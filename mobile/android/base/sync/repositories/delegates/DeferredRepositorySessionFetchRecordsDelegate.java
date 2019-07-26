



package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.domain.Record;

public class DeferredRepositorySessionFetchRecordsDelegate implements RepositorySessionFetchRecordsDelegate {
  private RepositorySessionFetchRecordsDelegate inner;
  private ExecutorService executor;
  public DeferredRepositorySessionFetchRecordsDelegate(final RepositorySessionFetchRecordsDelegate inner, final ExecutorService executor) {
    this.inner = inner;
    this.executor = executor;
  }

  @Override
  public void onFetchedRecord(final Record record) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
         inner.onFetchedRecord(record);
      }
    });
  }

  @Override
  public void onFetchFailed(final Exception ex, final Record record) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onFetchFailed(ex, record);
      }
    });
  }

  @Override
  public void onFetchCompleted(final long fetchEnd) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onFetchCompleted(fetchEnd);
      }
    });
  }

  @Override
  public RepositorySessionFetchRecordsDelegate deferredFetchDelegate(ExecutorService newExecutor) {
    if (newExecutor == executor) {
      return this;
    }
    throw new IllegalArgumentException("Can't re-defer this delegate.");
  }
}
