




































package org.mozilla.gecko.sync.syncadapter;

import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import java.util.concurrent.TimeUnit;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.AlreadySyncingException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConfigurationException;
import org.mozilla.gecko.sync.SyncException;
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
import android.content.SyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

public class SyncAdapter extends AbstractThreadedSyncAdapter implements GlobalSessionCallback {

  private static final String  LOG_TAG = "SyncAdapter";
  private final AccountManager mAccountManager;
  private final Context        mContext;

  public SyncAdapter(Context context, boolean autoInitialize) {
    super(context, autoInitialize);
    mContext = context;
    mAccountManager = AccountManager.get(context);
  }

  private void handleException(Exception e, SyncResult syncResult) {
    if (e instanceof OperationCanceledException) {
      Log.e(LOG_TAG, "Operation canceled. Aborting sync.");
      e.printStackTrace();
      return;
    }
    if (e instanceof AuthenticatorException) {
      syncResult.stats.numParseExceptions++;
      Log.e(LOG_TAG, "AuthenticatorException. Aborting sync.");
      e.printStackTrace();
      return;
    }
    if (e instanceof IOException) {
      syncResult.stats.numIoExceptions++;
      Log.e(LOG_TAG, "IOException. Aborting sync.");
      e.printStackTrace();
      return;
    }
    syncResult.stats.numIoExceptions++;
    Log.e(LOG_TAG, "Unknown exception. Aborting sync.");
    e.printStackTrace();
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

  public boolean shouldPerformSync = false;

  @Override
  public void onPerformSync(final Account account,
                            final Bundle extras,
                            final String authority,
                            final ContentProviderClient provider,
                            final SyncResult syncResult) {

    
    Log.i(LOG_TAG, "Got onPerformSync. Extras bundle is " + extras);
    Log.d(LOG_TAG, "Extras clusterURL: " + extras.getString("clusterURL"));
    Log.i(LOG_TAG, "Account name: " + account.name);
    if (!shouldPerformSync) {
      Log.i(LOG_TAG, "Not performing sync.");
      return;
    }
    Log.i(LOG_TAG, "XXX CLEARING AUTH TOKEN XXX");
    invalidateAuthToken(account);

    final SyncAdapter self = this;
    AccountManagerCallback<Bundle> callback = new AccountManagerCallback<Bundle>() {
      @Override
      public void run(AccountManagerFuture<Bundle> future) {
        Log.i(LOG_TAG, "AccountManagerCallback invoked.");
        
        try {
          Bundle bundle      = future.getResult(60L, TimeUnit.SECONDS);
          String username    = bundle.getString(Constants.OPTION_USERNAME);
          String syncKey     = bundle.getString(Constants.OPTION_SYNCKEY);
          String serverURL   = bundle.getString(Constants.OPTION_SERVER);
          String password    = bundle.getString(AccountManager.KEY_AUTHTOKEN);
          Log.d(LOG_TAG, "Username: " + username);
          Log.d(LOG_TAG, "Server:   " + serverURL);
          Log.d(LOG_TAG, "Password: " + password);  
          Log.d(LOG_TAG, "Key:      " + syncKey);   
          if (password == null) {
            Log.e(LOG_TAG, "No password: aborting sync.");
            return;
          }
          if (syncKey == null) {
            Log.e(LOG_TAG, "No Sync Key: aborting sync.");
            return;
          }
          KeyBundle keyBundle = new KeyBundle(username, syncKey);
          self.performSync(account, extras, authority, provider, syncResult,
              username, password, serverURL, keyBundle);
        } catch (Exception e) {
          self.handleException(e, syncResult);
          return;
        }
      }
    };
    Handler handler = null;
    getAuthToken(account, callback, handler);
    synchronized (syncMonitor) {
      try {
        Log.i(LOG_TAG, "Waiting on sync monitor.");
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
                             String serverURL,
                             KeyBundle keyBundle)
                                 throws NoSuchAlgorithmException,
                                        SyncConfigurationException,
                                        IllegalArgumentException,
                                        AlreadySyncingException,
                                        IOException, ParseException,
                                        NonObjectJSONException {
    Log.i(LOG_TAG, "Performing sync.");
    this.syncResult = syncResult;
    
    GlobalSession globalSession = new GlobalSession(SyncConfiguration.DEFAULT_USER_API,
                                                    serverURL, username, password, keyBundle,
                                                    this, this.mContext, null);

    globalSession.start();

  }

  private void notifyMonitor() {
    synchronized (syncMonitor) {
      Log.i(LOG_TAG, "Notifying sync monitor.");
      syncMonitor.notify();
    }
  }

  
  @Override
  public void handleError(GlobalSession globalSession, Exception ex) {
    Log.i(LOG_TAG, "GlobalSession indicated error.");
    this.updateStats(globalSession, ex);
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
    notifyMonitor();
  }

  @Override
  public void handleStageCompleted(Stage currentState,
                                   GlobalSession globalSession) {
    Log.i(LOG_TAG, "Stage completed: " + currentState);
  }
}














































































