



package org.mozilla.gecko.sync;

public class EngineSettings {
  public final String syncID;
  public final int version;

  public EngineSettings(final String syncID, final int version) {
    this.syncID = syncID;
    this.version = version;
  }

  public EngineSettings(ExtendedJSONObject object) {
    try {
      this.syncID = object.getString("syncID");
      this.version = object.getIntegerSafely("version");
    } catch (Exception e ) {
      throw new IllegalArgumentException(e);
    }
  }

  public ExtendedJSONObject toJSONObject() {
    ExtendedJSONObject json = new ExtendedJSONObject();
    json.put("syncID", syncID);
    json.put("version", version);
    return json;
  }
}
