



package org.mozilla.gecko.sync;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;

public class InfoCounts {
  static final String LOG_TAG = "InfoCounts";

  


  private Map<String, Integer> counts = null;

  @SuppressWarnings("unchecked")
  public InfoCounts(final ExtendedJSONObject record) {
    Logger.debug(LOG_TAG, "info/collection_counts is " + record.toJSONString());
    HashMap<String, Integer> map = new HashMap<String, Integer>();

    Set<Entry<String, Object>> entrySet = record.object.entrySet();

    String key;
    Object value;

    for (Entry<String, Object> entry : entrySet) {
      key = entry.getKey();
      value = entry.getValue();

      if (value instanceof Integer) {
        map.put(key, (Integer) value);
        continue;
      }

      if (value instanceof Long) {
        map.put(key, ((Long) value).intValue());
        continue;
      }

      Logger.warn(LOG_TAG, "Skipping info/collection_counts entry for " + key);
    }

    this.counts = Collections.unmodifiableMap(map);
  }

  







  public Integer getCount(String collection) {
    if (counts == null) {
      return null;
    }
    return counts.get(collection);
  }
}
