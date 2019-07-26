



package org.mozilla.gecko.sync.syncadapter;

import java.io.IOException;
import java.net.URI;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.atomic.AtomicBoolean;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.AlreadySyncingException;
import org.mozilla.gecko.sync.CredentialException;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConfigurationException;
import org.mozilla.gecko.sync.SyncException;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.config.AccountPickler;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;
import org.mozilla.gecko.sync.net.ConnectionMonitorThread;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SyncResult;
import android.database.sqlite.SQLiteConstraintException;
import android.database.sqlite.SQLiteException;
import android.os.Bundle;

public class SyncAdapter extends AbstractThreadedSyncAdapter implements GlobalSessionCallback, ClientsDataDelegate {
  private static final String  LOG_TAG = "SyncAdapter";

  private static final int     BACKOFF_PAD_SECONDS = 5;
  public  static final int     MULTI_DEVICE_INTERVAL_MILLISECONDS = 5 * 60 * 1000;         
  public  static final int     SINGLE_DEVICE_INTERVAL_MILLISECONDS = 24 * 60 * 60 * 1000;  

  private final Context        mContext;

  protected long syncStartTimestamp;

  public SyncAdapter(Context context, boolean autoInitialize) {
    super(context, autoInitialize);
    mContext = context;
  }

  


  public synchronized long getEarliestNextSync() {
    return accountSharedPreferences.getLong(SyncConfiguration.PREF_EARLIEST_NEXT_SYNC, 0);
  }

  public synchronized void setEarliestNextSync(long next) {
    Editor edit = accountSharedPreferences.edit();
    edit.putLong(SyncConfiguration.PREF_EARLIEST_NEXT_SYNC, next);
    edit.commit();
  }

  public synchronized void extendEarliestNextSync(long next) {
    if (accountSharedPreferences.getLong(SyncConfiguration.PREF_EARLIEST_NEXT_SYNC, 0) >= next) {
      return;
    }
    Editor edit = accountSharedPreferences.edit();
    edit.putLong(SyncConfiguration.PREF_EARLIEST_NEXT_SYNC, next);
    edit.commit();
  }

  








  protected void processException(final GlobalSession globalSession, final Exception e) {
    try {
      if (e instanceof SQLiteConstraintException) {
        Logger.error(LOG_TAG, "Constraint exception. Aborting sync.", e);
        syncResult.stats.numParseExceptions++;       
        return;
      }
      if (e instanceof SQLiteException) {
        Logger.error(LOG_TAG, "Couldn't open database (locked?). Aborting sync.", e);
        syncResult.stats.numIoExceptions++;
        return;
      }
      if (e instanceof OperationCanceledException) {
        Logger.error(LOG_TAG, "Operation canceled. Aborting sync.", e);
        return;
      }
      if (e instanceof AuthenticatorException) {
        syncResult.stats.numParseExceptions++;
        Logger.error(LOG_TAG, "AuthenticatorException. Aborting sync.", e);
        return;
      }
      if (e instanceof IOException) {
        syncResult.stats.numIoExceptions++;
        Logger.error(LOG_TAG, "IOException. Aborting sync.", e);
        e.printStackTrace();
        return;
      }

      
      if (e instanceof SyncException) {
        ((SyncException) e).updateStats(globalSession, syncResult);
      } else {
        
        syncResult.stats.numIoExceptions++;
      }

      if (e instanceof CredentialException.MissingAllCredentialsException) {
        
        
        
        
        if (localAccount == null) {
          
          Logger.error(LOG_TAG, "No credentials attached to account. Aborting sync.");
          return;
        }

        Logger.error(LOG_TAG, "No credentials attached to account " + localAccount.name + ". Aborting sync.");
        try {
          SyncAccounts.setSyncAutomatically(localAccount, false);
        } catch (Exception ex) {
          Logger.error(LOG_TAG, "Unable to disable account " + localAccount.name + ".", ex);
        }
        return;
      }

      if (e instanceof CredentialException.MissingCredentialException) {
        Logger.error(LOG_TAG, "Credentials attached to account, but missing " +
            ((CredentialException.MissingCredentialException) e).missingCredential + ". Aborting sync.");
        return;
      }

      if (e instanceof CredentialException) {
        Logger.error(LOG_TAG, "Credentials attached to account were bad.");
        return;
      }

      
      
      
      if (e instanceof SecurityException) {
        Logger.error(LOG_TAG, "SecurityException, multiple Fennecs. Disabling this instance.", e);
        SyncAccounts.backgroundSetSyncAutomatically(localAccount, false);
        return;
      }
      
      Logger.error(LOG_TAG, "Unknown exception. Aborting sync.", e);
    } finally {
      notifyMonitor();
    }
  }

