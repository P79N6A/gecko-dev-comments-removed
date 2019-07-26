



package org.mozilla.gecko.sync;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;






public class InfoCollections {
  private static final String LOG_TAG = "InfoCollections";

  





  final Map<String, Long> timestamps;

  @SuppressWarnings("unchecked")
  public InfoCollections(final ExtendedJSONObject record) {
    Logger.debug(LOG_TAG, "info/collections is " + record.toJSONString());
    HashMap<String, Long> map = new HashMap<String, Long>();

    Set<Entry<String, Object>> entrySet = record.object.entrySet();

    String key;
    Object value;
    for (Entry<String, Object> entry : entrySet) {
      key = entry.getKey();
      value = entry.getValue();

      
      
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
      Logger.warn(LOG_TAG, "Skipping info/collections entry for " + key);
    }

    this.timestamps = Collections.unmodifiableMap(map);
  }

  







  public Long getTimestamp(String collection) {
    if (timestamps == null) {
      return null;
    }
    return timestamps.get(collection);
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
}
