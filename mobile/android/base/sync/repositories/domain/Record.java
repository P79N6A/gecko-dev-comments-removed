



package org.mozilla.gecko.sync.repositories.domain;

import java.io.UnsupportedEncodingException;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.ExtendedJSONObject;























































public abstract class Record {

  public String guid;
  public String collection;
  public long lastModified;
  public boolean deleted;
  public long androidID;
  




  public long sortIndex;
  








  public long ttl;

  public Record(String guid, String collection, long lastModified, boolean deleted) {
    this.guid         = guid;
    this.collection   = collection;
    this.lastModified = lastModified;
    this.deleted      = deleted;
    this.sortIndex    = 0;
    this.ttl          = 365 * 24 * 60 * 60; 
    this.androidID    = -1;
  }

  



  public boolean equalIdentifiers(Object o) {
    if (o == null || !(o instanceof Record)) {
      return false;
    }

    Record other = (Record) o;
    if (this.guid == null) {
      if (other.guid != null) {
        return false;
      }
    } else {
      if (!this.guid.equals(other.guid)) {
        return false;
      }
    }
    if (this.collection == null) {
      if (other.collection != null) {
        return false;
      }
    } else {
      if (!this.collection.equals(other.collection)) {
        return false;
      }
    }
    return true;
  }

  






  public boolean equalPayloads(Object o) {
    if (!this.equalIdentifiers(o)) {
      return false;
    }
    Record other = (Record) o;
    return this.deleted == other.deleted;
  }

  









  public boolean congruentWith(Object o) {
    if (!this.equalIdentifiers(o)) {
      return false;
    }
    Record other = (Record) o;
    return congruentAndroidIDs(other) &&
           (this.deleted == other.deleted);
  }

  public boolean congruentAndroidIDs(Record other) {
    
    
    if (this.androidID  != -1 &&
        other.androidID != -1 &&
        this.androidID  != other.androidID) {
      return false;
    }
    return true;
  }

  



  @Override
  public boolean equals(Object o) {
    if (o == null || !(o instanceof Record)) {
      return false;
    }

    Record other = (Record) o;
    return equalTimestamps(other) &&
           equalSortIndices(other) &&
           equalAndroidIDs(other) &&
           equalPayloads(o);
  }

  public boolean equalAndroidIDs(Record other) {
    return this.androidID == other.androidID;
  }

  public boolean equalSortIndices(Record other) {
    return this.sortIndex == other.sortIndex;
  }

  public boolean equalTimestamps(Object o) {
    if (o == null || !(o instanceof Record)) {
      return false;
    }
    return ((Record) o).lastModified == this.lastModified;
  }

  protected abstract void populatePayload(ExtendedJSONObject payload);
  protected abstract void initFromPayload(ExtendedJSONObject payload);

  public void initFromEnvelope(CryptoRecord envelope) {
    ExtendedJSONObject p = envelope.payload;
    this.guid = envelope.guid;
    checkGUIDs(p);

    this.collection    = envelope.collection;
    this.lastModified  = envelope.lastModified;

    final Object del = p.get("deleted");
    if (del instanceof Boolean) {
      this.deleted = (Boolean) del;
    } else {
      this.initFromPayload(p);
    }

  }

  public CryptoRecord getEnvelope() {
    CryptoRecord rec = new CryptoRecord(this);
    ExtendedJSONObject payload = new ExtendedJSONObject();
    payload.put("id", this.guid);

    if (this.deleted) {
      payload.put("deleted", true);
    } else {
      populatePayload(payload);
    }
    rec.payload = payload;
    return rec;
  }

  @SuppressWarnings("static-method")
  public String toJSONString() {
    throw new RuntimeException("Cannot JSONify non-CryptoRecord Records.");
  }

  public byte[] toJSONBytes() {
    try {
      return this.toJSONString().getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      
      return null;
    }
  }

  






  @SuppressWarnings("static-method")
  protected void putPayload(CryptoRecord rec, String key, String value) {
    if (value == null) {
      return;
    }
    rec.payload.put(key, value);
  }

  protected void putPayload(ExtendedJSONObject payload, String key, String value) {
    this.putPayload(payload, key, value, false);
  }

  @SuppressWarnings("static-method")
  protected void putPayload(ExtendedJSONObject payload, String key, String value, boolean excludeEmpty) {
    if (value == null) {
      return;
    }
    if (excludeEmpty && value.equals("")) {
      return;
    }
    payload.put(key, value);
  }

  protected void checkGUIDs(ExtendedJSONObject payload) {
    String payloadGUID = (String) payload.get("id");
    if (this.guid == null ||
        payloadGUID == null) {
      String detailMessage = "Inconsistency: either envelope or payload GUID missing.";
      throw new IllegalStateException(detailMessage);
    }
    if (!this.guid.equals(payloadGUID)) {
      String detailMessage = "Inconsistency: record has envelope ID " + this.guid + ", payload ID " + payloadGUID;
      throw new IllegalStateException(detailMessage);
    }
  }

  







  public abstract Record copyWithIDs(String guid, long androidID);
}
