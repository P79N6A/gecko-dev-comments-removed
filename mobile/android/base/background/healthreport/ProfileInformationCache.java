



package org.mozilla.gecko.background.healthreport;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.Locale;
import java.util.Scanner;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ProfileInformationProvider;







public class ProfileInformationCache implements ProfileInformationProvider {
  private static final String LOG_TAG = "GeckoProfileInfo";
  private static final String CACHE_FILE = "profile_info_cache.json";

  






  public static final int FORMAT_VERSION = 3;

  protected boolean initialized = false;
  protected boolean needsWrite = false;

  protected final File file;

  private volatile boolean blocklistEnabled = true;
  private volatile boolean telemetryEnabled = false;
  private volatile boolean isAcceptLangUserSet = false;

  private volatile long profileCreationTime = 0;
  private volatile String distribution = "";

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  private volatile String osLocale = "";
  private volatile String appLocale = "";

  private volatile JSONObject addons = null;

  public ProfileInformationCache(String profilePath) {
    file = new File(profilePath + File.separator + CACHE_FILE);
    Logger.pii(LOG_TAG, "Using " + file.getAbsolutePath() + " for profile information cache.");
  }

  public synchronized void beginInitialization() {
    initialized = false;
    needsWrite = true;
  }

  public JSONObject toJSON() {
    JSONObject object = new JSONObject();
    try {
      object.put("version", FORMAT_VERSION);
      object.put("blocklist", blocklistEnabled);
      object.put("telemetry", telemetryEnabled);
      object.put("isAcceptLangUserSet", isAcceptLangUserSet);
      object.put("profileCreated", profileCreationTime);
      object.put("osLocale", osLocale);
      object.put("appLocale", appLocale);
      object.put("distribution", distribution);
      object.put("addons", addons);
    } catch (JSONException e) {
      
      
      return null;
    }
    return object;
  }

  







  private boolean fromJSON(JSONObject object) throws JSONException {
    int version = object.optInt("version", 1);
    switch (version) {
    case FORMAT_VERSION:
      blocklistEnabled = object.getBoolean("blocklist");
      telemetryEnabled = object.getBoolean("telemetry");
      isAcceptLangUserSet = object.getBoolean("isAcceptLangUserSet");
      profileCreationTime = object.getLong("profileCreated");
      addons = object.getJSONObject("addons");
      distribution = object.getString("distribution");
      osLocale = object.getString("osLocale");
      appLocale = object.getString("appLocale");
      return true;
    default:
      Logger.warn(LOG_TAG, "Unable to restore from version " + version + " PIC file: expecting " + FORMAT_VERSION);
      return false;
    }
  }

  protected JSONObject readFromFile() throws FileNotFoundException, JSONException {
    Scanner scanner = null;
    try {
      scanner = new Scanner(file, "UTF-8");
      final String contents = scanner.useDelimiter("\\A").next();
      return new JSONObject(contents);
    } finally {
      if (scanner != null) {
        scanner.close();
      }
    }
  }

  protected void writeToFile(JSONObject object) throws IOException {
    Logger.debug(LOG_TAG, "Writing profile information.");
    Logger.pii(LOG_TAG, "Writing to file: " + file.getAbsolutePath());
    FileOutputStream stream = new FileOutputStream(file);
    OutputStreamWriter writer = new OutputStreamWriter(stream, Charset.forName("UTF-8"));
    try {
      writer.append(object.toString());
      needsWrite = false;
    } finally {
      writer.close();
    }
  }

  



  public synchronized void completeInitialization() throws IOException {
    initialized = true;
    if (!needsWrite) {
      Logger.debug(LOG_TAG, "No write needed.");
      return;
    }

    JSONObject object = toJSON();
    if (object == null) {
      throw new IOException("Couldn't serialize JSON.");
    }

    writeToFile(object);
  }

  






  public synchronized boolean restoreUnlessInitialized() {
    if (initialized) {
      return true;
    }

    if (!file.exists()) {
      return false;
    }

    
    Logger.info(LOG_TAG, "Restoring ProfileInformationCache from file.");
    Logger.pii(LOG_TAG, "Restoring from file: " + file.getAbsolutePath());

    try {
      if (!fromJSON(readFromFile())) {
        
        return false;
      }
      initialized = true;
      needsWrite = false;
      return true;
    } catch (FileNotFoundException e) {
      return false;
    } catch (JSONException e) {
      Logger.warn(LOG_TAG, "Malformed ProfileInformationCache. Not restoring.");
      return false;
    }
  }

