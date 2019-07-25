





































package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.Map;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.FreshStartDelegate;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;
import org.mozilla.gecko.sync.delegates.InfoCollectionsDelegate;
import org.mozilla.gecko.sync.delegates.KeyUploadDelegate;
import org.mozilla.gecko.sync.delegates.MetaGlobalDelegate;
import org.mozilla.gecko.sync.delegates.WipeServerDelegate;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;
import org.mozilla.gecko.sync.stage.AndroidBrowserBookmarksServerSyncStage;
import org.mozilla.gecko.sync.stage.AndroidBrowserHistoryServerSyncStage;
import org.mozilla.gecko.sync.stage.CheckPreconditionsStage;
import org.mozilla.gecko.sync.stage.CompletedStage;
import org.mozilla.gecko.sync.stage.EnsureClusterURLStage;
import org.mozilla.gecko.sync.stage.EnsureKeysStage;
import org.mozilla.gecko.sync.stage.FetchInfoCollectionsStage;
import org.mozilla.gecko.sync.stage.FetchMetaGlobalStage;
import org.mozilla.gecko.sync.stage.GlobalSyncStage;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;
import org.mozilla.gecko.sync.stage.NoSuchStageException;

import ch.boye.httpclientandroidlib.HttpResponse;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

public class GlobalSession implements CredentialsSource, PrefsSource {
  private static final String LOG_TAG = "GlobalSession";

  public static final String API_VERSION   = "1.1";
  public static final long STORAGE_VERSION = 5;

  private static final String HEADER_RETRY_AFTER     = "retry-after";
  private static final String HEADER_X_WEAVE_BACKOFF = "x-weave-backoff";

  public SyncConfiguration config = null;

  protected Map<Stage, GlobalSyncStage> stages;
  public Stage currentState = Stage.idle;

  private GlobalSessionCallback callback;
  private Context context;

  


  public void setCollectionKeys(CollectionKeys k) {
    config.setCollectionKeys(k);
  }
  @Override
  public CollectionKeys getCollectionKeys() {
    return config.collectionKeys;
  }
  @Override
  public KeyBundle keyForCollection(String collection) throws NoCollectionKeysSetException {
    return config.keyForCollection(collection);
  }

  


  @Override
  public String credentials() {
    return config.credentials();
  }

  public URI wboURI(String collection, String id) throws URISyntaxException {
    return config.wboURI(collection, id);
  }

  


  private static boolean isInvalidString(String s) {
    return s == null ||
           s.trim().length() == 0;
  }

  private static boolean anyInvalidStrings(String s, String...strings) {
    if (isInvalidString(s)) {
      return true;
    }
    for (String str : strings) {
      if (isInvalidString(str)) {
        return true;
      }
    }
    return false;
  }

  public GlobalSession(String userAPI,
                       String serverURL,
                       String username,
                       String password,
                       String prefsPath,
                       KeyBundle syncKeyBundle,
                       GlobalSessionCallback callback,
                       Context context,
                       Bundle persisted)
                           throws SyncConfigurationException, IllegalArgumentException, IOException, ParseException, NonObjectJSONException {
    if (callback == null) {
      throw new IllegalArgumentException("Must provide a callback to GlobalSession constructor.");
    }

    if (anyInvalidStrings(username, password)) {
      throw new SyncConfigurationException();
    }

    Log.i(LOG_TAG, "GlobalSession initialized with bundle " + persisted);
    URI serverURI;
    try {
      serverURI = (serverURL == null) ? null : new URI(serverURL);
    } catch (URISyntaxException e) {
      throw new SyncConfigurationException();
    }

    if (syncKeyBundle == null ||
        syncKeyBundle.getEncryptionKey() == null ||
        syncKeyBundle.getHMACKey() == null) {
      throw new SyncConfigurationException();
    }

    this.callback        = callback;
    this.context         = context;

    config = new SyncConfiguration(prefsPath, this);
    config.userAPI       = userAPI;
    config.serverURL     = serverURI;
    config.username      = username;
    config.password      = password;
    config.syncKeyBundle = syncKeyBundle;
    

    
    this.synchronizerConfigurations = new SynchronizerConfigurations(persisted);
    prepareStages();
  }

