




































package org.mozilla.gecko.sync.repositories.domain;

import java.util.HashMap;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;







public class HistoryRecord extends Record {
  private static final String LOG_TAG = "HistoryRecord";

  public static final String COLLECTION_NAME = "history";

  public HistoryRecord(String guid, String collection, long lastModified,
      boolean deleted) {
    super(guid, collection, lastModified, deleted);
  }
  public HistoryRecord(String guid, String collection, long lastModified) {
    super(guid, collection, lastModified, false);
  }
  public HistoryRecord(String guid, String collection) {
    super(guid, collection, 0, false);
  }
  public HistoryRecord(String guid) {
    super(guid, COLLECTION_NAME, 0, false);
  }
  public HistoryRecord() {
    super(Utils.generateGuid(), COLLECTION_NAME, 0, false);
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

    
    out.title             = this.title;
    out.histURI           = this.histURI;
    out.fennecDateVisited = this.fennecDateVisited;
    out.fennecVisitCount  = this.fennecVisitCount;
    out.visits            = this.copyVisits();

    return out;
  }

  @Override
  public void initFromPayload(CryptoRecord payload) {
    ExtendedJSONObject p = payload.payload;

    this.guid = payload.guid;
    this.checkGUIDs(p);

    this.lastModified  = payload.lastModified;
    this.deleted       = payload.deleted;

    this.histURI = (String) p.get("histUri");
    this.title   = (String) p.get("title");
    try {
      this.visits = p.getArray("visits");
    } catch (NonArrayJSONException e) {
      Logger.error(LOG_TAG, "Got non-array visits in history record " + this.guid, e);
      this.visits = new JSONArray();
    }
  }

  @Override
  public CryptoRecord getPayload() {
    CryptoRecord rec = new CryptoRecord(this);
    rec.payload = new ExtendedJSONObject();
    Logger.debug(LOG_TAG, "Getting payload for history record " + this.guid + " (" + this.guid.length() + ").");
    rec.payload.put("id",      this.guid);
    rec.payload.put("title",   this.title);
    rec.payload.put("histUri", this.histURI);             
    rec.payload.put("visits",  this.visits);
    return rec;
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
    return RepoUtils.stringsEqual(this.title, other.title) &&
           RepoUtils.stringsEqual(this.histURI, other.histURI);
  }

  @Override
  public boolean equalPayloads(Object o) {
    if (o == null || !(o instanceof HistoryRecord)) {
      Logger.debug(LOG_TAG, "Not a HistoryRecord: " + o);
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
    if (Logger.logVerbose(LOG_TAG)) {
      
      Logger.trace(LOG_TAG, ">> Mine:   " + ((this.visits == null) ? "null" : this.visits.toJSONString()));
      Logger.trace(LOG_TAG, ">> Theirs: " + ((other.visits == null) ? "null" : other.visits.toJSONString()));
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
