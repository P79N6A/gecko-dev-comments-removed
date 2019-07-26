



package org.mozilla.gecko.background.healthreport;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.UUID;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.apache.commons.codec.digest.DigestUtils;

import android.content.ContentUris;
import android.net.Uri;

public class HealthReportUtils {
  public static final String LOG_TAG = HealthReportUtils.class.getSimpleName();

  public static String getEnvironmentHash(final String input) {
    return DigestUtils.shaHex(input);
  }

  









  public static Uri getEventURI(Uri environmentURI) {
    return environmentURI.buildUpon().path("/events/" + ContentUris.parseId(environmentURI) + "/").build();
  }

  


  private static <T extends Set<String>> T intoKeySet(T keys, JSONObject o) {
    if (o == null || o == JSONObject.NULL) {
      return keys;
    }

    @SuppressWarnings("unchecked")
    Iterator<String> it = o.keys();
    while (it.hasNext()) {
      keys.add(it.next());
    }
    return keys;
  }

  






  public static SortedSet<String> sortedKeySet(JSONObject o) {
    return intoKeySet(new TreeSet<String>(), o);
  }

  




  public static Set<String> keySet(JSONObject o) {
    return intoKeySet(new HashSet<String>(), o);
  }

  



  public static void append(JSONObject o, String key, Object value) throws JSONException {
    if (!o.has(key)) {
      JSONArray arr = new JSONArray();
      arr.put(value);
      o.put(key, arr);
      return;
    }
    Object dest = o.get(key);
    if (dest instanceof JSONArray) {
      ((JSONArray) dest).put(value);
      return;
    }
    JSONArray arr = new JSONArray();
    arr.put(dest);
    arr.put(value);
    o.put(key, arr);
  }

  



















  public static void count(JSONObject o, String key,
                           String value) throws JSONException {
    if (!o.has(key)) {
      JSONObject counts = new JSONObject();
      counts.put(value, 1);
      o.put(key, counts);
      return;
    }
    JSONObject dest = o.getJSONObject(key);
    dest.put(value, dest.optInt(value, 0) + 1);
  }

  public static String generateDocumentId() {
    return UUID.randomUUID().toString();
  }
}
