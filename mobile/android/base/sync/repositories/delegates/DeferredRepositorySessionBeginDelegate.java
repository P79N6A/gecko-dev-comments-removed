




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.RepositorySession;

public class DeferredRepositorySessionBeginDelegate implements RepositorySessionBeginDelegate {
  private RepositorySessionBeginDelegate inner;
  private ExecutorService executor;
  public DeferredRepositorySessionBeginDelegate(final RepositorySessionBeginDelegate inner, final ExecutorService executor) {
    this.inner = inner;
    this.executor = executor;
  }

  @Override
  public void onBeginSucceeded(final RepositorySession session) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onBeginSucceeded(session);
      }
    });
  }

  @Override
  public void onBeginFailed(final Exception ex) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        inner.onBeginFailed(ex);
      }
    });
  }
  
  @Override
  public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService newExecutor) {
    if (newExecutor == executor) {
      return this;
    }
    throw new IllegalArgumentException("Can't re-defer this delegate.");
  }
}