  protected void prepareStages() {
    stages = new HashMap<Stage, GlobalSyncStage>();
    stages.put(Stage.checkPreconditions,      new CheckPreconditionsStage());
    stages.put(Stage.ensureClusterURL,        new EnsureClusterURLStage());
    stages.put(Stage.fetchInfoCollections,    new FetchInfoCollectionsStage());
    stages.put(Stage.fetchMetaGlobal,         new FetchMetaGlobalStage());
    stages.put(Stage.ensureKeysStage,         new EnsureKeysStage());

    
    stages.put(Stage.syncBookmarks,           new AndroidBrowserBookmarksServerSyncStage());
    stages.put(Stage.syncHistory,             new AndroidBrowserHistoryServerSyncStage());
    stages.put(Stage.completed,               new CompletedStage());
  }

  protected GlobalSyncStage getStageByName(Stage next) throws NoSuchStageException {
    GlobalSyncStage stage = stages.get(next);
    if (stage == null) {
      throw new NoSuchStageException(next);
    }
    return stage;
  }

  




  public static Stage nextStage(Stage current) {
    int index = current.ordinal() + 1;
    int max   = Stage.completed.ordinal() + 1;
    return Stage.values()[index % max];
  }

  





  public void advance() {
    this.callback.handleStageCompleted(this.currentState, this);
    Stage next = nextStage(this.currentState);
    GlobalSyncStage nextStage;
    try {
      nextStage = this.getStageByName(next);
    } catch (NoSuchStageException e) {
      this.abort(e, "No such stage " + next);
      return;
    }
    this.currentState = next;
    Log.i(LOG_TAG, "Running next stage " + next + " (" + nextStage + ")...");
    try {
      nextStage.execute(this);
    } catch (Exception ex) {
      Log.w(LOG_TAG, "Caught exception " + ex + " running stage " + next);
      this.abort(ex, "Uncaught exception in stage.");
    }
  }

  private String getSyncID() {
    return config.syncID;
  }

  private String generateSyncID() {
    config.syncID = Utils.generateGuid();
    return config.syncID;
  }

  


  @Override
  public SharedPreferences getPrefs(String name, int mode) {
    return this.getContext().getSharedPreferences(name, mode);
  }

  @Override
  public Context getContext() {
    return this.context;
  }

  











  public void start() throws AlreadySyncingException {
    if (this.currentState != GlobalSyncStage.Stage.idle) {
      throw new AlreadySyncingException(this.currentState);
    }
    this.advance();
  }

  



  protected void restart() throws AlreadySyncingException {
    this.currentState = GlobalSyncStage.Stage.idle;
    if (callback.shouldBackOff()) {
      this.callback.handleAborted(this, "Told to back off.");
      return;
    }
    this.start();
  }

  public void completeSync() {
    this.currentState = GlobalSyncStage.Stage.idle;
    this.callback.handleSuccess(this);
  }

  public void abort(Exception e, String reason) {
    Log.w(LOG_TAG, "Aborting sync: " + reason, e);
    this.callback.handleError(this, e);
  }

  public void handleHTTPError(SyncStorageResponse response, String reason) {
    
    
    Log.w(LOG_TAG, "Aborting sync due to HTTP " + response.getStatusCode());
    this.interpretHTTPFailure(response.httpResponse());
    this.abort(new HTTPFailureException(response), reason);
  }

  


  public void interpretHTTPFailure(HttpResponse response) {
    
    long retryAfter = 0;
    long weaveBackoff = 0;
    if (response.containsHeader(HEADER_RETRY_AFTER)) {
      
      String headerValue = response.getFirstHeader(HEADER_RETRY_AFTER).getValue();
      retryAfter = Utils.decimalSecondsToMilliseconds(headerValue);
    }
    if (response.containsHeader(HEADER_X_WEAVE_BACKOFF)) {
      
      String headerValue = response.getFirstHeader(HEADER_X_WEAVE_BACKOFF).getValue();
      weaveBackoff = Utils.decimalSecondsToMilliseconds(headerValue);
    }
    long backoff = Math.max(retryAfter, weaveBackoff);
    if (backoff > 0) {
      callback.requestBackoff(backoff);
    }
  }


  public void fetchMetaGlobal(MetaGlobalDelegate callback) throws URISyntaxException {
    if (this.config.metaGlobal == null) {
      this.config.metaGlobal = new MetaGlobal(config.metaURL(), credentials());
    }
    this.config.metaGlobal.fetch(callback);
  }

  public void fetchInfoCollections(InfoCollectionsDelegate callback) throws URISyntaxException {
    if (this.config.infoCollections == null) {
      this.config.infoCollections = new InfoCollections(config.infoURL(), credentials());
    }
    this.config.infoCollections.fetch(callback);
  }

