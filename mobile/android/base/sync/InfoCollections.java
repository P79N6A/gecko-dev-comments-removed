



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map.Entry;
import java.util.Set;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.delegates.InfoCollectionsDelegate;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

import android.util.Log;

public class InfoCollections implements SyncStorageRequestDelegate {
  private static final String LOG_TAG = "InfoCollections";
  protected String infoURL;
  protected String credentials;

  
  
  
  private HashMap<String, Long> timestamps;

  public HashMap<String, Long> getTimestamps() {
    if (this.timestamps == null) {
      throw new IllegalStateException("No record fetched.");
    }
    return this.timestamps;
  }

  public Long getTimestamp(String collection) {
    return this.getTimestamps().get(collection);
  }

  







  public boolean updateNeeded(String collection, long lastModified) {
    Logger.trace(LOG_TAG, "Testing " + collection + " for updateNeeded. Local last modified is " + lastModified + ".");

    
    if (lastModified <= 0) {
      return true;
    }

    
    
    Long serverLastModified = getTimestamp(collection);
    if (serverLastModified == null) {
      return true;
    }

    
    return (serverLastModified.longValue() > lastModified);
  }

  
  private InfoCollectionsDelegate callback;

  public InfoCollections(String metaURL, String credentials) {
    this.infoURL     = metaURL;
    this.credentials = credentials;
  }

  public void fetch(InfoCollectionsDelegate callback) {
    if (this.timestamps == null) {
      this.callback = callback;
      this.doFetch();
      return;
    }
    callback.handleSuccess(this);
  }

  private void doFetch() {
    try {
      final SyncStorageRecordRequest r = new SyncStorageRecordRequest(this.infoURL);
      r.delegate = this;
      
      
      ThreadPool.run(new Runnable() {
        @Override
        public void run() {
          try {
            r.get();
          } catch (Exception e) {
            callback.handleError(e);
          }
        }});
    } catch (Exception e) {
      callback.handleError(e);
    }
  }

  @SuppressWarnings("unchecked")
  public void setFromRecord(ExtendedJSONObject record) throws IllegalStateException, IOException, ParseException, NonObjectJSONException {
    Log.i(LOG_TAG, "info/collections is " + record.toJSONString());
    HashMap<String, Long> map = new HashMap<String, Long>();

    Set<Entry<String, Object>> entrySet = record.object.entrySet();
    for (Entry<String, Object> entry : entrySet) {
      
      
      String key = entry.getKey();
      Object value = entry.getValue();
      if (value instanceof Double) {
        map.put(key, Utils.decimalSecondsToMilliseconds((Double) value));
        continue;
      }
      if (value instanceof Long) {
        map.put(key, Utils.decimalSecondsToMilliseconds((Long) value));
        continue;
      }
      if (value instanceof Integer) {
        map.put(key, Utils.decimalSecondsToMilliseconds((Integer) value));
        continue;
      }
      Log.w(LOG_TAG, "Skipping info/collections entry for " + key);
    }
    this.timestamps = map;
  }

  
  public String credentials() {
    return this.credentials;
  }

  public String ifUnmodifiedSince() {
    return null;
  }

  public void handleRequestSuccess(SyncStorageResponse response) {
    if (response.wasSuccessful()) {
      try {
        this.setFromRecord(response.jsonObjectBody());
        this.callback.handleSuccess(this);
        this.callback = null;
      } catch (Exception e) {
        this.callback.handleError(e);
        this.callback = null;
      }
      return;
    }
    this.callback.handleFailure(response);
    this.callback = null;
  }

  public void handleRequestFailure(SyncStorageResponse response) {
    this.callback.handleFailure(response);
    this.callback = null;
  }

  public void handleRequestError(Exception e) {
    this.callback.handleError(e);
    this.callback = null;
  }
}
