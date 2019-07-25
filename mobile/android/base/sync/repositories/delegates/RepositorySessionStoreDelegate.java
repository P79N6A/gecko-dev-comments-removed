



package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;








public interface RepositorySessionStoreDelegate {
  public void onRecordStoreFailed(Exception ex, String recordGuid);

  
  public void onRecordStoreSucceeded(String guid);
  public void onStoreCompleted(long storeEnd);
  public RepositorySessionStoreDelegate deferredStoreDelegate(ExecutorService executor);
}
