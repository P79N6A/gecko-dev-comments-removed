



package org.mozilla.gecko.fxa.sync;

import java.net.URI;
import java.net.URISyntaxException;
import java.security.NoSuchAlgorithmException;
import java.util.Collection;
import java.util.Collections;
import java.util.EnumSet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient;
import org.mozilla.gecko.background.fxa.FxAccountClient20;
import org.mozilla.gecko.background.fxa.SkewHandler;
import org.mozilla.gecko.browserid.BrowserIDKeyPair;
import org.mozilla.gecko.browserid.JSONWebTokenUtils;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AccountPickler;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.authenticator.FxAccountAuthenticator;
import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine;
import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine.LoginStateMachineDelegate;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.Transition;
import org.mozilla.gecko.fxa.login.Married;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.fxa.login.StateFactory;
import org.mozilla.gecko.sync.BackoffHandler;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.PrefsBackoffHandler;
import org.mozilla.gecko.sync.SharedPreferencesClientsDataDelegate;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.BaseGlobalSessionCallback;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.HawkAuthHeaderProvider;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;
import org.mozilla.gecko.tokenserver.TokenServerClient;
import org.mozilla.gecko.tokenserver.TokenServerClientDelegate;
import org.mozilla.gecko.tokenserver.TokenServerException;
import org.mozilla.gecko.tokenserver.TokenServerToken;

import android.accounts.Account;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SyncResult;
import android.os.Bundle;
import android.os.SystemClock;

public class FxAccountSyncAdapter extends AbstractThreadedSyncAdapter {
  private static final String LOG_TAG = FxAccountSyncAdapter.class.getSimpleName();

  public static final String SYNC_EXTRAS_RESPECT_LOCAL_RATE_LIMIT = "respect_local_rate_limit";
  public static final String SYNC_EXTRAS_RESPECT_REMOTE_SERVER_BACKOFF = "respect_remote_server_backoff";

  protected static final int NOTIFICATION_ID = LOG_TAG.hashCode();

  
  private static final String PREF_BACKOFF_STORAGE_HOST = "backoffStorageHost";

  
  
  private static final int MINIMUM_SYNC_DELAY_MILLIS = 15 * 1000;        
  private volatile long lastSyncRealtimeMillis = 0L;

  protected final ExecutorService executor;
  protected final FxAccountNotificationManager notificationManager;

  public FxAccountSyncAdapter(Context context, boolean autoInitialize) {
    super(context, autoInitialize);
    this.executor = Executors.newSingleThreadExecutor();
    this.notificationManager = new FxAccountNotificationManager(NOTIFICATION_ID);
  }

  protected static class SyncDelegate {
    protected final CountDownLatch latch;
    protected final SyncResult syncResult;
    protected final AndroidFxAccount fxAccount;
    protected final Collection<String> stageNamesToSync;

    public SyncDelegate(CountDownLatch latch, SyncResult syncResult, AndroidFxAccount fxAccount, Collection<String> stageNamesToSync) {
      if (latch == null) {
        throw new IllegalArgumentException("latch must not be null");
      }
      if (syncResult == null) {
        throw new IllegalArgumentException("syncResult must not be null");
      }
      if (fxAccount == null) {
        throw new IllegalArgumentException("fxAccount must not be null");
      }
      this.latch = latch;
      this.syncResult = syncResult;
      this.fxAccount = fxAccount;
      this.stageNamesToSync = Collections.unmodifiableCollection(stageNamesToSync);
    }

    


    protected void setSyncResultSuccess() {
      syncResult.stats.numUpdates += 1;
    }

    



    protected void setSyncResultSoftError() {
      syncResult.stats.numUpdates += 1;
      syncResult.stats.numIoExceptions += 1;
    }

    



    protected void setSyncResultHardError() {
      syncResult.stats.numAuthExceptions += 1;
    }

    public void handleSuccess() {
      Logger.info(LOG_TAG, "Sync succeeded.");
      setSyncResultSuccess();
      latch.countDown();
    }

    public void handleError(Exception e) {
      Logger.error(LOG_TAG, "Got exception syncing.", e);
      setSyncResultSoftError();
      
      
      if (e instanceof TokenServerException) {
        
        State state = fxAccount.getState();
        if (state.getStateLabel() == StateLabel.Married) {
          Married married = (Married) state;
          fxAccount.setState(married.makeCohabitingState());
        }
      }
      latch.countDown();
    }

    













    public void handleCannotSync(State finalState) {
      Logger.warn(LOG_TAG, "Cannot sync from state: " + finalState.getStateLabel());
      setSyncResultSoftError();
      latch.countDown();
    }

