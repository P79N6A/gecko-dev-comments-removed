




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.domain.Record;

public class DeferredRepositorySessionStoreDelegate implements
    RepositorySessionStoreDelegate {
  protected final RepositorySessionStoreDelegate inner;
  protected final ExecutorService                executor;

  public DeferredRepositorySessionStoreDelegate(
      RepositorySessionStoreDelegate inner, ExecutorService executor) {
    this.inner = inner;
    this.executor = executor;
  }

  @Override
  public void onRecordStoreSucceeded(final Record record) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onRecordStoreSucceeded(record);
      }
    });
  }

  @Override
  public void onRecordStoreFailed(final Exception ex) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onRecordStoreFailed(ex);
      }
    });
  }

  @Override
  public RepositorySessionStoreDelegate deferredStoreDelegate(ExecutorService newExecutor) {
    if (newExecutor == executor) {
      return this;
    }
    throw new IllegalArgumentException("Can't re-defer this delegate.");
  }

  @Override
  public void onStoreCompleted(final long storeEnd) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onStoreCompleted(storeEnd);
      }
    });
  }
}
