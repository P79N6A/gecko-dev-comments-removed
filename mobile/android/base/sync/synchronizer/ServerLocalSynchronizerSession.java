



package org.mozilla.gecko.sync.synchronizer;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.FetchFailedException;
import org.mozilla.gecko.sync.repositories.StoreFailedException;













public class ServerLocalSynchronizerSession extends SynchronizerSession {
  protected static final String LOG_TAG = "ServLocSynchronizerSess";

  public ServerLocalSynchronizerSession(Synchronizer synchronizer, SynchronizerSessionDelegate delegate) {
    super(synchronizer, delegate);
  }

  public void onFirstFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
    
    int numRemoteFetchFailed = recordsChannel.getFetchFailureCount();
    if (numRemoteFetchFailed > 0) {
      final String message = "Got " + numRemoteFetchFailed + " failures fetching remote records!";
      Logger.warn(LOG_TAG, message + " Aborting session.");
      delegate.onSynchronizeFailed(this, new FetchFailedException(), message);
      return;
    }
    Logger.trace(LOG_TAG, "No failures fetching remote records.");

    
    int numLocalStoreFailed = recordsChannel.getStoreFailureCount();
    if (numLocalStoreFailed > 0) {
      final String message = "Got " + numLocalStoreFailed + " failures storing local records!";
      Logger.warn(LOG_TAG, message + " Ignoring local store failures and continuing synchronizer session.");
    } else {
      Logger.trace(LOG_TAG, "No failures storing local records.");
    }

    super.onFirstFlowCompleted(recordsChannel, fetchEnd, storeEnd);
  }

  public void onSecondFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
    
    int numLocalFetchFailed = recordsChannel.getFetchFailureCount();
    if (numLocalFetchFailed > 0) {
      final String message = "Got " + numLocalFetchFailed + " failures fetching local records!";
      Logger.warn(LOG_TAG, message + " Aborting session.");
      delegate.onSynchronizeFailed(this, new FetchFailedException(), message);
      return;
    }
    Logger.trace(LOG_TAG, "No failures fetching local records.");

    
    int numRemoteStoreFailed = recordsChannel.getStoreFailureCount();
    if (numRemoteStoreFailed > 0) {
      final String message = "Got " + numRemoteStoreFailed + " failures storing remote records!";
      Logger.warn(LOG_TAG, message + " Aborting session.");
      delegate.onSynchronizeFailed(this, new StoreFailedException(), message);
      return;
    }
    Logger.trace(LOG_TAG, "No failures storing remote records.");

    super.onSecondFlowCompleted(recordsChannel, fetchEnd, storeEnd);
  }
}