    public void postponeSync(long millis) {
      if (millis <= 0) {
        Logger.debug(LOG_TAG, "Asked to postpone sync, but zero delay. Short-circuiting.");
      } else {
        
        
        
        



      }
      setSyncResultSoftError();
      latch.countDown();
    }

    




    public void rejectSync() {
      latch.countDown();
    }

    public Collection<String> getStageNamesToSync() {
      return this.stageNamesToSync;
    }
  }

  protected static class SessionCallback implements BaseGlobalSessionCallback {
    protected final SyncDelegate syncDelegate;
    protected final SchedulePolicy schedulePolicy;
    protected volatile BackoffHandler storageBackoffHandler;

    public SessionCallback(SyncDelegate syncDelegate, SchedulePolicy schedulePolicy) {
      this.syncDelegate = syncDelegate;
      this.schedulePolicy = schedulePolicy;
    }

    public void setBackoffHandler(BackoffHandler backoffHandler) {
      this.storageBackoffHandler = backoffHandler;
    }

    @Override
    public boolean shouldBackOffStorage() {
      return storageBackoffHandler.delayMilliseconds() > 0;
    }

    @Override
    public void requestBackoff(long backoffMillis) {
      final boolean onlyExtend = true;      
      schedulePolicy.configureBackoffMillisOnBackoff(storageBackoffHandler, backoffMillis, onlyExtend);
    }

    @Override
    public void informUpgradeRequiredResponse(GlobalSession session) {
      schedulePolicy.onUpgradeRequired();
    }

    @Override
    public void informUnauthorizedResponse(GlobalSession globalSession, URI oldClusterURL) {
      schedulePolicy.onUnauthorized();
    }

    @Override
    public void handleStageCompleted(Stage currentState, GlobalSession globalSession) {
    }

    @Override
    public void handleSuccess(GlobalSession globalSession) {
      Logger.info(LOG_TAG, "Global session succeeded.");

      
      try {
        int otherClientsCount = globalSession.getClientsDelegate().getClientsCount();
        Logger.debug(LOG_TAG, "" + otherClientsCount + " other client(s).");
        this.schedulePolicy.onSuccessfulSync(otherClientsCount);
      } finally {
        
        syncDelegate.handleSuccess();
      }
    }

    @Override
    public void handleError(GlobalSession globalSession, Exception e) {
      Logger.warn(LOG_TAG, "Global session failed."); 
      syncDelegate.handleError(e);
      
    }

    @Override
    public void handleAborted(GlobalSession globalSession, String reason) {
      Logger.warn(LOG_TAG, "Global session aborted: " + reason);
      syncDelegate.handleError(null);
      
    }
  };

  




