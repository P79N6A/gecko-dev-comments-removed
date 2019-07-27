



package org.mozilla.gecko.background.common;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.AppConstants.Versions;




public class GlobalConstants {
  public static final String BROWSER_INTENT_PACKAGE = AppConstants.ANDROID_PACKAGE_NAME;
  public static final String BROWSER_INTENT_CLASS = AppConstants.MOZ_ANDROID_BROWSER_INTENT_CLASS;

  




  public static final String PER_ANDROID_PACKAGE_PERMISSION = AppConstants.ANDROID_PACKAGE_NAME + ".permission.PER_ANDROID_PACKAGE";

  public static final int SHARED_PREFERENCES_MODE = 0;

  
  
  
  
  
  public static String GECKO_PREFERENCES_CLASS = "org.mozilla.gecko.preferences.GeckoPreferences";
  public static String GECKO_BROADCAST_HEALTHREPORT_UPLOAD_PREF_METHOD  = "broadcastHealthReportUploadPref";
  public static String GECKO_BROADCAST_HEALTHREPORT_PRUNE_METHOD = "broadcastHealthReportPrune";

  
  public static final long MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
  public static final long MILLISECONDS_PER_SIX_MONTHS = 180 * MILLISECONDS_PER_DAY;

  
  



  public static final String[] DEFAULT_CIPHER_SUITES;
  public static final String[] DEFAULT_PROTOCOLS;

  static {
    DEFAULT_CIPHER_SUITES = new String[]
        {
          "TLS_DHE_RSA_WITH_AES_256_CBC_SHA",
          "TLS_DHE_RSA_WITH_AES_128_CBC_SHA",
          "SSL_RSA_WITH_RC4_128_SHA", 
        };
    DEFAULT_PROTOCOLS = new String[]
        {
          "SSLv3",
          "TLSv1",
        };
  }
}
