



package org.mozilla.gecko.sync.receivers;

import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.config.AccountPickler;
import org.mozilla.gecko.sync.config.ClientRecordTerminator;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.AccountManager;
import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

public class SyncAccountDeletedService extends IntentService {
  public static final String LOG_TAG = "SyncAccountDeletedService";

  public SyncAccountDeletedService() {
    super(LOG_TAG);
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    final Context context = this;

    long intentVersion = intent.getLongExtra(Constants.JSON_KEY_VERSION, 0);
    long expectedVersion = SyncConstants.SYNC_ACCOUNT_DELETED_INTENT_VERSION;
    if (intentVersion != expectedVersion) {
      Logger.warn(LOG_TAG, "Intent malformed: version " + intentVersion + " given but version " + expectedVersion + "expected. " +
          "Not cleaning up after deleted Account.");
      return;
    }

    String accountName = intent.getStringExtra(Constants.JSON_KEY_ACCOUNT); 
    if (accountName == null) {
      Logger.warn(LOG_TAG, "Intent malformed: no account name given. Not cleaning up after deleted Account.");
      return;
    }

    
    Logger.info(LOG_TAG, "Sync account named " + accountName + " being removed; " +
        "deleting saved pickle file '" + Constants.ACCOUNT_PICKLE_FILENAME + "'.");
    deletePickle(context);

    SyncAccountParameters params;
    try {
      String payload = intent.getStringExtra(Constants.JSON_KEY_PAYLOAD);
      if (payload == null) {
        Logger.warn(LOG_TAG, "Intent malformed: no payload given. Not deleting client record.");
        return;
      }
      ExtendedJSONObject o = ExtendedJSONObject.parseJSONObject(payload);
      params = new SyncAccountParameters(context, AccountManager.get(context), o);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception fetching account parameters from intent data; not deleting client record.");
      return;
    }

    
    Logger.info(LOG_TAG, "Account named " + accountName + " being removed; " +
        "deleting client record from server.");
    deleteClientRecord(context, accountName, params.password, params.serverURL);
  }

  public static void deletePickle(final Context context) {
    try {
      AccountPickler.deletePickle(context, Constants.ACCOUNT_PICKLE_FILENAME);
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception deleting saved pickle file; ignoring.", e);
    }
  }

  public static void deleteClientRecord(final Context context, final String accountName,
      final String password, final String serverURL) {
    String encodedUsername;
    try {
      encodedUsername = Utils.usernameFromAccount(accountName);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception deleting client record from server; ignoring.", e);
      return;
    }

    if (accountName == null || encodedUsername == null || password == null || serverURL == null) {
      Logger.warn(LOG_TAG, "Account parameters were null; not deleting client record from server.");
      return;
    }

    
    
    
    
    
    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE;
    final String profile = Constants.DEFAULT_PROFILE;
    final long version = SyncConfiguration.CURRENT_PREFS_VERSION;

    SharedPreferences prefs;
    try {
      prefs = Utils.getSharedPreferences(context, product, encodedUsername, serverURL, profile, version);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception fetching preferences; not deleting client record from server.", e);
      return;
    }

    final String clientGuid = prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
    final String clusterURL = prefs.getString(SyncConfiguration.PREF_CLUSTER_URL, null);

    
    prefs.edit().clear().commit();

    if (clientGuid == null) {
      Logger.warn(LOG_TAG, "Client GUID was null; not deleting client record from server.");
      return;
    }

    if (clusterURL == null) {
      Logger.warn(LOG_TAG, "Cluster URL was null; not deleting client record from server.");
      return;
    }

    try {
      ClientRecordTerminator.deleteClientRecord(encodedUsername, password, clusterURL, clientGuid);
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception deleting client record from server; ignoring.", e);
    }
  }
}
