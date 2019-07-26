



package org.mozilla.gecko.sync.setup;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.CredentialException;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.config.AccountPickler;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;







public class SyncAccounts {
  private static final String LOG_TAG = "SyncAccounts";

  private static final String MOTO_BLUR_SETTINGS_ACTIVITY = "com.motorola.blur.settings.AccountsAndServicesPreferenceActivity";
  private static final String MOTO_BLUR_PACKAGE           = "com.motorola.blur.setup";

  public final static String DEFAULT_SERVER = "https://auth.services.mozilla.com/";

  






  public static Account[] syncAccounts(final Context c) {
    return AccountManager.get(c).getAccountsByType(SyncConstants.ACCOUNTTYPE_SYNC);
  }

  






  public static boolean syncAccountsExist(Context c) {
    final boolean accountsExist = AccountManager.get(c).getAccountsByType(SyncConstants.ACCOUNTTYPE_SYNC).length > 0;
    if (accountsExist) {
      return true;
    }

    final File file = c.getFileStreamPath(Constants.ACCOUNT_PICKLE_FILENAME);
    if (!file.exists()) {
      return false;
    }

    
    
    
    final Account account = AccountPickler.unpickle(c, Constants.ACCOUNT_PICKLE_FILENAME);
    return (account != null);
  }

  





  public static class AccountsExistTask extends AsyncTask<Context, Void, Boolean> {
    @Override
    protected Boolean doInBackground(Context... params) {
      Context c = params[0];
      return syncAccountsExist(c);
    }
  }

  



  public static class SyncAccountParameters {
    public final Context context;
    public final AccountManager accountManager;


    public final String username;   
    public final String syncKey;    
    public final String password;   
    public final String serverURL;  
    public final String clusterURL; 
    public final String clientName; 
    public final String clientGuid; 

    

























    public SyncAccountParameters(Context context, AccountManager accountManager,
        String username, String syncKey, String password,
        String serverURL, String clusterURL,
        String clientName, String clientGuid) {
      if (context == null) {
        throw new IllegalArgumentException("Null context passed to SyncAccountParameters constructor.");
      }
      if (username == null) {
        throw new IllegalArgumentException("Null username passed to SyncAccountParameters constructor.");
      }
      if (syncKey == null) {
        throw new IllegalArgumentException("Null syncKey passed to SyncAccountParameters constructor.");
      }
      if (password == null) {
        throw new IllegalArgumentException("Null password passed to SyncAccountParameters constructor.");
      }
      this.context = context;
      this.accountManager = accountManager;
      this.username = username;
      this.syncKey = syncKey;
      this.password = password;
      this.serverURL = serverURL;
      this.clusterURL = clusterURL;
      this.clientName = clientName;
      this.clientGuid = clientGuid;
    }

    public SyncAccountParameters(Context context, AccountManager accountManager,
        String username, String syncKey, String password, String serverURL) {
      this(context, accountManager, username, syncKey, password, serverURL, null, null, null);
    }

    public SyncAccountParameters(final Context context, final AccountManager accountManager, final ExtendedJSONObject o) {
      this(context, accountManager,
          o.getString(Constants.JSON_KEY_ACCOUNT),
          o.getString(Constants.JSON_KEY_SYNCKEY),
          o.getString(Constants.JSON_KEY_PASSWORD),
          o.getString(Constants.JSON_KEY_SERVER),
          o.getString(Constants.JSON_KEY_CLUSTER),
          o.getString(Constants.JSON_KEY_CLIENT_NAME),
          o.getString(Constants.JSON_KEY_CLIENT_GUID));
    }

