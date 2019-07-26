



package org.mozilla.gecko.sync.repositories.domain;

import java.util.HashMap;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;







public class HistoryRecord extends Record {
  private static final String LOG_TAG = "HistoryRecord";

  public static final String COLLECTION_NAME = "history";
  public static final long HISTORY_TTL = 60 * 24 * 60 * 60; 

  public HistoryRecord(String guid, String collection, long lastModified, boolean deleted) {
    super(guid, collection, lastModified, deleted);
    this.ttl = HISTORY_TTL;
  }
  public HistoryRecord(String guid, String collection, long lastModified) {
    this(guid, collection, lastModified, false);
  }
  public HistoryRecord(String guid, String collection) {
    this(guid, collection, 0, false);
  }
  public HistoryRecord(String guid) {
    this(guid, COLLECTION_NAME, 0, false);
  }
  public HistoryRecord() {
    this(Utils.generateGuid(), COLLECTION_NAME, 0, false);
  }

  public String    title;
  public String    histURI;
  public JSONArray visits;
  public long      fennecDateVisited;
  public long      fennecVisitCount;

  @SuppressWarnings("unchecked")
  private JSONArray copyVisits() {
    if (this.visits == null) {
      return null;
    }
    JSONArray out = new JSONArray();
    out.addAll(this.visits);
    return out;
  }

  @Override
  public Record copyWithIDs(String guid, long androidID) {
    HistoryRecord out = new HistoryRecord(guid, this.collection, this.lastModified, this.deleted);
    out.androidID = androidID;
    out.sortIndex = this.sortIndex;
    out.ttl       = this.ttl;

    
    out.title             = this.title;
    out.histURI           = this.histURI;
    out.fennecDateVisited = this.fennecDateVisited;
    out.fennecVisitCount  = this.fennecVisitCount;
    out.visits            = this.copyVisits();

    return out;
  }

  @Override
  protected void populatePayload(ExtendedJSONObject payload) {
    putPayload(payload, "id",      this.guid);
    putPayload(payload, "title",   this.title);
    putPayload(payload, "histUri", this.histURI);             
    payload.put("visits",  this.visits);
  }

  @Override
  protected void initFromPayload(ExtendedJSONObject payload) {
    this.histURI = (String) payload.get("histUri");
    this.title   = (String) payload.get("title");
    try {
      this.visits = payload.getArray("visits");
    } catch (NonArrayJSONException e) {
      Logger.error(LOG_TAG, "Got non-array visits in history record " + this.guid, e);
      this.visits = new JSONArray();
    }
  }

  




  @Override
  public boolean congruentWith(Object o) {
    if (o == null || !(o instanceof HistoryRecord)) {
      return false;
    }
    HistoryRecord other = (HistoryRecord) o;
    if (!super.congruentWith(other)) {
      return false;
    }
    return RepoUtils.stringsEqual(this.histURI, other.histURI);
  }

  @Override
  public boolean equalPayloads(Object o) {
    if (o == null || !(o instanceof HistoryRecord)) {
      Logger.debug(LOG_TAG, "Not a HistoryRecord: " + o.getClass());
      return false;
    }
    HistoryRecord other = (HistoryRecord) o;
    if (!super.equalPayloads(other)) {
      Logger.debug(LOG_TAG, "super.equalPayloads returned false.");
      return false;
    }
    return RepoUtils.stringsEqual(this.title, other.title) &&
           RepoUtils.stringsEqual(this.histURI, other.histURI) &&
           checkVisitsEquals(other);
  }

  @Override
  public boolean equalAndroidIDs(Record other) {
    return super.equalAndroidIDs(other) &&
           this.equalFennecVisits(other);
  }

  private boolean equalFennecVisits(Record other) {
    if (!(other instanceof HistoryRecord)) {
      return false;
    }
    HistoryRecord h = (HistoryRecord) other;
    return this.fennecDateVisited == h.fennecDateVisited &&
           this.fennecVisitCount  == h.fennecVisitCount;
  }

  private boolean checkVisitsEquals(HistoryRecord other) {
    Logger.debug(LOG_TAG, "Checking visits.");
    if (Logger.LOG_PERSONAL_INFORMATION) {
      
      Logger.pii(LOG_TAG, ">> Mine:   " + ((this.visits == null) ? "null" : this.visits.toJSONString()));
      Logger.pii(LOG_TAG, ">> Theirs: " + ((other.visits == null) ? "null" : other.visits.toJSONString()));
    }

    
    if (this.visits == other.visits) {
      return true;
    }

    
    int aSize = this.visits == null ? 0 : this.visits.size();
    int bSize = other.visits == null ? 0 : other.visits.size();
    
    if (aSize != bSize) {
      return false;
    }

    

    
    HashMap<Long, Long> otherVisits = new HashMap<Long, Long>();
    for (int i = 0; i < bSize; i++) {
      JSONObject visit = (JSONObject) other.visits.get(i);
      otherVisits.put((Long) visit.get("date"), (Long) visit.get("type"));
    }
    
    for (int i = 0; i < aSize; i++) {
      JSONObject visit = (JSONObject) this.visits.get(i);
      if (!otherVisits.containsKey(visit.get("date"))) {
        return false;
      }
      Long otherDate = (Long) visit.get("date");
      Long otherType = otherVisits.get(otherDate);
      if (otherType == null) {
        return false;
      }
      if (!otherType.equals((Long) visit.get("type"))) {
        return false;
      }
    }
    
    return true;
  }
  






















}
