



package org.mozilla.gecko.sync.stage;

import org.mozilla.gecko.sync.GlobalSession;

public class CheckPreconditionsStage extends AbstractNonRepositorySyncStage {
  public CheckPreconditionsStage(GlobalSession session) {
    super(session);
  }

  @Override
  public void execute() throws NoSuchStageException {
    session.advance();
  }
}
