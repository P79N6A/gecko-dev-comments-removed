



package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.repositories.ConstrainedServer11Repository;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.FormHistoryRepositorySession;
import org.mozilla.gecko.sync.repositories.domain.FormHistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

public class FormHistoryServerSyncStage extends ServerSyncStage {

  
  
  private static final String FORM_HISTORY_SORT          = "index";
  private static final long   FORM_HISTORY_REQUEST_LIMIT = 5000;         

  public FormHistoryServerSyncStage(GlobalSession session) {
    super(session);
  }

  @Override
  public void execute() throws NoSuchStageException {
    super.execute();
  }

  @Override
  protected String getCollection() {
    return "forms";
  }
  @Override
  protected String getEngineName() {
    return "forms";
  }

  @Override
  protected Repository getRemoteRepository() throws URISyntaxException {
    return new ConstrainedServer11Repository(session.config.getClusterURLString(),
                                             session.config.username,
                                             getCollection(),
                                             session,
                                             FORM_HISTORY_REQUEST_LIMIT,
                                             FORM_HISTORY_SORT);
  }

  @Override
  protected Repository getLocalRepository() {
    return new FormHistoryRepositorySession.FormHistoryRepository();
  }

  public class FormHistoryRecordFactory extends RecordFactory {

    @Override
    public Record createRecord(Record record) {
      FormHistoryRecord r = new FormHistoryRecord();
      r.initFromEnvelope((CryptoRecord) record);
      return r;
    }
  }

  @Override
  protected RecordFactory getRecordFactory() {
    return new FormHistoryRecordFactory();
  }
}
