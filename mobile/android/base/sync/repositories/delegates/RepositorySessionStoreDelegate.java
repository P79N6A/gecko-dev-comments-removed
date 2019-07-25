




































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.domain.Record;








public interface RepositorySessionStoreDelegate {
  public void onRecordStoreFailed(Exception ex);
  public void onRecordStoreSucceeded(Record record);
  public void onStoreCompleted();
  public RepositorySessionStoreDelegate deferredStoreDelegate(ExecutorService executor);
}
