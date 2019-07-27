



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
    
    
    if (Versions.feature20Plus) {
      DEFAULT_CIPHER_SUITES = new String[]
          {
           "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",   
           "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",     
           "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256",     
           "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",        
           "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",     
           "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384",     
           "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA",        
          };
    } else if (Versions.feature11Plus) {
      DEFAULT_CIPHER_SUITES = new String[]
          {
           "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",        
           "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA",      
           "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA",        
           "TLS_RSA_WITH_AES_256_CBC_SHA",              
          };
    } else {       
      
      
      
      
      DEFAULT_CIPHER_SUITES = new String[]
          {
           
           "TLS_DHE_RSA_WITH_AES_128_CBC_SHA",
           "TLS_DHE_DSS_WITH_AES_128_CBC_SHA",

           
           "TLS_DHE_RSA_WITH_AES_256_CBC_SHA",          
           "TLS_RSA_WITH_AES_256_CBC_SHA",              
          };
    }

    if (Versions.feature16Plus) {
      DEFAULT_PROTOCOLS = new String[]
          {
           "TLSv1.2",
           "TLSv1.1",
           "TLSv1",             
          };
    } else {
      
      DEFAULT_PROTOCOLS = new String[]
          {
           "TLSv1",
          };
    }
  }
}
