



package org.mozilla.gecko.background.healthreport;

import java.io.UnsupportedEncodingException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Iterator;
import java.util.SortedSet;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.background.common.log.Logger;















public abstract class Environment {
  private static final String LOG_TAG = "GeckoEnvironment";

  public static int VERSION = 1;

  protected final Class<? extends EnvironmentAppender> appenderClass;

  protected volatile String hash = null;
  protected volatile int id = -1;

  
  public int profileCreation;

  
  public int cpuCount;
  public int memoryMB;
  public String architecture;
  public String sysName;
  public String sysVersion;      

  
  public String vendor;
  public String appName;
  public String appID;
  public String appVersion;
  public String appBuildID;
  public String platformVersion;
  public String platformBuildID;
  public String os;
  public String xpcomabi;
  public String updateChannel;

  
  public int isBlocklistEnabled;
  public int isTelemetryEnabled;
  

  
  public JSONObject addons = null;

  
  public int extensionCount;
  public int pluginCount;
  public int themeCount;

  public Environment() {
    this(Environment.HashAppender.class);
  }

  public Environment(Class<? extends EnvironmentAppender> appenderClass) {
    this.appenderClass = appenderClass;
  }

  public JSONObject getNonIgnoredAddons() {
    if (addons == null) {
      return null;
    }
    JSONObject out = new JSONObject();
    @SuppressWarnings("unchecked")
    Iterator<String> keys = addons.keys();
    while (keys.hasNext()) {
      try {
        final String key = keys.next();
        final Object obj = addons.get(key);
        if (obj != null && obj instanceof JSONObject && ((JSONObject) obj).optBoolean("ignore", false)) {
          continue;
        }
        out.put(key, obj);
      } catch (JSONException ex) {
        
      }
    }
    return out;
  }

  



  public static abstract class EnvironmentAppender {
    public abstract void append(String s);
    public abstract void append(int v);
  }

  public static class HashAppender extends EnvironmentAppender {
    final MessageDigest hasher;

    public HashAppender() throws NoSuchAlgorithmException {
      
      
      
      
      
      hasher = MessageDigest.getInstance("SHA-1");
    }

    @Override
    public void append(String s) {
      try {
        hasher.update(((s == null) ? "null" : s).getBytes("UTF-8"));
      } catch (UnsupportedEncodingException e) {
        
      }
    }

    @Override
    public void append(int profileCreation) {
      append(Integer.toString(profileCreation, 10));
    }

    @Override
    public String toString() {
      
      
      return new Base64(-1, null, false).encodeAsString(hasher.digest());
    }
  }

  




  public String getHash() {
    
    if (hash != null) {
      return hash;
    }

    EnvironmentAppender appender;
    try {
      appender = appenderClass.newInstance();
    } catch (InstantiationException ex) {
      
      Logger.warn(LOG_TAG,  "Could not compute hash.", ex);
      return null;
    } catch (IllegalAccessException ex) {
      
      Logger.warn(LOG_TAG,  "Could not compute hash.", ex);
      return null;
    }

    appender.append(profileCreation);
    appender.append(cpuCount);
    appender.append(memoryMB);
    appender.append(architecture);
    appender.append(sysName);
    appender.append(sysVersion);
    appender.append(vendor);
    appender.append(appName);
    appender.append(appID);
    appender.append(appVersion);
    appender.append(appBuildID);
    appender.append(platformVersion);
    appender.append(platformBuildID);
    appender.append(os);
    appender.append(xpcomabi);
    appender.append(updateChannel);
    appender.append(isBlocklistEnabled);
    appender.append(isTelemetryEnabled);
    appender.append(extensionCount);
    appender.append(pluginCount);
    appender.append(themeCount);

    
    if (addons != null) {
      appendSortedAddons(getNonIgnoredAddons(), appender);
    }

    return hash = appender.toString();
  }

  



  public static void appendSortedAddons(JSONObject addons,
                                        final EnvironmentAppender builder) {
    final SortedSet<String> keys = HealthReportUtils.sortedKeySet(addons);

    
    for (String key : keys) {
      try {
        JSONObject addon = addons.getJSONObject(key);

        
        builder.append(key);
        builder.append("={");

        for (String addonKey : HealthReportUtils.sortedKeySet(addon)) {
          builder.append(addonKey);
          builder.append("==");
          try {
            builder.append(addon.get(addonKey).toString());
          } catch (JSONException e) {
            builder.append("_e_");
          }
        }

        builder.append("}");
      } catch (Exception e) {
        
        Logger.warn(LOG_TAG, "Invalid add-on for ID " + key);
      }
    }
  }

  public void setJSONForAddons(byte[] json) throws Exception {
    setJSONForAddons(new String(json, "UTF-8"));
  }

  public void setJSONForAddons(String json) throws Exception {
    if (json == null || "null".equals(json)) {
      addons = null;
      return;
    }
    addons = new JSONObject(json);
  }

  public void setJSONForAddons(JSONObject json) {
    addons = json;
  }

  


  public String getNormalizedAddonsJSON() {
    
    
    return (addons == null) ? "null" : addons.toString();
  }

  








  public abstract int register();
}
