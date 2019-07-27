



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.ExtendedJSONObject;

public class ClientReadingListRecord extends ReadingListRecord {
  final ExtendedJSONObject fields;
  public ClientMetadata clientMetadata;

  private String getDefaultAddedBy() {
    return "Test Device";                
  }

  


  public ClientReadingListRecord(final ServerMetadata serverMetadata, final ClientMetadata clientMetadata, final ExtendedJSONObject fields) {
    super(serverMetadata);
    this.clientMetadata = clientMetadata == null ? new ClientMetadata(-1L, -1L, false, false) : clientMetadata;
    this.fields = fields;
  }

  public ClientReadingListRecord(String url, String title, String addedBy) {
    this(url, title, addedBy, System.currentTimeMillis(), false, false);
  }

  public ClientReadingListRecord(String url, String title, String addedBy, long lastModified, boolean isDeleted, boolean isArchived) {
    super(null);

    
    if (url == null) {
      throw new IllegalArgumentException("url must be provided.");
    }

    final ExtendedJSONObject f = new ExtendedJSONObject();
    f.put("url", url);
    f.put("title", title == null ? "" : title);
    f.put("added_by", addedBy == null ? getDefaultAddedBy() : addedBy);

    this.fields = f;
    this.clientMetadata = new ClientMetadata(-1L, lastModified, isDeleted, isArchived);
  }

  public ExtendedJSONObject toJSON() {
    final ExtendedJSONObject object = this.fields.deepCopy();
    final String guid = getGUID();

    if (guid != null) {
      object.put("id", guid);
    }
    return object;
  }

  @Override
  public String getAddedBy() {
    return this.fields.getString("added_by");
  }

  @Override
  public String getURL() {
    return this.fields.getString("url");    
  }

  @Override
  public String getTitle() {
    return this.fields.getString("title");  
  }

  



  public ClientReadingListRecord givenServerRecord(ServerReadingListRecord down) {
    return new ClientReadingListRecord(down.serverMetadata, this.clientMetadata, down.fields);
  }
}