  private boolean shouldPerformSync(final BackoffHandler backoffHandler, final String kind, final Bundle extras) {
    final long delay = backoffHandler.delayMilliseconds();
    if (delay <= 0) {
      return true;
    }

    if (extras == null) {
      return false;
    }

    final boolean forced = extras.getBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, false);
    if (forced) {
      Logger.info(LOG_TAG, "Forced sync (" + kind + "): overruling remaining backoff of " + delay + "ms.");
    } else {
      Logger.info(LOG_TAG, "Not syncing (" + kind + "): must wait another " + delay + "ms.");
    }
    return forced;
  }

  protected void syncWithAssertion(final String audience,
                                   final String assertion,
                                   final URI tokenServerEndpointURI,
                                   final BackoffHandler tokenBackoffHandler,
                                   final SharedPreferences sharedPrefs,
                                   final KeyBundle syncKeyBundle,
                                   final String clientState,
                                   final SessionCallback callback,
                                   final Bundle extras) {
    final TokenServerClientDelegate delegate = new TokenServerClientDelegate() {
      private boolean didReceiveBackoff = false;

      @Override
      public String getUserAgent() {
        return FxAccountConstants.USER_AGENT;
      }

      @Override
      public void handleSuccess(final TokenServerToken token) {
        FxAccountConstants.pii(LOG_TAG, "Got token! uid is " + token.uid + " and endpoint is " + token.endpoint + ".");

        if (!didReceiveBackoff) {
          
          tokenBackoffHandler.setEarliestNextRequest(0L);
        }

        final URI storageServerURI;
        try {
          storageServerURI = new URI(token.endpoint);
        } catch (URISyntaxException e) {
          handleError(e);
          return;
        }
        final String storageHostname = storageServerURI.getHost();

        
        
        
        final BackoffHandler storageBackoffHandler = new PrefsBackoffHandler(sharedPrefs, "sync.storage");
        callback.setBackoffHandler(storageBackoffHandler);

        String lastStorageHost = sharedPrefs.getString(PREF_BACKOFF_STORAGE_HOST, null);
        final boolean storageHostIsUnchanged = lastStorageHost != null &&
                                               lastStorageHost.equalsIgnoreCase(storageHostname);
        if (storageHostIsUnchanged) {
          Logger.debug(LOG_TAG, "Storage host is unchanged.");
          if (!shouldPerformSync(storageBackoffHandler, "storage", extras)) {
            Logger.info(LOG_TAG, "Not syncing: storage server requested backoff.");
            callback.handleAborted(null, "Storage backoff");
            return;
          }
        } else {
          Logger.debug(LOG_TAG, "Received new storage host.");
        }

        
        
        storageBackoffHandler.setEarliestNextRequest(0L);

        FxAccountGlobalSession globalSession = null;
        try {
          final ClientsDataDelegate clientsDataDelegate = new SharedPreferencesClientsDataDelegate(sharedPrefs, getContext());
          if (FxAccountConstants.LOG_PERSONAL_INFORMATION) {
            FxAccountConstants.pii(LOG_TAG, "Client device name is: '" + clientsDataDelegate.getClientName() + "'.");
            FxAccountConstants.pii(LOG_TAG, "Client device data last modified: " + clientsDataDelegate.getLastModifiedTimestamp());
          }

          
          
          
          
          final SkewHandler storageServerSkewHandler = SkewHandler.getSkewHandlerForHostname(storageHostname);
          final long storageServerSkew = storageServerSkewHandler.getSkewInSeconds();
          
          
          
          
          final boolean includePayloadVerificationHash = false;
          final AuthHeaderProvider authHeaderProvider = new HawkAuthHeaderProvider(token.id, token.key.getBytes("UTF-8"), includePayloadVerificationHash, storageServerSkew);

          final Context context = getContext();
          final SyncConfiguration syncConfig = new SyncConfiguration(token.uid, authHeaderProvider, sharedPrefs, syncKeyBundle);

          Collection<String> knownStageNames = SyncConfiguration.validEngineNames();
          syncConfig.stagesToSync = Utils.getStagesToSyncFromBundle(knownStageNames, extras);
          syncConfig.setClusterURL(storageServerURI);

          globalSession = new FxAccountGlobalSession(syncConfig, callback, context, clientsDataDelegate);
          globalSession.start();
        } catch (Exception e) {
          callback.handleError(globalSession, e);
          return;
        }
      }

      @Override
      public void handleFailure(TokenServerException e) {
        handleError(e);
      }

      @Override
      public void handleError(Exception e) {
        Logger.error(LOG_TAG, "Failed to get token.", e);
        callback.handleError(null, e);
      }

      @Override
      public void handleBackoff(int backoffSeconds) {
        
        Logger.info(LOG_TAG, "Token server requesting backoff of " + backoffSeconds + "s. Backoff handler: " + tokenBackoffHandler);
        didReceiveBackoff = true;

        
        
        tokenBackoffHandler.setEarliestNextRequest(delay(backoffSeconds * 1000));
      }

      private long delay(long delay) {
        return System.currentTimeMillis() + delay;
      }
    };

    TokenServerClient tokenServerclient = new TokenServerClient(tokenServerEndpointURI, executor);
    tokenServerclient.getTokenFromBrowserIDAssertion(assertion, true, clientState, delegate);
  }

  






  @Override
  public void onPerformSync(final Account account, final Bundle extras, final String authority, ContentProviderClient provider, final SyncResult syncResult) {
    Logger.setThreadLogTag(FxAccountConstants.GLOBAL_LOG_TAG);
    Logger.resetLogging();

    Logger.info(LOG_TAG, "Syncing FxAccount" +
        " account named like " + Utils.obfuscateEmail(account.name) +
        " for authority " + authority +
        " with instance " + this + ".");

    final EnumSet<FirefoxAccounts.SyncHint> syncHints = FirefoxAccounts.getHintsToSyncFromBundle(extras);
    FirefoxAccounts.logSyncHints(syncHints);

    
    if (this.lastSyncRealtimeMillis > 0L &&
        (this.lastSyncRealtimeMillis + MINIMUM_SYNC_DELAY_MILLIS) > SystemClock.elapsedRealtime()) {
      Logger.info(LOG_TAG, "Not syncing FxAccount " + Utils.obfuscateEmail(account.name) +
                           ": minimum interval not met.");
      return;
    }

    final Context context = getContext();
    final AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);
    if (FxAccountConstants.LOG_PERSONAL_INFORMATION) {
      fxAccount.dump();
    }

    
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        try {
          AccountPickler.pickle(fxAccount, FxAccountConstants.ACCOUNT_PICKLE_FILENAME);
        } catch (Exception e) {
          
          Logger.warn(LOG_TAG, "Got exception pickling current account details; ignoring.", e);
        }
      }
    });

    final CountDownLatch latch = new CountDownLatch(1);

    Collection<String> knownStageNames = SyncConfiguration.validEngineNames();
    Collection<String> stageNamesToSync = Utils.getStagesToSyncFromBundle(knownStageNames, extras);

    final SyncDelegate syncDelegate = new SyncDelegate(latch, syncResult, fxAccount, stageNamesToSync);

    try {
      final State state;
      try {
        state = fxAccount.getState();
      } catch (Exception e) {
        syncDelegate.handleError(e);
        return;
      }

      
      final SharedPreferences sharedPrefs = fxAccount.getSyncPrefs();

      final BackoffHandler backgroundBackoffHandler = new PrefsBackoffHandler(sharedPrefs, "background");
      final BackoffHandler rateLimitBackoffHandler = new PrefsBackoffHandler(sharedPrefs, "rate");

      
      final boolean isImmediate = (extras != null) &&
                                  (extras.getBoolean(ContentResolver.SYNC_EXTRAS_UPLOAD, false) ||
                                   extras.getBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, false));

      
      
      if (!isImmediate) {
        if (!shouldPerformSync(backgroundBackoffHandler, "background", extras)) {
          syncDelegate.rejectSync();
          return;
        }
      }

      
      if (!shouldPerformSync(rateLimitBackoffHandler, "rate", extras)) {
        syncDelegate.postponeSync(rateLimitBackoffHandler.delayMilliseconds());
        return;
      }

      final SchedulePolicy schedulePolicy = new FxAccountSchedulePolicy(context, fxAccount);

      
      
      schedulePolicy.configureBackoffMillisBeforeSyncing(rateLimitBackoffHandler, backgroundBackoffHandler);

      final String audience = fxAccount.getAudience();
      final String authServerEndpoint = fxAccount.getAccountServerURI();
      final String tokenServerEndpoint = fxAccount.getTokenServerURI();
      final URI tokenServerEndpointURI = new URI(tokenServerEndpoint);

      
      final FxAccountClient client = new FxAccountClient20(authServerEndpoint, executor);
      final FxAccountLoginStateMachine stateMachine = new FxAccountLoginStateMachine();
      stateMachine.advance(state, StateLabel.Married, new LoginStateMachineDelegate() {
        @Override
        public FxAccountClient getClient() {
          return client;
        }

        @Override
        public long getCertificateDurationInMilliseconds() {
          return 12 * 60 * 60 * 1000;
        }

        @Override
        public long getAssertionDurationInMilliseconds() {
          return 15 * 60 * 1000;
        }

        @Override
        public BrowserIDKeyPair generateKeyPair() throws NoSuchAlgorithmException {
          return StateFactory.generateKeyPair();
        }

        @Override
        public void handleTransition(Transition transition, State state) {
          Logger.info(LOG_TAG, "handleTransition: " + transition + " to " + state.getStateLabel());
        }

        private boolean shouldRequestToken(final BackoffHandler tokenBackoffHandler, final Bundle extras) {
          return shouldPerformSync(tokenBackoffHandler, "token", extras);
        }

        @Override
        public void handleFinal(State state) {
          Logger.info(LOG_TAG, "handleFinal: in " + state.getStateLabel());
          fxAccount.setState(state);
          schedulePolicy.onHandleFinal(state.getNeededAction());
          notificationManager.update(context, fxAccount);
          try {
            if (state.getStateLabel() != StateLabel.Married) {
              syncDelegate.handleCannotSync(state);
              return;
            }

            final Married married = (Married) state;
            final String assertion = married.generateAssertion(audience, JSONWebTokenUtils.DEFAULT_ASSERTION_ISSUER);

            















            
            
            
            
            final BackoffHandler tokenBackoffHandler = new PrefsBackoffHandler(sharedPrefs, "token");
            if (!shouldRequestToken(tokenBackoffHandler, extras)) {
              Logger.info(LOG_TAG, "Not syncing (token server).");
              syncDelegate.postponeSync(tokenBackoffHandler.delayMilliseconds());
              return;
            }

            final SessionCallback sessionCallback = new SessionCallback(syncDelegate, schedulePolicy);
            final KeyBundle syncKeyBundle = married.getSyncKeyBundle();
            final String clientState = married.getClientState();
            syncWithAssertion(audience, assertion, tokenServerEndpointURI, tokenBackoffHandler, sharedPrefs, syncKeyBundle, clientState, sessionCallback, extras);
          } catch (Exception e) {
            syncDelegate.handleError(e);
            return;
          }
        }
      });

      latch.await();
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Got error syncing.", e);
      syncDelegate.handleError(e);
    }

    Logger.info(LOG_TAG, "Syncing done.");
    lastSyncRealtimeMillis = SystemClock.elapsedRealtime();
  }
}
