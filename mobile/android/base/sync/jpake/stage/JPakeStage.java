



package org.mozilla.gecko.sync.jpake.stage;

import org.mozilla.gecko.sync.jpake.JPakeClient;

public abstract class JPakeStage {
  protected final String LOG_TAG = "SyncJPakeStage";
  public abstract void execute(JPakeClient jClient);
}
