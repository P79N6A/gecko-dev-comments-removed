



package org.mozilla.gecko.background.healthreport.upload;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;
import org.mozilla.gecko.sync.ExtendedJSONObject;

import android.content.SharedPreferences;

public class ObsoleteDocumentTracker {
  public static final String LOG_TAG = ObsoleteDocumentTracker.class.getSimpleName();

  protected final SharedPreferences sharedPrefs;

  public ObsoleteDocumentTracker(SharedPreferences sharedPrefs) {
    this.sharedPrefs = sharedPrefs;
  }

  protected ExtendedJSONObject getObsoleteIds() {
    String s = sharedPrefs.getString(HealthReportConstants.PREF_OBSOLETE_DOCUMENT_IDS_TO_DELETION_ATTEMPTS_REMAINING, null);
    if (s == null) {
      
      String lastId = sharedPrefs.getString(HealthReportConstants.PREF_LAST_UPLOAD_DOCUMENT_ID, null);
      if (lastId == null) {
        return new ExtendedJSONObject();
      }
      ExtendedJSONObject ids = new ExtendedJSONObject();
      ids.put(lastId, HealthReportConstants.DELETION_ATTEMPTS_PER_OBSOLETE_DOCUMENT_ID);
      setObsoleteIds(ids);
      return ids;
    }
    try {
      return ExtendedJSONObject.parseJSONObject(s);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception getting obsolete ids.", e);
      return new ExtendedJSONObject();
    }
  }

  




  protected void setObsoleteIds(ExtendedJSONObject ids) {
    sharedPrefs
      .edit()
      .putString(HealthReportConstants.PREF_OBSOLETE_DOCUMENT_IDS_TO_DELETION_ATTEMPTS_REMAINING, ids.toString())
      .commit();
  }

  






  public void removeObsoleteId(String id) {
    ExtendedJSONObject ids = getObsoleteIds();
    ids.remove(id);
    setObsoleteIds(ids);
  }

  protected void decrementObsoleteId(ExtendedJSONObject ids, String id) {
    if (!ids.containsKey(id)) {
      return;
    }
    try {
      Long attempts = ids.getLong(id);
      if (attempts == null || --attempts < 1) {
        ids.remove(id);
      } else {
        ids.put(id, attempts);
      }
    } catch (ClassCastException e) {
      ids.remove(id);
      Logger.info(LOG_TAG, "Got exception decrementing obsolete ids counter.", e);
    }
  }

  







  public void decrementObsoleteIdAttempts(String id) {
    ExtendedJSONObject ids = getObsoleteIds();
    decrementObsoleteId(ids, id);
    setObsoleteIds(ids);
  }

  public void purgeObsoleteIds(Collection<String> oldIds) {
    ExtendedJSONObject ids = getObsoleteIds();
    for (String oldId : oldIds) {
      ids.remove(oldId);
    }
    setObsoleteIds(ids);
  }

  public void decrementObsoleteIdAttempts(Collection<String> oldIds) {
    ExtendedJSONObject ids = getObsoleteIds();
    for (String oldId : oldIds) {
      decrementObsoleteId(ids, oldId);
    }
    setObsoleteIds(ids);
  }

  




  public static class PairComparator implements Comparator<Entry<String, Object>> {
    @Override
    public int compare(Entry<String, Object> lhs, Entry<String, Object> rhs) {
      Object l = lhs.getValue();
      Object r = rhs.getValue();
      if (l == null || !(l instanceof Long)) {
        if (r == null || !(r instanceof Long)) {
          return 0;
        }
        return -1;
      }
      if (r == null || !(r instanceof Long)) {
        return 1;
      }
      return ((Long) r).compareTo((Long) l);
    }
  }

  








  public Collection<String> getBatchOfObsoleteIds() {
    ExtendedJSONObject ids = getObsoleteIds();
    
    List<Entry<String, Object>> pairs = new ArrayList<Entry<String,Object>>(ids.entrySet());
    Collections.sort(pairs, new PairComparator());
    List<String> batch = new ArrayList<String>(HealthReportConstants.MAXIMUM_DELETIONS_PER_POST);
    int i = 0;
    while (batch.size() < HealthReportConstants.MAXIMUM_DELETIONS_PER_POST && i < pairs.size()) {
      batch.add(pairs.get(i++).getKey());
    }
    return batch;
  }

  






  public void addObsoleteId(String id) {
    ExtendedJSONObject ids = getObsoleteIds();
    if (ids.size() >= HealthReportConstants.MAXIMUM_STORED_OBSOLETE_DOCUMENT_IDS) {
      
      
      
      ids.remove(Collections.max(ids.entrySet(), new PairComparator()).getKey());
    }
    ids.put(id, HealthReportConstants.DELETION_ATTEMPTS_PER_OBSOLETE_DOCUMENT_ID);
    setObsoleteIds(ids);
  }

  






  public void markIdAsUploaded(String id) {
    ExtendedJSONObject ids = getObsoleteIds();
    ids.put(id, HealthReportConstants.DELETION_ATTEMPTS_PER_KNOWN_TO_BE_ON_SERVER_DOCUMENT_ID);
    setObsoleteIds(ids);
  }

  public boolean hasObsoleteIds() {
    return getObsoleteIds().size() > 0;
  }

  public int numberOfObsoleteIds() {
    return getObsoleteIds().size();
  }

  public String getNextObsoleteId() {
    ExtendedJSONObject ids = getObsoleteIds();
    if (ids.size() < 1) {
      return null;
    }
    try {
      
      
      
      return Collections.min(ids.entrySet(), new PairComparator()).getKey();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception picking obsolete id to delete.", e);
      return null;
    }
  }

  




  public void limitObsoleteIds() {
    ExtendedJSONObject ids = getObsoleteIds();

    Set<String> keys = new HashSet<String>(ids.keySet()); 
    for (String key : keys) {
      Object o = ids.get(key);
      if (!(o instanceof Long)) {
        continue;
      }
      if (((Long) o).longValue() > HealthReportConstants.DELETION_ATTEMPTS_PER_OBSOLETE_DOCUMENT_ID) {
        ids.put(key, HealthReportConstants.DELETION_ATTEMPTS_PER_OBSOLETE_DOCUMENT_ID);
      }
    }
    setObsoleteIds(ids);
  }
}
