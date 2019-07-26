



package org.mozilla.gecko.background.healthreport.prune;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.AndroidConfigurationProvider;
import org.mozilla.gecko.background.healthreport.Environment;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ConfigurationProvider;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.ProfileInformationCache;

import android.content.ContentProviderClient;
import android.content.Context;







public class PrunePolicyDatabaseStorage implements PrunePolicyStorage {
  public static final String LOG_TAG = PrunePolicyDatabaseStorage.class.getSimpleName();

  private final Context context;
  private final String profilePath;
  private final ConfigurationProvider config;

  private ContentProviderClient client;
  private HealthReportDatabaseStorage storage;

  private int currentEnvironmentID; 

  public PrunePolicyDatabaseStorage(final Context context, final String profilePath) {
    this.context = context;
    this.profilePath = profilePath;
    this.config = new AndroidConfigurationProvider(context);

    this.currentEnvironmentID = -1;
  }

  public void pruneEvents(final int count) {
    getStorage().pruneEvents(count);
  }

  public void pruneEnvironments(final int count) {
    getStorage().pruneEnvironments(count);

    
    
    this.currentEnvironmentID = -1;
    getCurrentEnvironmentID();
  }

  




  public int deleteDataBefore(final long time) {
    return getStorage().deleteDataBefore(time, getCurrentEnvironmentID());
  }

  public void cleanup() {
    final HealthReportDatabaseStorage storage = getStorage();
    
    storage.disableAutoVacuuming();
    storage.vacuum();
  }

  public int getEventCount() {
    return getStorage().getEventCount();
  }

  public int getEnvironmentCount() {
    return getStorage().getEnvironmentCount();
  }

  public void close() {
    if (client != null) {
      client.release();
      client = null;
    }
  }

  








  protected HealthReportDatabaseStorage getStorage() {
    if (storage != null) {
      return storage;
    }

    client = EnvironmentBuilder.getContentProviderClient(context);
    if (client == null) {
      
      Logger.warn(LOG_TAG, "Unable to get ContentProviderClient - throwing.");
      throw new IllegalStateException("Unable to get ContentProviderClient.");
    }

    try {
      storage = EnvironmentBuilder.getStorage(client, profilePath);
      if (storage == null) {
        
        Logger.warn(LOG_TAG,"Unable to get HealthReportDatabaseStorage for " + profilePath +
            " - throwing.");
        throw new IllegalStateException("Unable to get HealthReportDatabaseStorage for " +
            profilePath + " (== null).");
      }
    } catch (ClassCastException ex) {
      
      Logger.warn(LOG_TAG,"Unable to get HealthReportDatabaseStorage for " + profilePath +
          profilePath + " (ClassCastException).");
      throw new IllegalStateException("Unable to get HealthReportDatabaseStorage for " +
          profilePath + ".", ex);
    }

    return storage;
  }

  protected int getCurrentEnvironmentID() {
    if (currentEnvironmentID < 0) {
      final ProfileInformationCache cache = new ProfileInformationCache(profilePath);
      if (!cache.restoreUnlessInitialized()) {
        throw new IllegalStateException("Current environment unknown.");
      }
      final Environment env = EnvironmentBuilder.getCurrentEnvironment(cache, config);
      currentEnvironmentID = env.register();
    }
    return currentEnvironmentID;
  }
}
