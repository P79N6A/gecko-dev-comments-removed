



package org.mozilla.gecko.background.common;

import org.mozilla.gecko.AppConstants;





public class GlobalConstants {
  public static final String BROWSER_INTENT_PACKAGE = AppConstants.ANDROID_PACKAGE_NAME;
  public static final String BROWSER_INTENT_CLASS = AppConstants.BROWSER_INTENT_CLASS_NAME;

  




  public static final String PER_ANDROID_PACKAGE_PERMISSION = AppConstants.ANDROID_PACKAGE_NAME + ".permission.PER_ANDROID_PACKAGE";

  public static final int SHARED_PREFERENCES_MODE = 0;

  
  
  
  
  
  public static String GECKO_PREFERENCES_CLASS = "org.mozilla.gecko.preferences.GeckoPreferences";
  public static String GECKO_BROADCAST_HEALTHREPORT_UPLOAD_PREF_METHOD  = "broadcastHealthReportUploadPref";
  public static String GECKO_BROADCAST_HEALTHREPORT_PRUNE_METHOD = "broadcastHealthReportPrune";

  
  public static final long MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
  public static final long MILLISECONDS_PER_SIX_MONTHS = 180 * MILLISECONDS_PER_DAY;
}
