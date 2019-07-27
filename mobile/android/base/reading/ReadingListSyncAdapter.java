



package org.mozilla.gecko.reading;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collection;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import org.mozilla.gecko.background.ReadingListConstants;
import org.mozilla.gecko.background.common.PrefsBranch;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.sync.FxAccountSyncDelegate;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BearerAuthHeaderProvider;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SyncResult;
import android.os.Bundle;

public class ReadingListSyncAdapter extends AbstractThreadedSyncAdapter {
  public static final String PREF_LOCAL_NAME = "device.localname";

  private static final String LOG_TAG = ReadingListSyncAdapter.class.getSimpleName();
  private static final long TIMEOUT_SECONDS = 60;
  protected final ExecutorService executor;

  public ReadingListSyncAdapter(Context context, boolean autoInitialize) {
    super(context, autoInitialize);
    this.executor = Executors.newSingleThreadExecutor();
  }

  protected static abstract class SyncAdapterSynchronizerDelegate implements ReadingListSynchronizerDelegate {
    private final FxAccountSyncDelegate syncDelegate;
    private final ContentProviderClient cpc;
    private final SyncResult result;

    SyncAdapterSynchronizerDelegate(FxAccountSyncDelegate syncDelegate,
                                    ContentProviderClient cpc,
                                    SyncResult result) {
      this.syncDelegate = syncDelegate;
      this.cpc = cpc;
      this.result = result;
    }

    abstract public void onInvalidAuthentication();

    @Override
    public void onUnableToSync(Exception e) {
      Logger.warn(LOG_TAG, "Unable to sync.", e);
      if (e instanceof ReadingListInvalidAuthenticationException) {
        onInvalidAuthentication();
      }
      cpc.release();
      syncDelegate.handleError(e);
    }

    @Override
    public void onDeletionsUploadComplete() {
      Logger.debug(LOG_TAG, "Step: onDeletionsUploadComplete");
      this.result.stats.numEntries += 1;   
    }

    @Override
    public void onStatusUploadComplete(Collection<String> uploaded,
                                       Collection<String> failed) {
      Logger.debug(LOG_TAG, "Step: onStatusUploadComplete");
      this.result.stats.numEntries += 1;   
    }

    @Override
    public void onNewItemUploadComplete(Collection<String> uploaded,
                                        Collection<String> failed) {
      Logger.debug(LOG_TAG, "Step: onNewItemUploadComplete");
      this.result.stats.numEntries += 1;   
    }

    @Override
    public void onModifiedUploadComplete() {
      Logger.debug(LOG_TAG, "Step: onModifiedUploadComplete");
      this.result.stats.numEntries += 1;   
    }

    @Override
    public void onDownloadComplete() {
      Logger.debug(LOG_TAG, "Step: onDownloadComplete");
      this.result.stats.numInserts += 1;   
    }

    @Override
    public void onComplete() {
      Logger.info(LOG_TAG, "Reading list synchronization complete.");
      cpc.release();
      syncDelegate.handleSuccess();
    }
  }

  private void syncWithAuthorization(final Context context,
                                     final String endpointString,
                                     final SyncResult syncResult,
                                     final FxAccountSyncDelegate syncDelegate,
                                     final String authToken,
                                     final SharedPreferences sharedPrefs,
                                     final Bundle extras) {
    final AuthHeaderProvider auth = new BearerAuthHeaderProvider(authToken);

    final URI endpoint;
    Logger.info(LOG_TAG, "Syncing reading list against " + endpointString);
    try {
      endpoint = new URI(endpointString);
    } catch (URISyntaxException e) {
      
      Logger.error(LOG_TAG, "Unexpected malformed URI for reading list service: " + endpointString);
      syncDelegate.handleError(e);
      return;
    }

    final PrefsBranch branch = new PrefsBranch(sharedPrefs, "readinglist.");
    final ReadingListClient remote = new ReadingListClient(endpoint, auth);
    final ContentProviderClient cpc = getContentProviderClient(context); 

    final LocalReadingListStorage local = new LocalReadingListStorage(cpc);
    String localName = branch.getString(PREF_LOCAL_NAME, null);
    if (localName == null) {
      localName = FxAccountUtils.defaultClientName(context);
    }

    
    local.updateLocalNames(localName);

    final ReadingListSynchronizer synchronizer = new ReadingListSynchronizer(branch, remote, local);

    synchronizer.syncAll(new SyncAdapterSynchronizerDelegate(syncDelegate, cpc, syncResult) {
      @Override
      public void onInvalidAuthentication() {
        
        
        
        Logger.info(LOG_TAG, "Invalidating oauth token after 401!");
        AccountManager.get(context).invalidateAuthToken(FxAccountConstants.ACCOUNT_TYPE, authToken);
      }
    });
    
  }

