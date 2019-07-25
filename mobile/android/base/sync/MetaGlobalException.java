



package org.mozilla.gecko.sync;

public class MetaGlobalException extends SyncException {
  private static final long serialVersionUID = -6182315615113508925L;

  public static class MetaGlobalMalformedSyncIDException extends MetaGlobalException {
    private static final long serialVersionUID = 1L;
  }

  public static class MetaGlobalMalformedVersionException extends MetaGlobalException {
    private static final long serialVersionUID = 1L;
  }

  public static class MetaGlobalOutdatedVersionException extends MetaGlobalException {
    private static final long serialVersionUID = 1L;
  }

  public static class MetaGlobalStaleClientVersionException extends MetaGlobalException {
    private static final long serialVersionUID = 1L;
    public final int serverVersion;
    public MetaGlobalStaleClientVersionException(final int version) {
      this.serverVersion = version;
    }
  }

  public static class MetaGlobalStaleClientSyncIDException extends MetaGlobalException {
    private static final long serialVersionUID = 1L;
    public final String serverSyncID;
    public MetaGlobalStaleClientSyncIDException(final String syncID) {
      this.serverSyncID = syncID;
    }
  }
}