  public void uploadKeys(CryptoRecord keysRecord,
                         final KeyUploadDelegate keyUploadDelegate) {
    SyncStorageRecordRequest request;
    final GlobalSession self = this;
    try {
      request = new SyncStorageRecordRequest(this.config.keysURI());
    } catch (URISyntaxException e) {
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    }

    request.delegate = new SyncStorageRequestDelegate() {

      @Override
      public String ifUnmodifiedSince() {
        return null;
      }

      @Override
      public void handleRequestSuccess(SyncStorageResponse response) {
        keyUploadDelegate.onKeysUploaded();
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        self.interpretHTTPFailure(response.httpResponse());
        keyUploadDelegate.onKeyUploadFailed(new HTTPFailureException(response));
      }

      @Override
      public void handleRequestError(Exception ex) {
        keyUploadDelegate.onKeyUploadFailed(ex);
      }

      @Override
      public String credentials() {
        return self.credentials();
      }
    };

    keysRecord.setKeyBundle(config.syncKeyBundle);
    try {
      keysRecord.encrypt();
    } catch (UnsupportedEncodingException e) {
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    } catch (CryptoException e) {
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    }
    request.put(keysRecord);
  }


  


  public void processMetaGlobal(MetaGlobal global) {
    Long storageVersion = global.getStorageVersion();
    if (storageVersion < STORAGE_VERSION) {
      
      freshStart();
      return;
    }
    if (storageVersion > STORAGE_VERSION) {
      
      requiresUpgrade();
      return;
    }
    String remoteSyncID = global.getSyncID();
    if (remoteSyncID == null) {
      
      freshStart();
      return;
    }
    String localSyncID = this.getSyncID();
    if (!remoteSyncID.equals(localSyncID)) {
      
      resetClient();
      if (config.collectionKeys != null) {
        config.collectionKeys.clear();
      }
      config.syncID = remoteSyncID;
      
    }
    config.persistToPrefs();
    advance();
  }

  public void processMissingMetaGlobal(MetaGlobal global) {
    freshStart();
  }

  


  protected void freshStart() {
    final GlobalSession globalSession = this;
    freshStart(this, new FreshStartDelegate() {

      @Override
      public void onFreshStartFailed(Exception e) {
        globalSession.abort(e, "Fresh start failed.");
      }

      @Override
      public void onFreshStart() {
        try {
          globalSession.config.persistToPrefs();
          globalSession.restart();
        } catch (Exception e) {
          Log.w(LOG_TAG, "Got exception when restarting sync after freshStart.", e);
          globalSession.abort(e, "Got exception after freshStart.");
        }
      }
    });
  }

  


