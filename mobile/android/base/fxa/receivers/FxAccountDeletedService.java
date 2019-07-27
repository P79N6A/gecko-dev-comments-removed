



package org.mozilla.gecko.fxa.receivers;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.sync.FxAccountNotificationManager;
import org.mozilla.gecko.fxa.sync.FxAccountSyncAdapter;
import org.mozilla.gecko.sync.config.AccountPickler;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserHistoryDataExtender;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabase;
import org.mozilla.gecko.sync.repositories.android.FennecTabsRepository;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;









public class FxAccountDeletedService extends IntentService {
  public static final String LOG_TAG = FxAccountDeletedService.class.getSimpleName();

  public FxAccountDeletedService() {
    super(LOG_TAG);
  }

  @Override
  protected void onHandleIntent(final Intent intent) {
    
    if (intent == null) {
      Logger.debug(LOG_TAG, "Short-circuiting on null intent.");
      return;
    }

    final Context context = this;

    long intentVersion = intent.getLongExtra(
        FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION_KEY, 0);
    long expectedVersion = FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION;
    if (intentVersion != expectedVersion) {
      Logger.warn(LOG_TAG, "Intent malformed: version " + intentVersion + " given but " +
          "version " + expectedVersion + "expected. Not cleaning up after deleted Account.");
      return;
    }

    
    final String accountName = intent.getStringExtra(
        FxAccountConstants.ACCOUNT_DELETED_INTENT_ACCOUNT_KEY);
    if (accountName == null) {
      Logger.warn(LOG_TAG, "Intent malformed: no account name given. Not cleaning up after " +
          "deleted Account.");
      return;
    }

    Logger.info(LOG_TAG, "Firefox account named " + accountName + " being removed; " +
        "deleting saved pickle file '" + FxAccountConstants.ACCOUNT_PICKLE_FILENAME + "'.");
    deletePickle(context);

    
    Logger.info(LOG_TAG, "Deleting the entire Fennec clients database and non-local tabs");
    FennecTabsRepository.deleteNonLocalClientsAndTabs(context);


    
    try {
      Logger.info(LOG_TAG, "Deleting the Firefox Sync clients database.");
      ClientsDatabase db = null;
      try {
        db = new ClientsDatabase(context);
        db.wipeClientsTable();
        db.wipeCommandsTable();
      } finally {
        if (db != null) {
          db.close();
        }
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception deleting the Firefox Sync clients database; ignoring.", e);
    }

    
    try {
      Logger.info(LOG_TAG, "Deleting the Firefox Sync extended history database.");
      AndroidBrowserHistoryDataExtender historyDataExtender = null;
      try {
        historyDataExtender = new AndroidBrowserHistoryDataExtender(context);
        historyDataExtender.wipe();
      } finally {
        if (historyDataExtender != null) {
          historyDataExtender.close();
        }
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception deleting the Firefox Sync extended history database; ignoring.", e);
    }

    
    new FxAccountNotificationManager(FxAccountSyncAdapter.NOTIFICATION_ID).clear(context);

    
    
    
  }

  public static void deletePickle(final Context context) {
    try {
      AccountPickler.deletePickle(context, FxAccountConstants.ACCOUNT_PICKLE_FILENAME);
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception deleting saved pickle file; ignoring.", e);
    }
  }
}
