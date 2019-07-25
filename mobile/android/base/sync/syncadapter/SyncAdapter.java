





































package org.mozilla.gecko.sync.syncadapter;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.TimeUnit;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.AlreadySyncingException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConfigurationException;
import org.mozilla.gecko.sync.SyncException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.Cryptographer;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SyncResult;
import android.database.sqlite.SQLiteConstraintException;
import android.database.sqlite.SQLiteException;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

public class SyncAdapter extends AbstractThreadedSyncAdapter implements GlobalSessionCallback {
  private static final String  LOG_TAG = "SyncAdapter";

  private static final String  PREFS_EARLIEST_NEXT_SYNC = "earliestnextsync";
  private static final String  PREFS_INVALIDATE_AUTH_TOKEN = "invalidateauthtoken";

  private static final int     SHARED_PREFERENCES_MODE = 0;
  private static final int     BACKOFF_PAD_SECONDS = 5;

  private final AccountManager mAccountManager;
  private final Context        mContext;

  public SyncAdapter(Context context, boolean autoInitialize) {
    super(context, autoInitialize);
    mContext = context;
    Log.d(LOG_TAG, "AccountManager.get(" + mContext + ")");
    mAccountManager = AccountManager.get(context);
  }

  


  public synchronized long getEarliestNextSync() {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    return sharedPreferences.getLong(PREFS_EARLIEST_NEXT_SYNC, 0);
  }
  public synchronized void setEarliestNextSync(long next) {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    Editor edit = sharedPreferences.edit();
    edit.putLong(PREFS_EARLIEST_NEXT_SYNC, next);
    edit.commit();
  }
  public synchronized void extendEarliestNextSync(long next) {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    if (sharedPreferences.getLong(PREFS_EARLIEST_NEXT_SYNC, 0) >= next) {
      return;
    }
    Editor edit = sharedPreferences.edit();
    edit.putLong(PREFS_EARLIEST_NEXT_SYNC, next);
    edit.commit();
  }

  public synchronized boolean getShouldInvalidateAuthToken() {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    return sharedPreferences.getBoolean(PREFS_INVALIDATE_AUTH_TOKEN, false);
  }
  public synchronized void clearShouldInvalidateAuthToken() {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    Editor edit = sharedPreferences.edit();
    edit.remove(PREFS_INVALIDATE_AUTH_TOKEN);
    edit.commit();
  }
  public synchronized void setShouldInvalidateAuthToken() {
    SharedPreferences sharedPreferences = mContext.getSharedPreferences("sync.prefs.global", SHARED_PREFERENCES_MODE);
    Editor edit = sharedPreferences.edit();
    edit.putBoolean(PREFS_INVALIDATE_AUTH_TOKEN, true);
    edit.commit();
  }

  private void handleException(Exception e, SyncResult syncResult) {
    setShouldInvalidateAuthToken();
    try {
      if (e instanceof SQLiteConstraintException) {
        Log.e(LOG_TAG, "Constraint exception. Aborting sync.", e);
        syncResult.stats.numParseExceptions++;       
        return;
      }
      if (e instanceof SQLiteException) {
        Log.e(LOG_TAG, "Couldn't open database (locked?). Aborting sync.", e);
        syncResult.stats.numIoExceptions++;
        return;
      }
      if (e instanceof OperationCanceledException) {
        Log.e(LOG_TAG, "Operation canceled. Aborting sync.", e);
        return;
      }
      if (e instanceof AuthenticatorException) {
        syncResult.stats.numParseExceptions++;
        Log.e(LOG_TAG, "AuthenticatorException. Aborting sync.", e);
        return;
      }
      if (e instanceof IOException) {
        syncResult.stats.numIoExceptions++;
        Log.e(LOG_TAG, "IOException. Aborting sync.", e);
        e.printStackTrace();
        return;
      }
      syncResult.stats.numIoExceptions++;
      Log.e(LOG_TAG, "Unknown exception. Aborting sync.", e);
    } finally {
      notifyMonitor();
    }
  }

  private AccountManagerFuture<Bundle> getAuthToken(final Account account,
                            AccountManagerCallback<Bundle> callback,
                            Handler handler) {
    return mAccountManager.getAuthToken(account, Constants.AUTHTOKEN_TYPE_PLAIN, true, callback, handler);
  }

  private void invalidateAuthToken(Account account) {
    AccountManagerFuture<Bundle> future = getAuthToken(account, null, null);
    String token;
    try {
      token = future.getResult().getString(AccountManager.KEY_AUTHTOKEN);
      mAccountManager.invalidateAuthToken(Constants.ACCOUNTTYPE_SYNC, token);
    } catch (Exception e) {
      Log.e(LOG_TAG, "Couldn't invalidate auth token: " + e);
    }
  }

  @Override
  public void onSyncCanceled() {
    super.onSyncCanceled();
    
    
    
    
  }

