



package org.mozilla.gecko.fxa.receivers;

import java.util.concurrent.Executor;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.oauth.FxAccountAbstractClient;
import org.mozilla.gecko.background.fxa.oauth.FxAccountAbstractClientException.FxAccountAbstractClientRemoteException;
import org.mozilla.gecko.background.fxa.oauth.FxAccountOAuthClient10;
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

    
    
    
    final String oauthServerURI = intent.getStringExtra(FxAccountConstants.ACCOUNT_OAUTH_SERVICE_ENDPOINT_KEY);
    final String[] tokens = intent.getStringArrayExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_ACCOUNT_AUTH_TOKENS);
    if (oauthServerURI != null && tokens != null) {
      final Executor directExecutor = new Executor() {
        @Override
        public void execute(Runnable runnable) {
          runnable.run();
        }
      };

      final FxAccountOAuthClient10 oauthClient = new FxAccountOAuthClient10(oauthServerURI, directExecutor);

      for (String token : tokens) {
        if (token == null) {
          Logger.error(LOG_TAG, "Cached OAuth token is null; should never happen.  Ignoring.");
          continue;
        }
        try {
          oauthClient.deleteToken(token, new FxAccountAbstractClient.RequestDelegate<Void>() {
            @Override
            public void handleSuccess(Void result) {
              Logger.info(LOG_TAG, "Successfully deleted cached OAuth token.");
            }

            @Override
            public void handleError(Exception e) {
              Logger.error(LOG_TAG, "Failed to delete cached OAuth token; ignoring.", e);
            }

            @Override
            public void handleFailure(FxAccountAbstractClientRemoteException e) {
              Logger.error(LOG_TAG, "Exception during cached OAuth token deletion; ignoring.", e);
            }
          });
        } catch (Exception e) {
          Logger.error(LOG_TAG, "Exception during cached OAuth token deletion; ignoring.", e);
        }
      }
    } else {
      Logger.error(LOG_TAG, "Cached OAuth server URI is null or cached OAuth tokens are null; ignoring.");
    }
  }

  public static void deletePickle(final Context context) {
    try {
      AccountPickler.deletePickle(context, FxAccountConstants.ACCOUNT_PICKLE_FILENAME);
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception deleting saved pickle file; ignoring.", e);
    }
  }
}
