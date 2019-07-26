



package org.mozilla.gecko.sync.net;

public interface SyncStorageRequestDelegate {
  public AuthHeaderProvider getAuthHeaderProvider();

  String ifUnmodifiedSince();

  
  
  

  







  void handleRequestSuccess(SyncStorageResponse response);

  








  void handleRequestFailure(SyncStorageResponse response);

  void handleRequestError(Exception ex);
}
