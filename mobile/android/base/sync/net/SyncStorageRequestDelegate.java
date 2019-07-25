




































package org.mozilla.gecko.sync.net;

public interface SyncStorageRequestDelegate {
  String credentials();
  String ifUnmodifiedSince();

  
  
  
  void handleRequestSuccess(SyncStorageResponse response);
  void handleRequestFailure(SyncStorageResponse response);
  void handleRequestError(Exception ex);
}
