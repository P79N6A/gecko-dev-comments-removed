



package org.mozilla.gecko.sync.setup;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;
import org.mozilla.gecko.sync.syncadapter.SyncAdapter;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;







public class SyncAccounts {

  public final static String DEFAULT_SERVER = "https://auth.services.mozilla.com/";
  private static final String LOG_TAG = "SyncAccounts";
  private static final String GLOBAL_LOG_TAG = "FxSync";

  




  public static boolean syncAccountsExist(Context c) {
    return AccountManager.get(c).getAccountsByType(Constants.ACCOUNTTYPE_SYNC).length > 0;
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
  }

  







  public static class CreateSyncAccountTask extends AsyncTask<SyncAccountParameters, Void, Account> {
    @Override
    protected Account doInBackground(SyncAccountParameters... params) {
      SyncAccountParameters syncAccount = params[0];
      try {
        return createSyncAccount(syncAccount);
      } catch (Exception e) {
        Log.e(GLOBAL_LOG_TAG, "Unable to create account.", e);
        return null;
      }
    }
  }

  









  public static Account createSyncAccount(SyncAccountParameters syncAccount) {
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

    final Account account = new Account(username, Constants.ACCOUNTTYPE_SYNC);
    final Bundle userbundle = new Bundle();

    
    userbundle.putString(Constants.OPTION_SYNCKEY, syncKey);
    userbundle.putString(Constants.OPTION_SERVER, serverURL);
    Logger.debug(LOG_TAG, "Adding account for " + Constants.ACCOUNTTYPE_SYNC);
    boolean result = false;
    try {
      result = accountManager.addAccountExplicitly(account, password, userbundle);
    } catch (SecurityException e) {
      
      final String message = e.getMessage();
      if (message != null && (message.indexOf("is different than the authenticator's uid") > 0)) {
        Log.wtf(GLOBAL_LOG_TAG,
                "Unable to create account. " +
                "If you have more than one version of " +
                "Firefox/Beta/Aurora/Nightly/Fennec installed, that's why.",
                e);
      } else {
        Log.e(GLOBAL_LOG_TAG, "Unable to create account.", e);
      }
    }

    if (!result) {
      Logger.error(LOG_TAG, "Failed to add account " + account + "!");
      return null;
    }
    Logger.debug(LOG_TAG, "Account " + account + " added successfully.");

    setSyncAutomatically(account);
    setClientRecord(context, accountManager, account, syncAccount.clientName, syncAccount.clientGuid);

    
    
    Logger.debug(LOG_TAG, "Finished setting syncables.");

    
    Logger.info(LOG_TAG, "Clearing preferences for this account.");
    try {
      SharedPreferences.Editor editor = Utils.getSharedPreferences(context, username, serverURL).edit().clear();
      if (syncAccount.clusterURL != null) {
        editor.putString(SyncConfiguration.PREF_CLUSTER_URL, syncAccount.clusterURL);
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

  public static void setSyncAutomatically(Account account) {
    setSyncAutomatically(account, true);
    setIsSyncable(account, true);
  }

  protected static void setClientRecord(Context context, AccountManager accountManager, Account account,
      String clientName, String clientGuid) {
    if (clientName != null && clientGuid != null) {
      Logger.debug(LOG_TAG, "Setting client name to " + clientName + " and client GUID to " + clientGuid + ".");
      SyncAdapter.setAccountGUID(accountManager, account, clientGuid);
      SyncAdapter.setClientName(accountManager, account, clientName);
      return;
    }
    Logger.debug(LOG_TAG, "Client name and guid not both non-null, so not setting client data.");
  }
}
