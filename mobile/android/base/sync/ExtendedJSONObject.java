




































package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.util.Map;
import java.util.Map.Entry;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;







public class ExtendedJSONObject {

  public JSONObject object;

  private static Object processParseOutput(Object parseOutput) {
    if (parseOutput instanceof JSONObject) {
      return new ExtendedJSONObject((JSONObject) parseOutput);
    } else {
      return parseOutput;
    }
  }

  public static Object parse(String string) throws IOException, ParseException {
    return processParseOutput(new JSONParser().parse(string));
  }

  public static Object parse(InputStreamReader reader) throws IOException, ParseException {
    return processParseOutput(new JSONParser().parse(reader));

  }

  public static Object parse(InputStream stream) throws IOException, ParseException {
    InputStreamReader reader = new InputStreamReader(stream, "UTF-8");
    return ExtendedJSONObject.parse(reader);
  }

  








  public static ExtendedJSONObject parseJSONObject(String jsonString)
                                                                     throws IOException,
                                                                     ParseException,
                                                                     NonObjectJSONException {
    return new ExtendedJSONObject(jsonString);
  }

  public ExtendedJSONObject() {
    this.object = new JSONObject();
  }

  public ExtendedJSONObject(JSONObject o) {
    this.object = o;
  }

  public ExtendedJSONObject(String jsonString) throws IOException, ParseException, NonObjectJSONException {
    if (jsonString == null) {
      this.object = new JSONObject();
      return;
    }
    Reader in = new StringReader(jsonString);
    Object obj = new JSONParser().parse(in);
    if (obj instanceof JSONObject) {
      this.object = ((JSONObject) obj);
    } else {
      throw new NonObjectJSONException(obj);
    }
  }

  
  public Object get(String key) {
    return this.object.get(key);
  }
  public Long getLong(String key) {
    return (Long) this.get(key);
  }

  




  public Long getTimestamp(String key) {
    Object val = this.object.get(key);

    
    if (val instanceof Double) {
      double millis = ((Double) val).doubleValue() * 1000;
      return new Double(millis).longValue();
    }
    if (val instanceof Float) {
      double millis = ((Float) val).doubleValue() * 1000;
      return new Double(millis).longValue();
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

  public ExtendedJSONObject clone() {
    return new ExtendedJSONObject((JSONObject) this.object.clone());
  }

  










  public ExtendedJSONObject getJSONObject(String key) throws IOException,
                                                     ParseException,
                                                     NonObjectJSONException {
    String val = (String) this.object.get(key);
    return ExtendedJSONObject.parseJSONObject(val);
  }

  @SuppressWarnings("unchecked")
  public Iterable<Entry<String, Object>> entryIterable() {
    return this.object.entrySet();
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
}