    public ExtendedJSONObject asJSON() {
      final ExtendedJSONObject o = new ExtendedJSONObject();
      o.put(Constants.JSON_KEY_ACCOUNT, username);
      o.put(Constants.JSON_KEY_PASSWORD, password);
      o.put(Constants.JSON_KEY_SERVER, serverURL);
      o.put(Constants.JSON_KEY_SYNCKEY, syncKey);
      o.put(Constants.JSON_KEY_CLUSTER, clusterURL);
      o.put(Constants.JSON_KEY_CLIENT_NAME, clientName);
      o.put(Constants.JSON_KEY_CLIENT_GUID, clientGuid);
      return o;
    }
  }

  







  public static class CreateSyncAccountTask extends AsyncTask<SyncAccountParameters, Void, Account> {
    protected final boolean syncAutomatically;

    public CreateSyncAccountTask() {
      this(true);
    }

    public CreateSyncAccountTask(final boolean syncAutomically) {
      this.syncAutomatically = syncAutomically;
    }

    @Override
    protected Account doInBackground(SyncAccountParameters... params) {
      SyncAccountParameters syncAccount = params[0];
      try {
        return createSyncAccount(syncAccount, syncAutomatically);
      } catch (Exception e) {
        Log.e(SyncConstants.GLOBAL_LOG_TAG, "Unable to create account.", e);
        return null;
      }
    }
  }

  










  public static Account createSyncAccount(SyncAccountParameters syncAccount) {
    return createSyncAccount(syncAccount, true, true);
  }

  















  public static Account createSyncAccount(SyncAccountParameters syncAccount,
      boolean syncAutomatically) {
    return createSyncAccount(syncAccount, syncAutomatically, true);
  }

  public static Account createSyncAccountPreservingExistingPreferences(SyncAccountParameters syncAccount,
      boolean syncAutomatically) {
    return createSyncAccount(syncAccount, syncAutomatically, false);
  }

  

















  protected static Account createSyncAccount(SyncAccountParameters syncAccount,
      boolean syncAutomatically, boolean clearPreferences) {
    final Context context = syncAccount.context;
    final AccountManager accountManager = (syncAccount.accountManager == null) ?
          AccountManager.get(syncAccount.context) : syncAccount.accountManager;
    final String username  = syncAccount.username;
    final String syncKey   = syncAccount.syncKey;
    final String password  = syncAccount.password;
    final String serverURL = (syncAccount.serverURL == null) ?
        DEFAULT_SERVER : syncAccount.serverURL;

    Logger.debug(LOG_TAG, "Using account manager " + accountManager);
    if (!RepoUtils.stringsEqual(syncAccount.serverURL, DEFAULT_SERVER)) {
      Logger.info(LOG_TAG, "Setting explicit server URL: " + serverURL);
    }

    final Account account = new Account(username, SyncConstants.ACCOUNTTYPE_SYNC);
    final Bundle userbundle = new Bundle();

    
    userbundle.putString(Constants.OPTION_SYNCKEY, syncKey);
    userbundle.putString(Constants.OPTION_SERVER, serverURL);
    Logger.debug(LOG_TAG, "Adding account for " + SyncConstants.ACCOUNTTYPE_SYNC);
    boolean result = false;
    try {
      result = accountManager.addAccountExplicitly(account, password, userbundle);
    } catch (SecurityException e) {
      
      final String message = e.getMessage();
      if (message != null && (message.indexOf("is different than the authenticator's uid") > 0)) {
        Log.wtf(SyncConstants.GLOBAL_LOG_TAG,
                "Unable to create account. " +
                "If you have more than one version of " +
                "Firefox/Beta/Aurora/Nightly/Fennec installed, that's why.",
                e);
      } else {
        Log.e(SyncConstants.GLOBAL_LOG_TAG, "Unable to create account.", e);
      }
    }

    if (!result) {
      Logger.error(LOG_TAG, "Failed to add account " + account + "!");
      return null;
    }
    Logger.debug(LOG_TAG, "Account " + account + " added successfully.");

    setSyncAutomatically(account, syncAutomatically);
    setIsSyncable(account, syncAutomatically);
    Logger.debug(LOG_TAG, "Set account to sync automatically? " + syncAutomatically + ".");

    try {
      final String product = GlobalConstants.BROWSER_INTENT_PACKAGE;
      final String profile = Constants.DEFAULT_PROFILE;
      final long version = SyncConfiguration.CURRENT_PREFS_VERSION;

      final SharedPreferences.Editor editor = Utils.getSharedPreferences(context, product, username, serverURL, profile, version).edit();
      if (clearPreferences) {
        final String prefsPath = Utils.getPrefsPath(product, username, serverURL, profile, version);
        Logger.info(LOG_TAG, "Clearing preferences path " + prefsPath + " for this account.");
        editor.clear();
      }

      if (syncAccount.clusterURL != null) {
        editor.putString(SyncConfiguration.PREF_CLUSTER_URL, syncAccount.clusterURL);
      }

      if (syncAccount.clientName != null && syncAccount.clientGuid != null) {
        Logger.debug(LOG_TAG, "Setting client name to " + syncAccount.clientName + " and client GUID to " + syncAccount.clientGuid + ".");
        editor.putString(SyncConfiguration.PREF_CLIENT_NAME, syncAccount.clientName);
        editor.putString(SyncConfiguration.PREF_ACCOUNT_GUID, syncAccount.clientGuid);
      } else {
        Logger.debug(LOG_TAG, "Client name and guid not both non-null, so not setting client data.");
      }

      editor.commit();
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Could not clear prefs path!", e);
    }
    return account;
  }

