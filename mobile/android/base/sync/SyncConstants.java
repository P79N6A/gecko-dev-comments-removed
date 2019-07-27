



package org.mozilla.gecko.sync;

import org.mozilla.gecko.AppConstants;

public class SyncConstants {
  public static final String GLOBAL_LOG_TAG = "FxSync";
  public static final String SYNC_MAJOR_VERSION  = "1";
  public static final String SYNC_MINOR_VERSION  = "0";
  public static final String SYNC_VERSION_STRING = SYNC_MAJOR_VERSION + "." +
                                                   AppConstants.MOZ_APP_VERSION + "." +
                                                   SYNC_MINOR_VERSION;

  public static final String USER_AGENT = "Firefox AndroidSync " +
                                          SYNC_VERSION_STRING + " (" +
                                          AppConstants.MOZ_APP_DISPLAYNAME + ")";

  public static final String ACCOUNTTYPE_SYNC = AppConstants.MOZ_ANDROID_SHARED_ACCOUNT_TYPE;

  











  public static final String SYNC_ACCOUNT_DELETED_ACTION = AppConstants.MOZ_ANDROID_SHARED_ACCOUNT_TYPE + ".accounts.SYNC_ACCOUNT_DELETED_ACTION";

  






  public static final long SYNC_ACCOUNT_DELETED_INTENT_VERSION = 1;

  




  public static final String PER_ACCOUNT_TYPE_PERMISSION = AppConstants.MOZ_ANDROID_SHARED_ACCOUNT_TYPE + ".permission.PER_ACCOUNT_TYPE";

  public static final String DEFAULT_AUTH_SERVER = "https://auth.services.mozilla.com/";

  
  public static final String BACKOFF_PREF_SUFFIX_11 = "sync";
}
