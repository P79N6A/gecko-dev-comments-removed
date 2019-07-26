



package org.mozilla.gecko.background.healthreport;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.SysInfo;
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
    public long getProfileCreationTime();
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

    e.profileCreation = (int) (info.getProfileCreationTime() / HealthReportConstants.MILLISECONDS_PER_DAY);

    
    e.isBlocklistEnabled = (info.isBlocklistEnabled() ? 1 : 0);

    
    
    e.isTelemetryEnabled = (info.isTelemetryEnabled() ? 1 : 0);

    
    e.extensionCount = 0;
    e.pluginCount = 0;
    e.themeCount = 0;
    

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
    return e.register();
  }
}