  @Override
  public void onSyncCanceled() {
    super.onSyncCanceled();
    
    
    
    
  }

  public Object syncMonitor = new Object();
  private SyncResult syncResult;

  protected Account localAccount;
  protected boolean thisSyncIsForced = false;
  protected SharedPreferences accountSharedPreferences;

  



  public long delayMilliseconds() {
    long earliestNextSync = getEarliestNextSync();
    if (earliestNextSync <= 0) {
      return 0;
    }
    long now = System.currentTimeMillis();
    return Math.max(0, earliestNextSync - now);
  }

  @Override
  public boolean shouldBackOff() {
    if (thisSyncIsForced) {
      




      return false;
    }

    if (wantNodeAssignment()) {
      






      return false;
    }

    return delayMilliseconds() > 0;
  }

  





  @Override
  public void requestBackoff(final long backoff) {
    if (backoff > 0) {
      
      final long fuzzedBackoff = backoff + Math.round((double) backoff * 0.25d * Math.random());
      this.extendEarliestNextSync(System.currentTimeMillis() + fuzzedBackoff);
    }
  }

  










  public static void requestImmediateSync(final Account account, final String[] stageNames) {
    requestImmediateSync(account, stageNames, null);
  }

  












  public static void requestImmediateSync(final Account account, final String[] stageNames, Bundle moreExtras) {
    if (account == null) {
      Logger.warn(LOG_TAG, "Not requesting immediate sync because Android Account is null.");
      return;
    }

    final Bundle extras = new Bundle();
    Utils.putStageNamesToSync(extras, stageNames, null);
    extras.putBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, true);

    if (moreExtras != null) {
      extras.putAll(moreExtras);
    }

