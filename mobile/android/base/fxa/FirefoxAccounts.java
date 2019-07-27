



package org.mozilla.gecko.fxa;

import java.io.File;
import java.util.EnumSet;
import java.util.Locale;
import java.util.concurrent.CountDownLatch;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.fxa.authenticator.AccountPickler;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.sync.FxAccountSyncAdapter;
import org.mozilla.gecko.fxa.sync.FxAccountSyncStatusHelper;
import org.mozilla.gecko.fxa.tasks.FxAccountCodeResender;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.Bundle;




public class FirefoxAccounts {
  private static final String LOG_TAG = FirefoxAccounts.class.getSimpleName();

  public enum SyncHint {
    





    SCHEDULE_NOW,

    




    IGNORE_LOCAL_RATE_LIMIT,

    




    IGNORE_REMOTE_SERVER_BACKOFF,
  }

  public static final EnumSet<SyncHint> SOON = EnumSet.noneOf(SyncHint.class);

  public static final EnumSet<SyncHint> NOW = EnumSet.of(
      SyncHint.SCHEDULE_NOW);

  public static final EnumSet<SyncHint> FORCE = EnumSet.of(
      SyncHint.SCHEDULE_NOW,
      SyncHint.IGNORE_LOCAL_RATE_LIMIT,
      SyncHint.IGNORE_REMOTE_SERVER_BACKOFF);

  public interface SyncStatusListener {
    public Context getContext();
    public Account getAccount();
    public void onSyncStarted();
    public void onSyncFinished();
  }

  





  public static boolean firefoxAccountsExist(final Context context) {
    return getFirefoxAccounts(context).length > 0;
  }

  











  public static Account[] getFirefoxAccounts(final Context context) {
    final Account[] accounts =
        AccountManager.get(context).getAccountsByType(FxAccountConstants.ACCOUNT_TYPE);
    if (accounts.length > 0) {
      return accounts;
    }

    final Account pickledAccount = getPickledAccount(context);
    return (pickledAccount != null) ? new Account[] {pickledAccount} : new Account[0];
  }

