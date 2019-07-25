




































package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.InfoCollections;
import org.mozilla.gecko.sync.delegates.InfoCollectionsDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

import android.util.Log;

public class FetchInfoCollectionsStage implements GlobalSyncStage {
  private static final String LOG_TAG = "FetchInfoCollectionsStage";

  public class StageInfoCollectionsDelegate implements InfoCollectionsDelegate {

    private GlobalSession session;
    public StageInfoCollectionsDelegate(GlobalSession session) {
      this.session = session;
    }

    @Override
    public void handleSuccess(InfoCollections global) {
      Log.i(LOG_TAG, "Got timestamps: ");

      
      session.advance();
    }

    @Override
    public void handleFailure(SyncStorageResponse response) {
      session.handleHTTPError(response, "Failure fetching info/collections.");
    }

    @Override
    public void handleError(Exception e) {
      session.abort(e, "Failure fetching info/collections.");
    }

  }

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    try {
      session.fetchInfoCollections(new StageInfoCollectionsDelegate(session));
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

}
