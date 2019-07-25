




































package org.mozilla.gecko.sync.repositories.domain;

import java.util.HashMap;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

public class HistoryRecord extends Record {

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

  public String     title;
  public String     histURI;
  public JSONArray  visits;
  public long       fennecDateVisited;
  public long       fennecVisitCount;

  @Override
  public void initFromPayload(CryptoRecord payload) {
    this.histURI = (String) payload.payload.get("histUri");
    this.title       = (String) payload.payload.get("title");
    
  }
  @Override
  public CryptoRecord getPayload() {
    
    return null;
  }

  @Override
  public boolean equals(Object o) {
    if (!o.getClass().equals(HistoryRecord.class)) return false;
    HistoryRecord other = (HistoryRecord) o;
    return
        super.equals(other) &&
        RepoUtils.stringsEqual(this.title, other.title) &&
        RepoUtils.stringsEqual(this.histURI, other.histURI) &&
        this.checkVisitsEquals(other);
  }
  
  private boolean checkVisitsEquals(HistoryRecord other) {
    
    
    if (this.visits == other.visits) return true;
    else if ((this.visits == null || this.visits.size() == 0) && (other.visits != null && other.visits.size() !=0)) return false;
    else if ((this.visits != null && this.visits.size() != 0) && (other.visits == null || other.visits.size() == 0)) return false;
    
    
    if (this.visits.size() != other.visits.size()) return false;
    
    HashMap<Long, Long> otherVisits = new HashMap<Long, Long>();
    for (int i = 0; i < other.visits.size(); i++) {
      JSONObject visit = (JSONObject) other.visits.get(i);
      otherVisits.put((Long)visit.get("date"), (Long)visit.get("type"));
    }
    
    for (int i = 0; i < this.visits.size(); i++) {
      JSONObject visit = (JSONObject) this.visits.get(i);
      if (!otherVisits.containsKey(visit.get("date"))) return false;
      if (otherVisits.get(visit.get("date")) != (Long) visit.get("type")) return false;
    }
    
    return true;
  }
  






















}
