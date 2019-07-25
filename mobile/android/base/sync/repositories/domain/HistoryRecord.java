




































package org.mozilla.gecko.sync.repositories.domain;

import java.util.HashMap;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

import android.util.Log;







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
      Log.e(LOG_TAG, "Got non-array visits in history record " + this.guid, e);
      this.visits = new JSONArray();
    }
  }

  @Override
  public CryptoRecord getPayload() {
    CryptoRecord rec = new CryptoRecord(this);
    rec.payload = new ExtendedJSONObject();
    Log.d(LOG_TAG, "Getting payload for history record " + this.guid + " (" + this.guid.length() + ").");
    rec.payload.put("id",      this.guid);
    rec.payload.put("title",   this.title);
    rec.payload.put("histUri", this.histURI);             
    rec.payload.put("visits",  this.visits);
    return rec;
  }

  public boolean equalsExceptVisits(Object o) {
    if (!(o instanceof HistoryRecord)) {
      return false;
    }
    HistoryRecord other = (HistoryRecord) o;
    return super.equals(other) &&
           RepoUtils.stringsEqual(this.title, other.title) &&
           RepoUtils.stringsEqual(this.histURI, other.histURI);
  }

  public boolean equalsIncludingVisits(Object o) {
    HistoryRecord other = (HistoryRecord) o;
    return equalsExceptVisits(other) && this.checkVisitsEquals(other);
  }

  @Override
  



  public boolean equals(Object o) {
    return equalsExceptVisits(o);
  }

  private boolean checkVisitsEquals(HistoryRecord other) {
    
    
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