    ContentResolver.requestSync(account, BrowserContract.AUTHORITY, extras);
  }

  @Override
  public void onPerformSync(final Account account,
                            final Bundle extras,
                            final String authority,
                            final ContentProviderClient provider,
                            final SyncResult syncResult) {
    syncStartTimestamp = System.currentTimeMillis();

    Logger.setThreadLogTag(SyncConstants.GLOBAL_LOG_TAG);
    Logger.resetLogging();
    Utils.reseedSharedRandom(); 

    
    this.syncResult   = syncResult;
    this.localAccount = account;

    SyncAccountParameters params;
    try {
      params = SyncAccounts.blockingFromAndroidAccountV0(mContext, AccountManager.get(mContext), this.localAccount);
    } catch (Exception e) {
      
      processException(null, e);
      return;
    }

    
    final String username  = params.username; 
    final String password  = params.password;
    final String serverURL = params.serverURL;
    final String syncKey   = params.syncKey;

    final AtomicBoolean setNextSync = new AtomicBoolean(true);
    final SyncAdapter self = this;
    final Runnable runnable = new Runnable() {
      @Override
      public void run() {
        Logger.trace(LOG_TAG, "AccountManagerCallback invoked.");
        
        try {
          Logger.info(LOG_TAG, "Syncing account named " + account.name +
              " for authority " + authority + ".");

          
          Logger.debug(LOG_TAG, "Username: " + username);
          Logger.debug(LOG_TAG, "Server:   " + serverURL);
          if (Logger.LOG_PERSONAL_INFORMATION) {
            Logger.debug(LOG_TAG, "Password: " + password);
            Logger.debug(LOG_TAG, "Sync key: " + syncKey);
          } else {
            Logger.debug(LOG_TAG, "Password? " + (password != null));
            Logger.debug(LOG_TAG, "Sync key? " + (syncKey != null));
          }

          
          
          final String product = GlobalConstants.BROWSER_INTENT_PACKAGE;
          final String profile = Constants.DEFAULT_PROFILE;
          final long version = SyncConfiguration.CURRENT_PREFS_VERSION;
          self.accountSharedPreferences = Utils.getSharedPreferences(mContext, product, username, serverURL, profile, version);

          Logger.info(LOG_TAG,
              "Client is named '" + getClientName() + "'" +
              ", has client guid " + getAccountGUID() +
              ", and has " + getClientsCount() + " clients.");

          thisSyncIsForced = (extras != null) && (extras.getBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, false));
          long delay = delayMilliseconds();
          if (delay > 0) {
            if (thisSyncIsForced) {
              Logger.info(LOG_TAG, "Forced sync: overruling remaining backoff of " + delay + "ms.");
            } else {
              Logger.info(LOG_TAG, "Not syncing: must wait another " + delay + "ms.");
              long remainingSeconds = delay / 1000;
              syncResult.delayUntil = remainingSeconds + BACKOFF_PAD_SECONDS;
              setNextSync.set(false);
              self.notifyMonitor();
              return;
            }
          }

          final String prefsPath = Utils.getPrefsPath(product, username, serverURL, profile, version);
          self.performSync(account, extras, authority, provider, syncResult,
              username, password, prefsPath, serverURL, syncKey);
        } catch (Exception e) {
          self.processException(null, e);
          return;
        }
      }
    };

    synchronized (syncMonitor) {
      
      
      
      new Thread(runnable).start();

      
      ConnectionMonitorThread stale = new ConnectionMonitorThread();
      stale.start();

      Logger.trace(LOG_TAG, "Waiting on sync monitor.");
      try {
        syncMonitor.wait();

        if (setNextSync.get()) {
          long interval = getSyncInterval();
          long next = System.currentTimeMillis() + interval;

          if (thisSyncIsForced) {
            Logger.info(LOG_TAG, "Setting minimum next sync time to " + next + " (" + interval + "ms from now).");
            setEarliestNextSync(next);
          } else {
            Logger.info(LOG_TAG, "Extending minimum next sync time to " + next + " (" + interval + "ms from now).");
            extendEarliestNextSync(next);
          }
        }
        Logger.info(LOG_TAG, "Sync took " + Utils.formatDuration(syncStartTimestamp, System.currentTimeMillis()) + ".");
      } catch (InterruptedException e) {
        Logger.warn(LOG_TAG, "Waiting on sync monitor interrupted.", e);
      } finally {
        
        stale.shutdown();
      }
    }
  }

  public int getSyncInterval() {
    
    if (this.localAccount == null) {
      return SINGLE_DEVICE_INTERVAL_MILLISECONDS;
    }

    int clientsCount = this.getClientsCount();
    if (clientsCount <= 1) {
      return SINGLE_DEVICE_INTERVAL_MILLISECONDS;
    }

    return MULTI_DEVICE_INTERVAL_MILLISECONDS;
  }

  










  protected void performSync(final Account account,
                             final Bundle extras,
                             final String authority,
                             final ContentProviderClient provider,
                             final SyncResult syncResult,
                             final String username,
                             final String password,
                             final String prefsPath,
                             final String serverURL,
                             final String syncKey)
                                 throws NoSuchAlgorithmException,
                                        SyncConfigurationException,
                                        IllegalArgumentException,
                                        AlreadySyncingException,
                                        IOException, ParseException,
                                        NonObjectJSONException, CryptoException {
    Logger.trace(LOG_TAG, "Performing sync.");

    



    try {
      
      final SyncAccountParameters params = new SyncAccountParameters(mContext, null,
        account.name, 
        syncKey,
        password,
        serverURL,
        null, 
        getClientName(),
        getAccountGUID());

      
      
      ThreadPool.run(new Runnable() {
        @Override
        public void run() {
          final boolean syncAutomatically = ContentResolver.getSyncAutomatically(account, authority);
          try {
            AccountPickler.pickle(mContext, Constants.ACCOUNT_PICKLE_FILENAME, params, syncAutomatically);
          } catch (Exception e) {
            
            Logger.warn(LOG_TAG, "Got exception pickling current account details; ignoring.", e);
          }
        }
      });
    } catch (IllegalArgumentException e) {
      
    }

    
    final KeyBundle keyBundle = new KeyBundle(username, syncKey);
    GlobalSession globalSession = new GlobalSession(SyncConfiguration.DEFAULT_USER_API,
                                                    serverURL, username, password, prefsPath,
                                                    keyBundle, this, this.mContext, extras, this);

    globalSession.start();
  }

  private void notifyMonitor() {
    synchronized (syncMonitor) {
      Logger.trace(LOG_TAG, "Notifying sync monitor.");
      syncMonitor.notifyAll();
    }
  }

  
  @Override
  public void handleError(GlobalSession globalSession, Exception ex) {
    Logger.info(LOG_TAG, "GlobalSession indicated error.");
    this.processException(globalSession, ex);
  }

  @Override
  public void handleAborted(GlobalSession globalSession, String reason) {
    Logger.warn(LOG_TAG, "Sync aborted: " + reason);
    notifyMonitor();
  }

  @Override
  public void handleSuccess(GlobalSession globalSession) {
    Logger.info(LOG_TAG, "GlobalSession indicated success.");
    Logger.debug(LOG_TAG, "Prefs target: " + globalSession.config.prefsPath);
    globalSession.config.persistToPrefs();
    notifyMonitor();
  }

  @Override
  public void handleStageCompleted(Stage currentState,
                                   GlobalSession globalSession) {
    Logger.trace(LOG_TAG, "Stage completed: " + currentState);
  }

  @Override
  public synchronized String getAccountGUID() {
    String accountGUID = accountSharedPreferences.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
    if (accountGUID == null) {
      Logger.debug(LOG_TAG, "Account GUID was null. Creating a new one.");
      accountGUID = Utils.generateGuid();
      accountSharedPreferences.edit().putString(SyncConfiguration.PREF_ACCOUNT_GUID, accountGUID).commit();
    }
    return accountGUID;
  }

  @Override
  public synchronized String getClientName() {
    String clientName = accountSharedPreferences.getString(SyncConfiguration.PREF_CLIENT_NAME, null);
    if (clientName == null) {
      clientName = GlobalConstants.MOZ_APP_DISPLAYNAME + " on " + android.os.Build.MODEL;
      accountSharedPreferences.edit().putString(SyncConfiguration.PREF_CLIENT_NAME, clientName).commit();
    }
    return clientName;
  }

  @Override
  public synchronized void setClientsCount(int clientsCount) {
    accountSharedPreferences.edit().putLong(SyncConfiguration.PREF_NUM_CLIENTS, (long) clientsCount).commit();
  }

  @Override
  public boolean isLocalGUID(String guid) {
    return getAccountGUID().equals(guid);
  }

  @Override
  public synchronized int getClientsCount() {
    return (int) accountSharedPreferences.getLong(SyncConfiguration.PREF_NUM_CLIENTS, 0);
  }

  public synchronized boolean getClusterURLIsStale() {
    return accountSharedPreferences.getBoolean(SyncConfiguration.PREF_CLUSTER_URL_IS_STALE, false);
  }

  public synchronized void setClusterURLIsStale(boolean clusterURLIsStale) {
    Editor edit = accountSharedPreferences.edit();
    edit.putBoolean(SyncConfiguration.PREF_CLUSTER_URL_IS_STALE, clusterURLIsStale);
    edit.commit();
  }

  @Override
  public boolean wantNodeAssignment() {
    return getClusterURLIsStale();
  }

  @Override
  public void informNodeAuthenticationFailed(GlobalSession session, URI failedClusterURL) {
    
    
    setClusterURLIsStale(false);
  }

  @Override
  public void informNodeAssigned(GlobalSession session, URI oldClusterURL, URI newClusterURL) {
    setClusterURLIsStale(false);
  }

  @Override
  public void informUnauthorizedResponse(GlobalSession session, URI oldClusterURL) {
    setClusterURLIsStale(true);
  }

  @Override
  public void informUpgradeRequiredResponse(final GlobalSession session) {
    final AccountManager manager = AccountManager.get(mContext);
    final Account toDisable      = localAccount;
    if (toDisable == null || manager == null) {
      Logger.warn(LOG_TAG, "Attempting to disable account, but null found.");
      return;
    }
    
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        manager.setUserData(toDisable, Constants.DATA_ENABLE_ON_UPGRADE, "1");
        SyncAccounts.setSyncAutomatically(toDisable, false);
      }
    });
  }
}
