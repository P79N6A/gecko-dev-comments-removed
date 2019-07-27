



package org.mozilla.gecko.background.common;

import java.util.Map;
import java.util.Set;

import android.content.SharedPreferences;




public class PrefsBranch implements SharedPreferences {
  private final SharedPreferences prefs;
  private final String prefix;                

  public PrefsBranch(SharedPreferences prefs, String prefix) {
    if (!prefix.endsWith(".")) {
      throw new IllegalArgumentException("No trailing period in prefix.");
    }
    this.prefs = prefs;
    this.prefix = prefix;
  }

  @Override
  public boolean contains(String key) {
    return prefs.contains(prefix + key);
  }

  @Override
  public Editor edit() {
    return new EditorBranch(prefs, prefix);
  }

  @Override
  public Map<String, ?> getAll() {
    
    return null;
  }

  @Override
  public boolean getBoolean(String key, boolean defValue) {
    return prefs.getBoolean(prefix + key, defValue);
  }

  @Override
  public float getFloat(String key, float defValue) {
    return prefs.getFloat(prefix + key, defValue);
  }

  @Override
  public int getInt(String key, int defValue) {
    return prefs.getInt(prefix + key, defValue);
  }

  @Override
  public long getLong(String key, long defValue) {
    return prefs.getLong(prefix + key, defValue);
  }

  @Override
  public String getString(String key, String defValue) {
    return prefs.getString(prefix + key, defValue);
  }

  
  
  public Set<String> getStringSet(String key, Set<String> defValue) {
    throw new RuntimeException("getStringSet not available.");
  }

  @Override
  public void registerOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener listener) {
    prefs.registerOnSharedPreferenceChangeListener(listener);
  }

  @Override
  public void unregisterOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener listener) {
    prefs.unregisterOnSharedPreferenceChangeListener(listener);
  }
}