





































package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;

public abstract class Record {
  public String guid;
  public String collection;
  public long lastModified;
  public boolean deleted;
  public long androidID;

  public Record(String guid, String collection, long lastModified, boolean deleted) {
    this.guid         = guid;
    this.collection   = collection;
    this.lastModified = lastModified;
    this.deleted = deleted;
  }

  @Override
  public boolean equals(Object o) {
    Record other = (Record) o;
    
    if(!((this.guid == other.guid) || (this.guid.equals(other.guid)))) return false;
    if(!((this.collection == other.collection) || (this.collection.equals(other.collection)))) return false;
    if(this.deleted != other.deleted) return false;
    return true;
  }

  public abstract void initFromPayload(CryptoRecord payload);
  public abstract CryptoRecord getPayload();
}