  @Override
  public void onPerformSync(final Account account, final Bundle extras, final String authority, final ContentProviderClient provider, final SyncResult syncResult) {
    Logger.setThreadLogTag(ReadingListConstants.GLOBAL_LOG_TAG);
    Logger.resetLogging();

    final Context context = getContext();
    final AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);

    
    final String accountServerURI = fxAccount.getAccountServerURI();
    final boolean usingDefaultAuthServer = FxAccountConstants.DEFAULT_AUTH_SERVER_ENDPOINT.equals(accountServerURI);
    final boolean usingStageAuthServer = FxAccountConstants.STAGE_AUTH_SERVER_ENDPOINT.equals(accountServerURI);
    if (!usingDefaultAuthServer && !usingStageAuthServer) {
      Logger.error(LOG_TAG, "Skipping Reading List sync because Firefox Account is not using prod or stage auth server.");
      
      ContentResolver.setIsSyncable(account, BrowserContract.READING_LIST_AUTHORITY, 0);
      return;
    }
    final String tokenServerURI = fxAccount.getTokenServerURI();
    final boolean usingDefaultSyncServer = FxAccountConstants.DEFAULT_TOKEN_SERVER_ENDPOINT.equals(tokenServerURI);
    final boolean usingStageSyncServer = FxAccountConstants.STAGE_TOKEN_SERVER_ENDPOINT.equals(tokenServerURI);
    if (!usingDefaultSyncServer && !usingStageSyncServer) {
      Logger.error(LOG_TAG, "Skipping Reading List sync because Sync is not using the prod or stage Sync (token) server.");
      Logger.debug(LOG_TAG, "If the user has chosen to not store Sync data with Mozilla, we shouldn't store Reading List data with Mozilla .");
      
      ContentResolver.setIsSyncable(account, BrowserContract.READING_LIST_AUTHORITY, 0);
      return;
    }

    
    final String endpointString;
    if (usingStageAuthServer) {
      endpointString = ReadingListConstants.DEFAULT_DEV_ENDPOINT;
    } else {
      endpointString = ReadingListConstants.DEFAULT_PROD_ENDPOINT;
    }

    final CountDownLatch latch = new CountDownLatch(1);
    final FxAccountSyncDelegate syncDelegate = new FxAccountSyncDelegate(latch, syncResult);

    final AccountManager accountManager = AccountManager.get(context);
    
    
    
    
    final boolean notifyAuthFailure = true;
    try {
      final String authToken = accountManager.blockingGetAuthToken(account, ReadingListConstants.AUTH_TOKEN_TYPE, notifyAuthFailure);
      if (authToken == null) {
        throw new RuntimeException("Couldn't get oauth token!  Aborting sync.");
      }
      final SharedPreferences sharedPrefs = fxAccount.getReadingListPrefs();
      syncWithAuthorization(context, endpointString, syncResult, syncDelegate, authToken, sharedPrefs, extras);

      latch.await(TIMEOUT_SECONDS, TimeUnit.SECONDS);
      Logger.info(LOG_TAG, "Reading list sync done.");
    } catch (Exception e) {
      
      Logger.error(LOG_TAG, "Got error syncing.", e);
      syncDelegate.handleError(e);
    }

    





    









  }

  private ContentProviderClient getContentProviderClient(Context context) {
    final ContentResolver contentResolver = context.getContentResolver();
    final ContentProviderClient client = contentResolver.acquireContentProviderClient(ReadingListItems.CONTENT_URI);
    return client;
  }
}
