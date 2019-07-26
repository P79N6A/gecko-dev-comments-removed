



package org.mozilla.gecko.background.healthreport;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.Scanner;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ProfileInformationProvider;









public class ProfileInformationCache implements ProfileInformationProvider {
  private static final String LOG_TAG = "GeckoProfileInfo";
  private static final String CACHE_FILE = "profile_info_cache.json";

  protected boolean initialized = false;
  protected boolean needsWrite = false;

  protected final File file;

  private volatile boolean blocklistEnabled = true;
  private volatile boolean telemetryEnabled = false;
  private volatile long profileCreationTime = 0;

  public ProfileInformationCache(String profilePath) {
    file = new File(profilePath + File.separator + CACHE_FILE);
    Logger.info(LOG_TAG, "Using " + file.getAbsolutePath() + " for profile information cache.");
  }

  public synchronized void beginInitialization() {
    initialized = false;
    needsWrite = true;
    profileCreationTime = 0;
  }

  public JSONObject toJSON() {
    JSONObject object = new JSONObject();
    try {
      object.put("blocklist", blocklistEnabled);
      object.put("telemetry", telemetryEnabled);
      object.put("profileCreated", profileCreationTime);
    } catch (JSONException e) {
      
      
      return null;
    }
    return object;
  }

  private void fromJSON(JSONObject object) throws JSONException {
    blocklistEnabled = object.getBoolean("blocklist");
    telemetryEnabled = object.getBoolean("telemetry");
    profileCreationTime = object.getLong("profileCreated");
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

    Logger.info(LOG_TAG, "Writing profile information to " + file.getAbsolutePath());
    FileOutputStream stream = new FileOutputStream(file);
    OutputStreamWriter writer = new OutputStreamWriter(stream, Charset.forName("UTF-8"));
    try {
      writer.append(object.toString());
      needsWrite = false;
    } finally {
      writer.close();
    }
  }

  






  public synchronized boolean restoreUnlessInitialized() {
    if (initialized) {
      return true;
    }

    if (!file.exists()) {
      return false;
    }

    
    Logger.info(LOG_TAG, "Restoring ProfileInformationCache from " + file.getAbsolutePath());
    Scanner scanner = null;
    try {
      scanner = new Scanner(file, "UTF-8");
      final String contents = scanner.useDelimiter("\\A").next();
      fromJSON(new JSONObject(contents));
      initialized = true;
      return true;
    } catch (FileNotFoundException e) {
      return false;
    } catch (JSONException e) {
      return false;
    } finally {
      if (scanner != null) {
        scanner.close();
      }
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
  public long getProfileCreationTime() {
    ensureInitialized();
    return profileCreationTime;
  }

  public void setProfileCreationTime(long value) {
    Logger.debug(LOG_TAG, "Setting profile creation time: " + value);
    profileCreationTime = value;
    needsWrite = true;
  }
}
