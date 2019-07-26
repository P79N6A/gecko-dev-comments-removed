



package org.mozilla.gecko.sync.delegates;

import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.net.SyncStorageResponse;





public interface JSONRecordFetchDelegate {
  public void handleSuccess(ExtendedJSONObject body);
  public void handleFailure(SyncStorageResponse response);
  public void handleError(Exception e);
}
