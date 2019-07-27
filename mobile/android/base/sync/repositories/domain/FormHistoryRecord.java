



package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;








public class FormHistoryRecord extends Record {
  private static final String LOG_TAG = "FormHistoryRecord";

  public static final String  COLLECTION_NAME = "forms";
  private static final String PAYLOAD_NAME    = "name";
  private static final String PAYLOAD_VALUE   = "value";
  public static final long FORMS_TTL = 60 * 24 * 60 * 60; 

  


  public String fieldName;

  


  public String fieldValue;

  public FormHistoryRecord(String guid, String collection, long lastModified, boolean deleted) {
    super(guid, collection, lastModified, deleted);
    this.ttl = FORMS_TTL;
  }

  public FormHistoryRecord(String guid, String collection, long lastModified) {
    this(guid, collection, lastModified, false);
  }

  public FormHistoryRecord(String guid, String collection) {
    this(guid, collection, 0, false);
  }

  public FormHistoryRecord(String guid) {
    this(guid, COLLECTION_NAME, 0, false);
  }

  public FormHistoryRecord() {
    this(Utils.generateGuid(), COLLECTION_NAME, 0, false);
  }

  @Override
  public Record copyWithIDs(String guid, long androidID) {
    FormHistoryRecord out = new FormHistoryRecord(guid, this.collection, this.lastModified, this.deleted);
    out.androidID = androidID;
    out.sortIndex = this.sortIndex;

    
    out.fieldName = this.fieldName;
    out.fieldValue = this.fieldValue;

    return out;
  }

  @Override
  public void populatePayload(ExtendedJSONObject payload) {
    putPayload(payload, PAYLOAD_NAME,  this.fieldName);
    putPayload(payload, PAYLOAD_VALUE, this.fieldValue);
  }

  @Override
  public void initFromPayload(ExtendedJSONObject payload) {
    this.fieldName  = payload.getString(PAYLOAD_NAME);
    this.fieldValue = payload.getString(PAYLOAD_VALUE);
  }

  



  @Override
  public boolean congruentWith(Object o) {
    if (!(o instanceof FormHistoryRecord)) {
      return false;
    }
    FormHistoryRecord other = (FormHistoryRecord) o;
    if (!super.congruentWith(other)) {
      return false;
    }
    return RepoUtils.stringsEqual(this.fieldName, other.fieldName) &&
           RepoUtils.stringsEqual(this.fieldValue, other.fieldValue);
  }

  @Override
  public boolean equalPayloads(Object o) {
    if (!(o instanceof FormHistoryRecord)) {
      Logger.debug(LOG_TAG, "Not a FormHistoryRecord: " + o.getClass());
      return false;
    }
    FormHistoryRecord other = (FormHistoryRecord) o;
    if (!super.equalPayloads(other)) {
      Logger.debug(LOG_TAG, "super.equalPayloads returned false.");
      return false;
    }

    if (this.deleted) {
      
      
      
      if (other.deleted) {
        return RepoUtils.stringsEqual(this.guid, other.guid);
      }
      return false;
    }

    return RepoUtils.stringsEqual(this.fieldName,  other.fieldName) &&
           RepoUtils.stringsEqual(this.fieldValue, other.fieldValue);
  }

  public FormHistoryRecord log(String logTag) {
    try {
      Logger.debug(logTag, "Returning form history record " + guid + " (" + androidID + ")");
      Logger.debug(logTag, "> Last modified: " + lastModified);
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.pii(logTag, "> Field name:    " + fieldName);
        Logger.pii(logTag, "> Field value:   " + fieldValue);
      }
    } catch (Exception e) {
      Logger.debug(logTag, "Exception logging form history record " + this, e);
    }
    return this;
  }
}
