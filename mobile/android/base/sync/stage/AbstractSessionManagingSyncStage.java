



package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;











public abstract class AbstractSessionManagingSyncStage implements GlobalSyncStage {
  protected GlobalSession session;

  protected abstract void execute() throws NoSuchStageException;
  protected abstract void resetLocal();
  protected abstract void wipeLocal() throws Exception;

  @Override
  public void resetLocal(GlobalSession session) {
    this.session = session;
    resetLocal();
  }

  @Override
  public void wipeLocal(GlobalSession session) throws Exception {
    this.session = session;
    wipeLocal();
  }

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    this.session = session;
    execute();
  }
}
