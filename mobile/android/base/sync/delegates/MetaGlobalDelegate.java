




































package org.mozilla.gecko.sync.delegates;

import org.mozilla.gecko.sync.MetaGlobal;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

public interface MetaGlobalDelegate {
  public void handleSuccess(MetaGlobal global);
  public void handleMissing(MetaGlobal global);
  public void handleFailure(SyncStorageResponse response);
  public void handleError(Exception e);
  public MetaGlobalDelegate deferred();
}
