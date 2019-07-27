



package org.mozilla.gecko.fxa;

import org.mozilla.gecko.AppConstants;

public class FxAccountConstants {
  public static final String GLOBAL_LOG_TAG = "FxAccounts";
  public static final String ACCOUNT_TYPE = AppConstants.MOZ_ANDROID_SHARED_FXACCOUNT_TYPE;

  
  public static final String OAUTH_CLIENT_ID_FENNEC = "3332a18d142636cb";

  public static final String DEFAULT_AUTH_SERVER_ENDPOINT = "https://api.accounts.firefox.com/v1";
  public static final String DEFAULT_TOKEN_SERVER_ENDPOINT = "https://token.services.mozilla.com/1.0/sync/1.5";
  public static final String DEFAULT_OAUTH_SERVER_ENDPOINT = "https://oauth.accounts.firefox.com/v1";

  public static final String STAGE_AUTH_SERVER_ENDPOINT = "https://stable.dev.lcip.org/auth/v1";
  public static final String STAGE_TOKEN_SERVER_ENDPOINT = "https://stable.dev.lcip.org/syncserver/token/1.0/sync/1.5";
  public static final String STAGE_OAUTH_SERVER_ENDPOINT = "https://oauth-stable.dev.lcip.org/v1";

  
  public static final int MINIMUM_AGE_TO_CREATE_AN_ACCOUNT = 13;

  
  public static final long MINIMUM_TIME_TO_WAIT_AFTER_AGE_CHECK_FAILED_IN_MILLISECONDS = 15 * 60 * 1000;

  public static final String USER_AGENT = "Firefox-Android-FxAccounts/" + AppConstants.MOZ_APP_VERSION + " (" + AppConstants.MOZ_APP_DISPLAYNAME + ")";

  public static final String ACCOUNT_PICKLE_FILENAME = "fxa.account.json";

  









  public static final String OLD_SYNC_ACCOUNT_PICKLE_FILENAME = "old.sync.account.json";

  













  public static final String ACCOUNT_DELETED_ACTION = AppConstants.MOZ_ANDROID_SHARED_FXACCOUNT_TYPE + ".accounts.ACCOUNT_DELETED_ACTION";

  


  public static final long ACCOUNT_DELETED_INTENT_VERSION = 1;

  public static final String ACCOUNT_DELETED_INTENT_VERSION_KEY = "account_deleted_intent_version";
  public static final String ACCOUNT_DELETED_INTENT_ACCOUNT_KEY = "account_deleted_intent_account";
  public static final String ACCOUNT_OAUTH_SERVICE_ENDPOINT_KEY = "account_oauth_service_endpoint";
  public static final String ACCOUNT_DELETED_INTENT_ACCOUNT_AUTH_TOKENS = "account_deleted_intent_auth_tokens";

  



  public static final String PER_ACCOUNT_TYPE_PERMISSION = AppConstants.MOZ_ANDROID_SHARED_FXACCOUNT_TYPE + ".permission.PER_ACCOUNT_TYPE";

  







  public static final String ACCOUNT_STATE_CHANGED_ACTION = AppConstants.MOZ_ANDROID_SHARED_FXACCOUNT_TYPE + ".accounts.ACCOUNT_STATE_CHANGED_ACTION";
}
