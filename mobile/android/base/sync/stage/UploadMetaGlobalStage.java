



package org.mozilla.gecko.sync.stage;


public class UploadMetaGlobalStage extends AbstractNonRepositorySyncStage {
  public static final String LOG_TAG = "UploadMGStage";

  @Override
  public void execute() throws NoSuchStageException {
    if (session.hasUpdatedMetaGlobal()) {
      session.uploadUpdatedMetaGlobal();
    }
    session.advance();
  }
}
