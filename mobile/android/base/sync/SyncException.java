




































package org.mozilla.gecko.sync;

import android.content.SyncResult;

public abstract class SyncException extends Exception {
  public Exception cause = null;
  public SyncException(Exception ex) {
    cause = ex;
  }
  public SyncException() {
  }

  private static final long serialVersionUID = -6928990004393234738L;
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    
    
    syncResult.databaseError = true;
  }
}