  protected void freshStart(final GlobalSession session, final FreshStartDelegate freshStartDelegate) {

    final String newSyncID   = session.generateSyncID();
    final String metaURL     = session.config.metaURL();
    final String credentials = session.credentials();

    wipeServer(session, new WipeServerDelegate() {

      @Override
      public void onWiped(long timestamp) {
        session.resetClient();
        session.config.collectionKeys.clear();      
        session.config.persistToPrefs();

        MetaGlobal mg = new MetaGlobal(metaURL, credentials);
        mg.setSyncID(newSyncID);
        mg.setStorageVersion(STORAGE_VERSION);

        
        
        
        
        mg.upload(new MetaGlobalDelegate() {

          @Override
          public void handleSuccess(MetaGlobal global, SyncStorageResponse response) {
            session.config.metaGlobal = global;
            Log.i(LOG_TAG, "New meta/global uploaded with sync ID " + newSyncID);

            
            try {
              session.uploadKeys(CollectionKeys.generateCollectionKeys().asCryptoRecord(), new KeyUploadDelegate() {
                @Override
                public void onKeysUploaded() {
                  
                  freshStartDelegate.onFreshStart();
                }

                @Override
                public void onKeyUploadFailed(Exception e) {
                  Log.e(LOG_TAG, "Got exception uploading new keys.", e);
                  freshStartDelegate.onFreshStartFailed(e);
                }
              });
            } catch (NoCollectionKeysSetException e) {
              Log.e(LOG_TAG, "Got exception generating new keys.", e);
              freshStartDelegate.onFreshStartFailed(e);
            } catch (CryptoException e) {
              Log.e(LOG_TAG, "Got exception generating new keys.", e);
              freshStartDelegate.onFreshStartFailed(e);
            }
          }

          @Override
          public void handleMissing(MetaGlobal global, SyncStorageResponse response) {
            
            Log.w(LOG_TAG, "Got 'missing' response uploading new meta/global.");
            freshStartDelegate.onFreshStartFailed(new Exception("meta/global missing"));
          }

          @Override
          public void handleFailure(SyncStorageResponse response) {
            
            Log.w(LOG_TAG, "Got failure " + response.getStatusCode() + " uploading new meta/global.");
            session.interpretHTTPFailure(response.httpResponse());
            freshStartDelegate.onFreshStartFailed(new HTTPFailureException(response));
          }

          @Override
          public void handleError(Exception e) {
            Log.w(LOG_TAG, "Got error uploading new meta/global.", e);
            freshStartDelegate.onFreshStartFailed(e);
          }

          @Override
          public MetaGlobalDelegate deferred() {
            final MetaGlobalDelegate self = this;
            return new MetaGlobalDelegate() {

              @Override
              public void handleSuccess(final MetaGlobal global, final SyncStorageResponse response) {
                ThreadPool.run(new Runnable() {
                  @Override
                  public void run() {
                    self.handleSuccess(global, response);
                  }});
              }

              @Override
              public void handleMissing(final MetaGlobal global, final SyncStorageResponse response) {
                ThreadPool.run(new Runnable() {
                  @Override
                  public void run() {
                    self.handleMissing(global, response);
                  }});
              }

              @Override
              public void handleFailure(final SyncStorageResponse response) {
                ThreadPool.run(new Runnable() {
                  @Override
                  public void run() {
                    self.handleFailure(response);
                  }});
              }

              @Override
              public void handleError(final Exception e) {
                ThreadPool.run(new Runnable() {
                  @Override
                  public void run() {
                    self.handleError(e);
                  }});
              }

              @Override
              public MetaGlobalDelegate deferred() {
                return this;
              }
            };
          }
        });
      }

      @Override
      public void onWipeFailed(Exception e) {
        Log.w(LOG_TAG, "Wipe failed.");
        freshStartDelegate.onFreshStartFailed(e);
      }
    });

  }

  private void wipeServer(final CredentialsSource credentials, final WipeServerDelegate wipeDelegate) {
    SyncStorageRequest request;
    final GlobalSession self = this;

    try {
      request = new SyncStorageRequest(config.storageURL(false));
    } catch (URISyntaxException ex) {
      Log.w(LOG_TAG, "Invalid URI in wipeServer.");
      wipeDelegate.onWipeFailed(ex);
      return;
    }

    request.delegate = new SyncStorageRequestDelegate() {

      @Override
      public String ifUnmodifiedSince() {
        return null;
      }

      @Override
      public void handleRequestSuccess(SyncStorageResponse response) {
        wipeDelegate.onWiped(response.normalizedWeaveTimestamp());
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        Log.w(LOG_TAG, "Got request failure " + response.getStatusCode() + " in wipeServer.");
        
        self.interpretHTTPFailure(response.httpResponse());
        wipeDelegate.onWipeFailed(new HTTPFailureException(response));
      }

      @Override
      public void handleRequestError(Exception ex) {
        Log.w(LOG_TAG, "Got exception in wipeServer.", ex);
        wipeDelegate.onWipeFailed(ex);
      }

      @Override
      public String credentials() {
        return credentials.credentials();
      }
    };
    request.delete();
  }

  



  private void resetClient() {
    
    

  }

  



  public void requiresUpgrade() {
    Log.i(LOG_TAG, "Client outdated storage version; requires update.");
    
  }

  








  public boolean engineIsEnabled(String engineName) throws MetaGlobalException {
    if (this.config.metaGlobal == null) {
      throw new MetaGlobalNotSetException();
    }
    if (this.config.metaGlobal.engines == null) {
      throw new MetaGlobalMissingEnginesException();
    }
    return this.config.metaGlobal.engines.get(engineName) != null;
  }

  





  public SynchronizerConfiguration configForEngine(String engineName) {
    
    SynchronizerConfiguration stored = this.getSynchronizerConfigurations().forEngine(engineName);
    if (stored == null) {
      return new SynchronizerConfiguration(engineName, new RepositorySessionBundle(0), new RepositorySessionBundle(0));
    }
    return stored;
  }
  private SynchronizerConfigurations synchronizerConfigurations;
  private SynchronizerConfigurations getSynchronizerConfigurations() {
    return this.synchronizerConfigurations;
  }
}