  private static Account getPickledAccount(final Context context) {
    
    
    final CountDownLatch latch = new CountDownLatch(1);
    final Account[] accounts = new Account[1];
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        try {
          final File file = context.getFileStreamPath(FxAccountConstants.ACCOUNT_PICKLE_FILENAME);
          if (!file.exists()) {
            accounts[0] = null;
            return;
          }

          
          
          
          final AndroidFxAccount fxAccount =
              AccountPickler.unpickle(context, FxAccountConstants.ACCOUNT_PICKLE_FILENAME);
          accounts[0] = fxAccount.getAndroidAccount();
        } finally {
          latch.countDown();
        }
      }
    });

    try {
      latch.await(); 
    } catch (InterruptedException e) {
      Logger.warn(LOG_TAG,
          "Foreground thread unexpectedly interrupted while getting pickled account", e);
      return null;
    }

    return accounts[0];
  }

  



  public static Account getFirefoxAccount(final Context context) {
    Account[] accounts = getFirefoxAccounts(context);
    if (accounts.length > 0) {
      return accounts[0];
    }
    return null;
  }

  




  public static State getFirefoxAccountState(final Context context) {
    final Account account = getFirefoxAccount(context);
    if (account == null) {
      return null;
    }

    final AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);
    try {
      return fxAccount.getState();
    } catch (final Exception ex) {
      Logger.warn(LOG_TAG, "Could not get FX account state.", ex);
      return null;
    }
  }

  



  public static String getFirefoxAccountEmail(final Context context) {
    final Account account = getFirefoxAccount(context);
    if (account == null) {
      return null;
    }
    return account.name;
  }

  protected static void putHintsToSync(final Bundle extras, EnumSet<SyncHint> syncHints) {
    
    if (syncHints == null) {
      throw new IllegalArgumentException("syncHints must not be null");
    }

    final boolean scheduleNow = syncHints.contains(SyncHint.SCHEDULE_NOW);
    final boolean ignoreLocalRateLimit = syncHints.contains(SyncHint.IGNORE_LOCAL_RATE_LIMIT);
    final boolean ignoreRemoteServerBackoff = syncHints.contains(SyncHint.IGNORE_REMOTE_SERVER_BACKOFF);

    extras.putBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, scheduleNow);
    
    
    
    
    extras.putBoolean(FxAccountSyncAdapter.SYNC_EXTRAS_RESPECT_LOCAL_RATE_LIMIT, !ignoreLocalRateLimit);
    extras.putBoolean(FxAccountSyncAdapter.SYNC_EXTRAS_RESPECT_REMOTE_SERVER_BACKOFF, !ignoreRemoteServerBackoff);
  }

  public static EnumSet<SyncHint> getHintsToSyncFromBundle(final Bundle extras) {
    final EnumSet<SyncHint> syncHints = EnumSet.noneOf(SyncHint.class);

    final boolean scheduleNow = extras.getBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, false);
    final boolean ignoreLocalRateLimit = !extras.getBoolean(FxAccountSyncAdapter.SYNC_EXTRAS_RESPECT_LOCAL_RATE_LIMIT, false);
    final boolean ignoreRemoteServerBackoff = !extras.getBoolean(FxAccountSyncAdapter.SYNC_EXTRAS_RESPECT_REMOTE_SERVER_BACKOFF, false);

    if (scheduleNow) {
      syncHints.add(SyncHint.SCHEDULE_NOW);
    }
    if (ignoreLocalRateLimit) {
      syncHints.add(SyncHint.IGNORE_LOCAL_RATE_LIMIT);
    }
    if (ignoreRemoteServerBackoff) {
      syncHints.add(SyncHint.IGNORE_REMOTE_SERVER_BACKOFF);
    }

    return syncHints;
  }

  public static void logSyncHints(EnumSet<SyncHint> syncHints) {
    final boolean scheduleNow = syncHints.contains(SyncHint.SCHEDULE_NOW);
    final boolean ignoreLocalRateLimit = syncHints.contains(SyncHint.IGNORE_LOCAL_RATE_LIMIT);
    final boolean ignoreRemoteServerBackoff = syncHints.contains(SyncHint.IGNORE_REMOTE_SERVER_BACKOFF);

    Logger.info(LOG_TAG, "Sync hints" +
        "; scheduling now: " + scheduleNow +
        "; ignoring local rate limit: " + ignoreLocalRateLimit +
        "; ignoring remote server backoff: " + ignoreRemoteServerBackoff + ".");
  }

  













  public static void requestSync(final Account account, EnumSet<SyncHint> syncHints, String[] stagesToSync, String[] stagesToSkip) {
    if (account == null) {
      throw new IllegalArgumentException("account must not be null");
    }
    if (syncHints == null) {
      throw new IllegalArgumentException("syncHints must not be null");
    }

    final Bundle extras = new Bundle();
    putHintsToSync(extras, syncHints);
    Utils.putStageNamesToSync(extras, stagesToSync, stagesToSkip);

    Logger.info(LOG_TAG, "Requesting sync.");
    logSyncHints(syncHints);

    
    
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        for (String authority : AndroidFxAccount.DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.keySet()) {
          ContentResolver.requestSync(account, authority, extras);
        }
      }
    });
  }

  






  public static void addSyncStatusListener(SyncStatusListener syncStatusListener) {
    
    FxAccountSyncStatusHelper.getInstance().startObserving(syncStatusListener);
  }

  




  public static void removeSyncStatusListener(SyncStatusListener syncStatusListener) {
    
    FxAccountSyncStatusHelper.getInstance().stopObserving(syncStatusListener);
  }

  public static String getOldSyncUpgradeURL(final Resources res, final Locale locale) {
    final String VERSION = AppConstants.MOZ_APP_VERSION;
    final String OS = AppConstants.OS_TARGET;
    final String LOCALE = Utils.getLanguageTag(locale);
    return res.getString(R.string.fxaccount_link_old_firefox, VERSION, OS, LOCALE);
  }

  










  public static boolean resendVerificationEmail(final Context context) {
    final Account account = getFirefoxAccount(context);
    if (account == null) {
      return false;
    }

    final AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);
    FxAccountCodeResender.resendCode(context, fxAccount);
    return true;
  }
}