  public Object syncMonitor = new Object();
  private SyncResult syncResult;

  



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
    return delayMilliseconds() > 0;
  }

  @Override
  public void onPerformSync(final Account account,
                            final Bundle extras,
                            final String authority,
                            final ContentProviderClient provider,
                            final SyncResult syncResult) {
    Utils.reseedSharedRandom(); 

    long delay = delayMilliseconds();
    if (delay > 0) {
      Log.i(LOG_TAG, "Not syncing: must wait another " + delay + "ms.");
      long remainingSeconds = delay / 1000;
      syncResult.delayUntil = remainingSeconds + BACKOFF_PAD_SECONDS;
      return;
    }

    
    Log.i(LOG_TAG, "Got onPerformSync. Extras bundle is " + extras);
    Log.d(LOG_TAG, "Extras clusterURL: " + extras.getString("clusterURL"));
    Log.i(LOG_TAG, "Account name: " + account.name);

    
    
    Log.d(LOG_TAG, "Invalidating auth token.");
    invalidateAuthToken(account);

    final SyncAdapter self = this;
    final AccountManagerCallback<Bundle> callback = new AccountManagerCallback<Bundle>() {
      @Override
      public void run(AccountManagerFuture<Bundle> future) {
        Log.i(LOG_TAG, "AccountManagerCallback invoked.");
        
        try {
          Bundle bundle = future.getResult(60L, TimeUnit.SECONDS);
          if (bundle.containsKey("KEY_INTENT")) {
            Log.w(LOG_TAG, "KEY_INTENT included in AccountManagerFuture bundle. Problem?");
          }
          String username  = bundle.getString(Constants.OPTION_USERNAME);
          String syncKey   = bundle.getString(Constants.OPTION_SYNCKEY);
          String serverURL = bundle.getString(Constants.OPTION_SERVER);
          String password  = bundle.getString(AccountManager.KEY_AUTHTOKEN);
          Log.d(LOG_TAG, "Username: " + username);
          Log.d(LOG_TAG, "Server:   " + serverURL);
          Log.d(LOG_TAG, "Password? " + (password != null));
          Log.d(LOG_TAG, "Key?      " + (syncKey != null));
          if (password == null) {
            Log.e(LOG_TAG, "No password: aborting sync.");
            syncResult.stats.numAuthExceptions++;
            notifyMonitor();
            return;
          }
          if (syncKey == null) {
            Log.e(LOG_TAG, "No Sync Key: aborting sync.");
            syncResult.stats.numAuthExceptions++;
            notifyMonitor();
            return;
          }
          KeyBundle keyBundle = new KeyBundle(username, syncKey);

          
          
          String prefsPath = Utils.getPrefsPath(username, serverURL);
          self.performSync(account, extras, authority, provider, syncResult,
              username, password, prefsPath, serverURL, keyBundle);
        } catch (Exception e) {
          self.handleException(e, syncResult);
          return;
        }
      }
    };

    final Handler handler = null;
    final Runnable fetchAuthToken = new Runnable() {
      @Override
      public void run() {
        getAuthToken(account, callback, handler);
      }
    };
    synchronized (syncMonitor) {
      
      
      
      new Thread(fetchAuthToken).start();

      Log.i(LOG_TAG, "Waiting on sync monitor.");
      try {
        syncMonitor.wait();
      } catch (InterruptedException e) {
        Log.i(LOG_TAG, "Waiting on sync monitor interrupted.", e);
      }
    }
 }


  










  protected void performSync(Account account, Bundle extras, String authority,
                             ContentProviderClient provider,
                             SyncResult syncResult,
                             String username, String password,
                             String prefsPath,
                             String serverURL, KeyBundle keyBundle)
                                 throws NoSuchAlgorithmException,
                                        SyncConfigurationException,
                                        IllegalArgumentException,
                                        AlreadySyncingException,
                                        IOException, ParseException,
                                        NonObjectJSONException {
    Log.i(LOG_TAG, "Performing sync.");
    this.syncResult = syncResult;
    
    GlobalSession globalSession = new GlobalSession(SyncConfiguration.DEFAULT_USER_API,
                                                    serverURL, username, password, prefsPath,
                                                    keyBundle, this, this.mContext, extras);

    globalSession.start();

  }

  private void notifyMonitor() {
    synchronized (syncMonitor) {
      Log.i(LOG_TAG, "Notifying sync monitor.");
      syncMonitor.notifyAll();
    }
  }

  
  @Override
  public void handleError(GlobalSession globalSession, Exception ex) {
    Log.i(LOG_TAG, "GlobalSession indicated error. Flagging auth token as invalid, just in case.");
    setShouldInvalidateAuthToken();
    this.updateStats(globalSession, ex);
    notifyMonitor();
  }

  @Override
  public void handleAborted(GlobalSession globalSession, String reason) {
    Log.w(LOG_TAG, "Sync aborted: " + reason);
    notifyMonitor();
  }

  






  private void updateStats(GlobalSession globalSession,
                           Exception ex) {
    if (ex instanceof SyncException) {
      ((SyncException) ex).updateStats(globalSession, syncResult);
    }
    
    
  }

  @Override
  public void handleSuccess(GlobalSession globalSession) {
    Log.i(LOG_TAG, "GlobalSession indicated success.");
    Log.i(LOG_TAG, "Prefs target: " + globalSession.config.prefsPath);
    globalSession.config.persistToPrefs();
    notifyMonitor();
  }

  @Override
  public void handleStageCompleted(Stage currentState,
                                   GlobalSession globalSession) {
    Log.i(LOG_TAG, "Stage completed: " + currentState);
  }

  @Override
  public void requestBackoff(long backoff) {
    if (backoff > 0) {
      this.extendEarliestNextSync(System.currentTimeMillis() + backoff);
    }
  }
}