



package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.FennecTabsRepository;
import org.mozilla.gecko.sync.repositories.domain.TabsRecordFactory;
import org.mozilla.gecko.sync.repositories.domain.VersionConstants;

public class FennecTabsServerSyncStage extends ServerSyncStage {
  private static final String COLLECTION = "tabs";

  @Override
  protected String getCollection() {
    return COLLECTION;
  }

  @Override
  protected String getEngineName() {
    return COLLECTION;
  }

  @Override
  public Integer getStorageVersion() {
    return VersionConstants.TABS_ENGINE_VERSION;
  }

  @Override
  protected Repository getLocalRepository() {
    final ClientsDataDelegate clientsDelegate = session.getClientsDelegate();
    return new FennecTabsRepository(clientsDelegate.getClientName(), clientsDelegate.getAccountGUID());
  }

  @Override
  protected RecordFactory getRecordFactory() {
    return new TabsRecordFactory();
  }
}