  public static void setIsSyncable(Account account, boolean isSyncable) {
    String authority = BrowserContract.AUTHORITY;
    ContentResolver.setIsSyncable(account, authority, isSyncable ? 1 : 0);
  }

  public static void setSyncAutomatically(Account account, boolean syncAutomatically) {
    if (syncAutomatically) {
      ContentResolver.setMasterSyncAutomatically(true);
    }

    String authority = BrowserContract.AUTHORITY;
    Logger.debug(LOG_TAG, "Setting authority " + authority + " to " +
                          (syncAutomatically ? "" : "not ") + "sync automatically.");
    ContentResolver.setSyncAutomatically(account, authority, syncAutomatically);
  }

  public static void backgroundSetSyncAutomatically(final Account account, final boolean syncAutomatically) {
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        setSyncAutomatically(account, syncAutomatically);
      }
    });
  }
  















  protected static Intent openVendorSyncSettings(Context context, final String vendorPackage, final String vendorClass) {
    try {
      final int contextFlags = Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY;
      Context foreignContext = context.createPackageContext(vendorPackage, contextFlags);
      Class<?> klass = foreignContext.getClassLoader().loadClass(vendorClass);

      final Intent intent = new Intent(foreignContext, klass);
      context.startActivity(intent);
      Logger.info(LOG_TAG, "Vendor package " + vendorPackage + " and class " +
          vendorClass + " found, and activity launched.");
      return intent;
    } catch (NameNotFoundException e) {
      Logger.debug(LOG_TAG, "Vendor package " + vendorPackage + " not found. Skipping.");
    } catch (ClassNotFoundException e) {
      Logger.debug(LOG_TAG, "Vendor package " + vendorPackage + " found but class " +
          vendorClass + " not found. Skipping.", e);
    } catch (ActivityNotFoundException e) {
      
      Logger.warn(LOG_TAG, "Vendor package " + vendorPackage + " and class " +
          vendorClass + " found, but activity not launched. Skipping.", e);
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Caught exception launching activity from vendor package " + vendorPackage +
          " and class " + vendorClass + ". Ignoring.", e);
    }
    return null;
  }

  






  public static Intent openSyncSettings(Context context) {
    
    
    
    Intent intent = openVendorSyncSettings(context, MOTO_BLUR_PACKAGE, MOTO_BLUR_SETTINGS_ACTIVITY);
    if (intent != null) {
      return intent;
    }

    
    intent = new Intent(Settings.ACTION_SYNC_SETTINGS);
    
    context.startActivity(intent); 
    return intent;
  }

  














  public static SyncAccountParameters blockingFromAndroidAccountV0(final Context context, final AccountManager accountManager, final Account account)
      throws CredentialException {
    String username;
    try {
      username = Utils.usernameFromAccount(account.name);
    } catch (NoSuchAlgorithmException e) {
      throw new CredentialException.MissingCredentialException("username");
    } catch (UnsupportedEncodingException e) {
      throw new CredentialException.MissingCredentialException("username");
    }

    





    String password;
    String syncKey;
    String serverURL;
    try {
      password = accountManager.getPassword(account);
      syncKey = accountManager.getUserData(account, Constants.OPTION_SYNCKEY);
      serverURL = accountManager.getUserData(account, Constants.OPTION_SERVER);
    } catch (SecurityException e) {
      Logger.warn(LOG_TAG, "Got security exception fetching Sync account parameters; throwing.");
      throw new CredentialException.MissingAllCredentialsException(e);
    }

    if (password  == null &&
        username  == null &&
        syncKey   == null &&
        serverURL == null) {
      throw new CredentialException.MissingAllCredentialsException();
    }

    if (password == null) {
      throw new CredentialException.MissingCredentialException("password");
    }

    if (syncKey == null) {
      throw new CredentialException.MissingCredentialException("syncKey");
    }

    if (serverURL == null) {
      throw new CredentialException.MissingCredentialException("serverURL");
    }

    try {
      
      
      return new SyncAccountParameters(context, accountManager, username, syncKey, password, serverURL);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception fetching Sync account parameters; throwing.");
      throw new CredentialException.MissingAllCredentialsException(e);
    }
  }

  




























  public static Intent makeSyncAccountDeletedIntent(final Context context, final AccountManager accountManager, final Account account) {
    final Intent intent = new Intent(SyncConstants.SYNC_ACCOUNT_DELETED_ACTION);

    intent.putExtra(Constants.JSON_KEY_VERSION, Long.valueOf(SyncConstants.SYNC_ACCOUNT_DELETED_INTENT_VERSION));
    intent.putExtra(Constants.JSON_KEY_TIMESTAMP, Long.valueOf(System.currentTimeMillis()));
    intent.putExtra(Constants.JSON_KEY_ACCOUNT, account.name);

    SyncAccountParameters accountParameters = null;
    try {
      accountParameters = SyncAccounts.blockingFromAndroidAccountV0(context, accountManager, account);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception fetching account parameters.", e);
    }

    if (accountParameters != null) {
      ExtendedJSONObject json = accountParameters.asJSON();
      json.put(Constants.JSON_KEY_SYNCKEY, ""); 
      intent.putExtra(Constants.JSON_KEY_PAYLOAD, json.toJSONString());
    }

    return intent;
  }

  






















  public static SharedPreferences blockingPrefsFromAndroidAccountV0(final Context context, final AccountManager accountManager, final Account account,
      final String product, final String profile, final long version)
          throws CredentialException, NoSuchAlgorithmException, UnsupportedEncodingException {
    SyncAccountParameters params = SyncAccounts.blockingFromAndroidAccountV0(context, accountManager, account);
    String prefsPath = Utils.getPrefsPath(product, params.username, params.serverURL, profile, version);

    return context.getSharedPreferences(prefsPath, Utils.SHARED_PREFERENCES_MODE);
  }

  


















  public static SharedPreferences blockingPrefsFromDefaultProfileV0(final Context context, final AccountManager accountManager, final Account account)
      throws CredentialException, NoSuchAlgorithmException, UnsupportedEncodingException {
    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE;
    final String profile = Constants.DEFAULT_PROFILE;
    final long version = SyncConfiguration.CURRENT_PREFS_VERSION;

    return blockingPrefsFromAndroidAccountV0(context, accountManager, account, product, profile, version);
  }
}
