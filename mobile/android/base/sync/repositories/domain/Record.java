





































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

  public Record(String guid, String collection, long lastModified, boolean deleted) {
    this.guid         = guid;
    this.collection   = collection;
    this.lastModified = lastModified;
    this.deleted      = deleted;
    this.sortIndex    = 0;
  }

  @Override
  public boolean equals(Object o) {
    if (o == null) {
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

    if (this.deleted != other.deleted) {
      return false;
    }
    return true;
  }

  public abstract void initFromPayload(CryptoRecord payload);
  public abstract CryptoRecord getPayload();

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
}
