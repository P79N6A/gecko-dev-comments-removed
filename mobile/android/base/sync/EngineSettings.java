



package org.mozilla.gecko.sync;

public class EngineSettings {
  public final String syncID;
  public final int version;

  public EngineSettings(final String syncID, final int version) {
    this.syncID = syncID;
    this.version = version;
  }
}
