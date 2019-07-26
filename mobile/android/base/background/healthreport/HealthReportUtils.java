



package org.mozilla.gecko.background.healthreport;

import java.text.SimpleDateFormat;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale;
import java.util.Set;
import java.util.SortedSet;
import java.util.TimeZone;
import java.util.TreeSet;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;
import org.mozilla.apache.commons.codec.digest.DigestUtils;

import android.content.ContentUris;
import android.net.Uri;

public class HealthReportUtils {
  public static int getDay(final long time) {
    return (int) Math.floor(time / HealthReportConstants.MILLISECONDS_PER_DAY);
  }

  public static String getEnvironmentHash(final String input) {
    return DigestUtils.shaHex(input);
  }

  public static String getDateStringForDay(long day) {
    return getDateString(HealthReportConstants.MILLISECONDS_PER_DAY * day);
  }

  public static String getDateString(long time) {
    final SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd", Locale.US);
    format.setTimeZone(TimeZone.getTimeZone("UTC"));
    return format.format(time);
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

  









  public static JSONObject shallowCopyObject(JSONObject o) throws JSONException {
    if (o == null) {
      return null;
    }

    JSONObject out = new JSONObject();
    @SuppressWarnings("unchecked")
    Iterator<String> keys = out.keys();
    while (keys.hasNext()) {
      final String key = keys.next();
      out.put(key, o.get(key));
    }
    return out;
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
    return;
  }
}
