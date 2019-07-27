



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.ExtendedJSONObject;




public abstract class ReadingListRecord {
  public static class ServerMetadata {
    public final String guid;             
    public final long lastModified;       

    public ServerMetadata(String guid, long lastModified) {
      this.guid = guid;
      this.lastModified = lastModified;
    }

    


    public ServerMetadata(ExtendedJSONObject obj) {
      this(obj.getString("id"), obj.containsKey("last_modified") ? obj.getLong("last_modified") : -1L);
    }
  }

  public final ServerMetadata serverMetadata;

  public String getGUID() {
    if (serverMetadata == null) {
      return null;
    }

    return serverMetadata.guid;
  }

  public long getServerLastModified() {
    if (serverMetadata == null) {
      return -1L;
    }

    return serverMetadata.lastModified;
  }

  protected ReadingListRecord(final ServerMetadata serverMetadata) {
    this.serverMetadata = serverMetadata;
  }

  public abstract String getURL();
  public abstract String getTitle();
  public abstract String getAddedBy();
}
