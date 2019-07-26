



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;







public class ExtendedJSONObject {

  public JSONObject object;

  







  protected static JSONParser getJSONParser() {
    return new JSONParser();
  }

  








  protected static Object parseRaw(Reader in) throws ParseException, IOException {
    return getJSONParser().parse(in);
  }

  








  protected static Object parseRaw(String input) throws ParseException {
    return getJSONParser().parse(input);
  }

  







  public static JSONArray parseJSONArray(Reader in)
      throws IOException, ParseException, NonArrayJSONException {
    Object o = parseRaw(in);

    if (o == null) {
      return null;
    }

    if (o instanceof JSONArray) {
      return (JSONArray) o;
    }

    throw new NonArrayJSONException(o);
  }

  









  public static JSONArray parseJSONArray(String jsonString)
      throws IOException, ParseException, NonArrayJSONException {
    Object o = parseRaw(jsonString);

    if (o == null) {
      return null;
    }

    if (o instanceof JSONArray) {
      return (JSONArray) o;
    }

    throw new NonArrayJSONException(o);
  }

  







  public static ExtendedJSONObject parseJSONObject(Reader in)
      throws IOException, ParseException, NonObjectJSONException {
    return new ExtendedJSONObject(in);
  }

  









  public static ExtendedJSONObject parseJSONObject(String jsonString)
      throws IOException, ParseException, NonObjectJSONException {
    return new ExtendedJSONObject(jsonString);
  }

  







  public static ExtendedJSONObject parseUTF8AsJSONObject(byte[] in)
      throws ParseException, NonObjectJSONException, IOException {
    return parseJSONObject(new String(in, "UTF-8"));
  }

  public ExtendedJSONObject() {
    this.object = new JSONObject();
  }

  public ExtendedJSONObject(JSONObject o) {
    this.object = o;
  }

  public ExtendedJSONObject(Reader in) throws IOException, ParseException, NonObjectJSONException {
    if (in == null) {
      this.object = new JSONObject();
      return;
    }

    Object obj = parseRaw(in);
    if (obj instanceof JSONObject) {
      this.object = ((JSONObject) obj);
    } else {
      throw new NonObjectJSONException(obj);
    }
  }

  public ExtendedJSONObject(String jsonString) throws IOException, ParseException, NonObjectJSONException {
    this(jsonString == null ? null : new StringReader(jsonString));
  }

  
  public Object get(String key) {
    return this.object.get(key);
  }
  public Long getLong(String key) {
    return (Long) this.get(key);
  }
  public String getString(String key) {
    return (String) this.get(key);
  }

  






  public Integer getIntegerSafely(String key) throws NumberFormatException {
    Object val = this.object.get(key);
    if (val == null) {
      return null;
    }
    if (val instanceof Integer) {
      return (Integer) val;
    }
    if (val instanceof Long) {
      return Integer.valueOf(((Long) val).intValue());
    }
    if (val instanceof String) {
      return Integer.parseInt((String) val, 10);
    }
    throw new NumberFormatException("Expecting Integer, got " + val.getClass());
  }

  





  public Long getTimestamp(String key) {
    Object val = this.object.get(key);

    
    if (val instanceof Double) {
      double millis = ((Double) val).doubleValue() * 1000;
      return Double.valueOf(millis).longValue();
    }
    if (val instanceof Float) {
      double millis = ((Float) val).doubleValue() * 1000;
      return Double.valueOf(millis).longValue();
    }
    if (val instanceof Number) {
      
      return ((Number) val).longValue() * 1000;
    }

    return null;
  }

  public boolean containsKey(String key) {
    return this.object.containsKey(key);
  }

  public String toJSONString() {
    return this.object.toJSONString();
  }

  public String toString() {
    return this.object.toString();
  }

  public void put(String key, Object value) {
    @SuppressWarnings("unchecked")
    Map<Object, Object> map = this.object;
    map.put(key, value);
  }

  






  public boolean remove(String key) {
    Object res = this.object.remove(key);
    return (res != null);
  }

  public ExtendedJSONObject getObject(String key) throws NonObjectJSONException {
    Object o = this.object.get(key);
    if (o == null) {
      return null;
    }
    if (o instanceof ExtendedJSONObject) {
      return (ExtendedJSONObject) o;
    }
    if (o instanceof JSONObject) {
      return new ExtendedJSONObject((JSONObject) o);
    }
    throw new NonObjectJSONException(o);
  }

  @SuppressWarnings("unchecked")
  public Iterable<Entry<String, Object>> entryIterable() {
    return this.object.entrySet();
  }

  @SuppressWarnings("unchecked")
  public Set<String> keySet() {
    return this.object.keySet();
  }

  public org.json.simple.JSONArray getArray(String key) throws NonArrayJSONException {
    Object o = this.object.get(key);
    if (o == null) {
      return null;
    }
    if (o instanceof JSONArray) {
      return (JSONArray) o;
    }
    throw new NonArrayJSONException(o);
  }

  public int size() {
    return this.object.size();
  }
}
