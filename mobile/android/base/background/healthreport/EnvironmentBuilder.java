



package org.mozilla.gecko.background.healthreport;

import java.util.Iterator;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.SysInfo;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;

import android.content.ContentProvider;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;




public class EnvironmentBuilder {
  private static final String LOG_TAG = "GeckoEnvBuilder";

  public static ContentProviderClient getContentProviderClient(Context context) {
    ContentResolver cr = context.getContentResolver();
    return cr.acquireContentProviderClient(HealthReportConstants.HEALTH_AUTHORITY);
  }

  











  public static HealthReportDatabaseStorage getStorage(ContentProviderClient cpc,
                                                       String profilePath) {
    ContentProvider pr = cpc.getLocalContentProvider();
    if (pr == null) {
      Logger.error(LOG_TAG, "Unable to retrieve local content provider. Running in a different process?");
      return null;
    }
    try {
      return ((HealthReportProvider) pr).getProfileStorage(profilePath);
    } catch (ClassCastException ex) {
      Logger.error(LOG_TAG, "ContentProvider not a HealthReportProvider!", ex);
      throw ex;
    }
  }

  public static interface ProfileInformationProvider {
    public boolean isBlocklistEnabled();
    public boolean isTelemetryEnabled();
    public boolean isAcceptLangUserSet();
    public long getProfileCreationTime();

    public String getDistributionString();
    public String getOSLocale();
    public String getAppLocale();

    public JSONObject getAddonsJSON();
  }

  protected static void populateEnvironment(Environment e,
                                            ProfileInformationProvider info) {
    e.cpuCount = SysInfo.getCPUCount();
    e.memoryMB = SysInfo.getMemSize();

    e.appName = AppConstants.MOZ_APP_NAME;
    e.appID = AppConstants.MOZ_APP_ID;
    e.appVersion = AppConstants.MOZ_APP_VERSION;
    e.appBuildID = AppConstants.MOZ_APP_BUILDID;
    e.updateChannel = AppConstants.MOZ_UPDATE_CHANNEL;
    e.vendor = AppConstants.MOZ_APP_VENDOR;
    e.platformVersion = AppConstants.MOZILLA_VERSION;
    e.platformBuildID = AppConstants.MOZ_APP_BUILDID;
    e.xpcomabi = AppConstants.TARGET_XPCOM_ABI;
    e.os = "Android";
    e.architecture = SysInfo.getArchABI();       
    e.sysName = SysInfo.getName();
    e.sysVersion = SysInfo.getReleaseVersion();

    e.profileCreation = (int) (info.getProfileCreationTime() / GlobalConstants.MILLISECONDS_PER_DAY);

    
    e.isBlocklistEnabled = (info.isBlocklistEnabled() ? 1 : 0);

    
    
    e.isTelemetryEnabled = (info.isTelemetryEnabled() ? 1 : 0);

    e.extensionCount = 0;
    e.pluginCount = 0;
    e.themeCount = 0;

    JSONObject addons = info.getAddonsJSON();
    if (addons == null) {
      return;
    }

    @SuppressWarnings("unchecked")
    Iterator<String> it = addons.keys();
    while (it.hasNext()) {
      String key = it.next();
      try {
        JSONObject addon = addons.getJSONObject(key);
        String type = addon.optString("type");
        Logger.pii(LOG_TAG, "Add-on " + key + " is a " + type);
        if ("extension".equals(type)) {
          ++e.extensionCount;
        } else if ("plugin".equals(type)) {
          ++e.pluginCount;
        } else if ("theme".equals(type)) {
          ++e.themeCount;
        } else if ("service".equals(type)) {
          
        } else {
          Logger.debug(LOG_TAG, "Unknown add-on type: " + type);
        }
      } catch (Exception ex) {
        Logger.warn(LOG_TAG, "Failed to process add-on " + key, ex);
      }
    }

    e.addons = addons;

    
    e.distribution = info.getDistributionString();
    e.osLocale = info.getOSLocale();
    e.appLocale = info.getAppLocale();
    e.acceptLangSet = info.isAcceptLangUserSet() ? 1 : 0;
  }

  






  public static Environment getCurrentEnvironment(ProfileInformationProvider info) {
    Environment e = new Environment() {
      @Override
      public int register() {
        return 0;
      }
    };
    populateEnvironment(e, info);
    return e;
  }

  


  public static int registerCurrentEnvironment(HealthReportDatabaseStorage storage,
                                               ProfileInformationProvider info) {
    Environment e = storage.getEnvironment();
    populateEnvironment(e, info);
    e.register();
    Logger.debug(LOG_TAG, "Registering current environment: " + e.getHash() + " = " + e.id);
    return e.id;
  }
}
