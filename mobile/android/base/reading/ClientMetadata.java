



package org.mozilla.gecko.reading;

public class ClientMetadata {
  public final long id;                 
  public final long lastModified;       
  public final boolean isDeleted;
  public final boolean isArchived;

  public ClientMetadata(final long id, final long lastModified, final boolean isDeleted, final boolean isArchived) {
    this.id = id;
    this.lastModified = lastModified;
    this.isDeleted = isDeleted;
    this.isArchived = isArchived;
  }
}
