




































package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.repositories.ConstrainedServer11Repository;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserHistoryRepository;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecordFactory;

public class AndroidBrowserHistoryServerSyncStage extends ServerSyncStage {

  
  
  private static final String HISTORY_SORT          = "index";
  private static final long   HISTORY_REQUEST_LIMIT = 250;

  @Override
  public void execute(org.mozilla.gecko.sync.GlobalSession session) throws NoSuchStageException {
    super.execute(session);
  }

  @Override
  protected String getCollection() {
    return "history";
  }
  @Override
  protected String getEngineName() {
    return "history";
  }

  @Override
  protected Repository getLocalRepository() {
    return new AndroidBrowserHistoryRepository();
  }

  @Override
  protected Repository getRemoteRepository() throws URISyntaxException {
    return new ConstrainedServer11Repository(session.config.getClusterURLString(),
                                             session.config.username,
                                             getCollection(),
                                             session,
                                             HISTORY_REQUEST_LIMIT,
                                             HISTORY_SORT);
  }

  @Override
  protected RecordFactory getRecordFactory() {
    return new HistoryRecordFactory();
  }
}