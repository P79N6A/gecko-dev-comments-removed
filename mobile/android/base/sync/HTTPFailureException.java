




































package org.mozilla.gecko.sync;

import org.mozilla.gecko.sync.net.SyncStorageResponse;

import android.content.SyncResult;

public class HTTPFailureException extends SyncException {
  private static final long serialVersionUID = -5415864029780770619L;
  public SyncStorageResponse response;

  public HTTPFailureException(SyncStorageResponse response) {
    this.response = response;
  }

  @Override
  public String toString() {
    String errorMessage = "[unknown error message]";
    try {
      errorMessage = this.response.getErrorMessage();
    } catch (Exception e) {
      
    }
    return "<HTTPFailureException " + this.response.getStatusCode() +
           " :: (" + errorMessage + ")>";
  }

  @Override
  public void updateStats(GlobalSession globalSession, SyncResult syncResult) {
    switch (response.getStatusCode()) {
    case 401:
      
      syncResult.stats.numAuthExceptions++;
      return;
    case 500:
    case 501:
    case 503:
      
      syncResult.stats.numIoExceptions++;
      return;
    }
  }
}
