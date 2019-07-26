



package org.mozilla.gecko.sync.stage;


public class CheckPreconditionsStage extends AbstractNonRepositorySyncStage {
  @Override
  public void execute() throws NoSuchStageException {
    session.advance();
  }
}
