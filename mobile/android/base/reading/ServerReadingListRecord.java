



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.ExtendedJSONObject;

public class ServerReadingListRecord extends ReadingListRecord {
  final ExtendedJSONObject fields;

  public ServerReadingListRecord(ExtendedJSONObject obj) {
    super(new ServerMetadata(obj));
    this.fields = obj.deepCopy();
  }

  @Override
  public String getURL() {
    return this.fields.getString("url");    
  }

  @Override
  public String getTitle() {
    return this.fields.getString("title");  
  }

  @Override
  public String getAddedBy() {
    return this.fields.getString("added_by");
  }
}