  private void ensureInitialized() {
    if (!initialized) {
      throw new IllegalStateException("Not initialized.");
    }
  }

  @Override
  public boolean isBlocklistEnabled() {
    ensureInitialized();
    return blocklistEnabled;
  }

  public void setBlocklistEnabled(boolean value) {
    Logger.debug(LOG_TAG, "Setting blocklist enabled: " + value);
    blocklistEnabled = value;
    needsWrite = true;
  }

  @Override
  public boolean isTelemetryEnabled() {
    ensureInitialized();
    return telemetryEnabled;
  }

  public void setTelemetryEnabled(boolean value) {
    Logger.debug(LOG_TAG, "Setting telemetry enabled: " + value);
    telemetryEnabled = value;
    needsWrite = true;
  }

  @Override
  public boolean isAcceptLangUserSet() {
    ensureInitialized();
    return isAcceptLangUserSet;
  }

  public void setAcceptLangUserSet(boolean value) {
    Logger.debug(LOG_TAG, "Setting accept-lang as user-set: " + value);
    isAcceptLangUserSet = value;
    needsWrite = true;
  }

  @Override
  public long getProfileCreationTime() {
    ensureInitialized();
    return profileCreationTime;
  }

  public void setProfileCreationTime(long value) {
    Logger.debug(LOG_TAG, "Setting profile creation time: " + value);
    profileCreationTime = value;
    needsWrite = true;
  }

  @Override
  public String getDistributionString() {
    ensureInitialized();
    return distribution;
  }

  


  public void setDistributionString(String distributionID, String distributionVersion) {
    Logger.debug(LOG_TAG, "Setting distribution: " + distributionID + ", " + distributionVersion);
    distribution = distributionID + ":" + distributionVersion;
    needsWrite = true;
  }

  @Override
  public String getAppLocale() {
    ensureInitialized();
    return appLocale;
  }

  public void setAppLocale(String value) {
    if (value.equalsIgnoreCase(appLocale)) {
      return;
    }
    Logger.debug(LOG_TAG, "Setting app locale: " + value);
    appLocale = value.toLowerCase(Locale.US);
    needsWrite = true;
  }

  @Override
  public String getOSLocale() {
    ensureInitialized();
    return osLocale;
  }

  public void setOSLocale(String value) {
    if (value.equalsIgnoreCase(osLocale)) {
      return;
    }
    Logger.debug(LOG_TAG, "Setting OS locale: " + value);
    osLocale = value.toLowerCase(Locale.US);
    needsWrite = true;
  }

  




  public boolean updateLocales(String osLocale, String appLocale) {
    if (this.osLocale.equalsIgnoreCase(osLocale) &&
        (appLocale == null || this.appLocale.equalsIgnoreCase(appLocale))) {
      return false;
    }
    this.setOSLocale(osLocale);
    if (appLocale != null) {
      this.setAppLocale(appLocale);
    }
    return true;
  }

  @Override
  public JSONObject getAddonsJSON() {
    ensureInitialized();
    return addons;
  }

  public void updateJSONForAddon(String id, String json) throws Exception {
    addons.put(id, new JSONObject(json));
    needsWrite = true;
  }

  public void removeAddon(String id) {
    if (null != addons.remove(id)) {
      needsWrite = true;
    }
  }

  


  public void updateJSONForAddon(String id, JSONObject json) {
    if (addons == null) {
      throw new IllegalStateException("Cannot incrementally update add-ons without first initializing.");
    }
    try {
      addons.put(id, json);
      needsWrite = true;
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Unexpected failure updating JSON for add-on.", e);
    }
  }

  




  public void setJSONForAddons(String json) throws Exception {
    addons = new JSONObject(json);
    needsWrite = true;
  }

  public void setJSONForAddons(JSONObject json) {
    addons = json;
    needsWrite = true;
  }
}
