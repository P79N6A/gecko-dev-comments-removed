



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.delegates.FreshStartDelegate;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;
import org.mozilla.gecko.sync.delegates.InfoCollectionsDelegate;
import org.mozilla.gecko.sync.delegates.KeyUploadDelegate;
import org.mozilla.gecko.sync.delegates.MetaGlobalDelegate;
import org.mozilla.gecko.sync.delegates.WipeServerDelegate;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.HttpResponseObserver;
import org.mozilla.gecko.sync.net.SyncResponse;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.stage.AndroidBrowserBookmarksServerSyncStage;
import org.mozilla.gecko.sync.stage.AndroidBrowserHistoryServerSyncStage;
import org.mozilla.gecko.sync.stage.CheckPreconditionsStage;
import org.mozilla.gecko.sync.stage.CompletedStage;
import org.mozilla.gecko.sync.stage.EnsureClusterURLStage;
import org.mozilla.gecko.sync.stage.EnsureCrypto5KeysStage;
import org.mozilla.gecko.sync.stage.FennecTabsServerSyncStage;
import org.mozilla.gecko.sync.stage.FetchInfoCollectionsStage;
import org.mozilla.gecko.sync.stage.FetchMetaGlobalStage;
import org.mozilla.gecko.sync.stage.FormHistoryServerSyncStage;
import org.mozilla.gecko.sync.stage.GlobalSyncStage;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;
import org.mozilla.gecko.sync.stage.NoSuchStageException;
import org.mozilla.gecko.sync.stage.PasswordsServerSyncStage;
import org.mozilla.gecko.sync.stage.SyncClientsEngineStage;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import ch.boye.httpclientandroidlib.HttpResponse;

public class GlobalSession implements CredentialsSource, PrefsSource, HttpResponseObserver {
  private static final String LOG_TAG = "GlobalSession";

  public static final String API_VERSION   = "1.1";
  public static final long STORAGE_VERSION = 5;

  public SyncConfiguration config = null;

  protected Map<Stage, GlobalSyncStage> stages;
  public Stage currentState = Stage.idle;

  public final GlobalSessionCallback callback;
  private Context context;
  private ClientsDataDelegate clientsDelegate;

  


  public KeyBundle keyBundleForCollection(String collection) throws NoCollectionKeysSetException {
    return config.getCollectionKeys().keyBundleForCollection(collection);
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
                       Bundle extras,
                       ClientsDataDelegate clientsDelegate)
                           throws SyncConfigurationException, IllegalArgumentException, IOException, ParseException, NonObjectJSONException {
    if (callback == null) {
      throw new IllegalArgumentException("Must provide a callback to GlobalSession constructor.");
    }

    if (anyInvalidStrings(username, password)) {
      throw new SyncConfigurationException();
    }

    Logger.info(LOG_TAG, "GlobalSession initialized with bundle " + extras);
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
    this.clientsDelegate = clientsDelegate;

    config = new SyncConfiguration(prefsPath, this);
    config.userAPI       = userAPI;
    config.serverURL     = serverURI;
    config.username      = username;
    config.password      = password;
    config.syncKeyBundle = syncKeyBundle;

    registerCommands();
    prepareStages();

    
  }

  protected void registerCommands() {
    CommandProcessor processor = CommandProcessor.getProcessor();

    processor.registerCommand("resetEngine", new CommandRunner() {
      @Override
      public void executeCommand(List<String> args) {
        HashSet<String> names = new HashSet<String>();
        names.add(args.get(0));
        resetStagesByName(names);
      }
    });

    processor.registerCommand("resetAll", new CommandRunner() {
      @Override
      public void executeCommand(List<String> args) {
        resetAllStages();
      }
    });

    processor.registerCommand("wipeEngine", new CommandRunner() {
      @Override
      public void executeCommand(List<String> args) {
        HashSet<String> names = new HashSet<String>();
        names.add(args.get(0));
        wipeStagesByName(names);
      }
    });

    processor.registerCommand("wipeAll", new CommandRunner() {
      @Override
      public void executeCommand(List<String> args) {
        wipeAllStages();
      }
    });

    processor.registerCommand("displayURI", new CommandRunner() {
      @Override
      public void executeCommand(List<String> args) {
        CommandProcessor.getProcessor().displayURI(args, getContext());
      }
    });
  }

  protected void prepareStages() {
    HashMap<Stage, GlobalSyncStage> stages = new HashMap<Stage, GlobalSyncStage>();

    stages.put(Stage.checkPreconditions,      new CheckPreconditionsStage(this));
    stages.put(Stage.ensureClusterURL,        new EnsureClusterURLStage(this));
    stages.put(Stage.fetchInfoCollections,    new FetchInfoCollectionsStage(this));
    stages.put(Stage.fetchMetaGlobal,         new FetchMetaGlobalStage(this));
    stages.put(Stage.ensureKeysStage,         new EnsureCrypto5KeysStage(this));
    stages.put(Stage.syncClientsEngine,       new SyncClientsEngineStage(this));

    stages.put(Stage.syncTabs,                new FennecTabsServerSyncStage(this));
    stages.put(Stage.syncPasswords,           new PasswordsServerSyncStage(this));
    stages.put(Stage.syncBookmarks,           new AndroidBrowserBookmarksServerSyncStage(this));
    stages.put(Stage.syncHistory,             new AndroidBrowserHistoryServerSyncStage(this));
    stages.put(Stage.syncFormHistory,         new FormHistoryServerSyncStage(this));

    stages.put(Stage.completed,               new CompletedStage(this));

    this.stages = Collections.unmodifiableMap(stages);
  }

  public GlobalSyncStage getSyncStageByName(String name) throws NoSuchStageException {
    return getSyncStageByName(Stage.byName(name));
  }

  public GlobalSyncStage getSyncStageByName(Stage next) throws NoSuchStageException {
    GlobalSyncStage stage = stages.get(next);
    if (stage == null) {
      throw new NoSuchStageException(next);
    }
    return stage;
  }

  public Collection<GlobalSyncStage> getSyncStagesByEnum(Collection<Stage> enums) {
    ArrayList<GlobalSyncStage> out = new ArrayList<GlobalSyncStage>();
    for (Stage name : enums) {
      try {
        GlobalSyncStage stage = this.getSyncStageByName(name);
        out.add(stage);
      } catch (NoSuchStageException e) {
        Logger.warn(LOG_TAG, "Unable to find stage with name " + name);
      }
    }
    return out;
  }

  public Collection<GlobalSyncStage> getSyncStagesByName(Collection<String> names) {
    ArrayList<GlobalSyncStage> out = new ArrayList<GlobalSyncStage>();
    for (String name : names) {
      try {
        GlobalSyncStage stage = this.getSyncStageByName(name);
        out.add(stage);
      } catch (NoSuchStageException e) {
        Logger.warn(LOG_TAG, "Unable to find stage with name " + name);
      }
    }
    return out;
  }

  





  public static Stage nextStage(Stage current) {
    int index = current.ordinal() + 1;
    int max   = Stage.completed.ordinal() + 1;
    return Stage.values()[index % max];
  }

  


  public void advance() {
    
    long existingBackoff = largestBackoffObserved.get();
    if (existingBackoff > 0) {
      this.abort(null, "Aborting sync because of backoff of " + existingBackoff + " milliseconds.");
      return;
    }

    this.callback.handleStageCompleted(this.currentState, this);
    Stage next = nextStage(this.currentState);
    GlobalSyncStage nextStage;
    try {
      nextStage = this.getSyncStageByName(next);
    } catch (NoSuchStageException e) {
      this.abort(e, "No such stage " + next);
      return;
    }
    this.currentState = next;
    Logger.info(LOG_TAG, "Running next stage " + next + " (" + nextStage + ")...");
    try {
      nextStage.execute();
    } catch (Exception ex) {
      Logger.warn(LOG_TAG, "Caught exception " + ex + " running stage " + next);
      this.abort(ex, "Uncaught exception in stage.");
      return;
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

  public Context getContext() {
    return this.context;
  }

  











  public void start() throws AlreadySyncingException {
    if (this.currentState != GlobalSyncStage.Stage.idle) {
      throw new AlreadySyncingException(this.currentState);
    }
    installAsHttpResponseObserver(); 
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
    uninstallAsHttpResponseObserver();
    this.currentState = GlobalSyncStage.Stage.idle;
    this.callback.handleSuccess(this);
  }

  public void abort(Exception e, String reason) {
    Logger.warn(LOG_TAG, "Aborting sync: " + reason, e);
    uninstallAsHttpResponseObserver();
    long existingBackoff = largestBackoffObserved.get();
    if (existingBackoff > 0) {
      callback.requestBackoff(existingBackoff);
    }
    this.callback.handleError(this, e);
  }

  public void handleHTTPError(SyncStorageResponse response, String reason) {
    
    
    Logger.warn(LOG_TAG, "Aborting sync due to HTTP " + response.getStatusCode());
    this.interpretHTTPFailure(response.httpResponse());
    this.abort(new HTTPFailureException(response), reason);
  }

  


  public void interpretHTTPFailure(HttpResponse response) {
    
    long responseBackoff = (new SyncResponse(response)).totalBackoffInMilliseconds();
    if (responseBackoff > 0) {
      callback.requestBackoff(responseBackoff);
    }

    if (response.getStatusLine() != null) {
      final int statusCode = response.getStatusLine().getStatusCode();
      switch(statusCode) {

      case 400:
        SyncStorageResponse storageResponse = new SyncStorageResponse(response);
        this.interpretHTTPBadRequestBody(storageResponse);
        break;

      case 401:
        




        callback.informUnauthorizedResponse(this, config.getClusterURL());
        break;
      }
    }
  }

  protected void interpretHTTPBadRequestBody(final SyncStorageResponse storageResponse) {
    try {
      final String body = storageResponse.body();
      if (body == null) {
        return;
      }
      if (SyncStorageResponse.RESPONSE_CLIENT_UPGRADE_REQUIRED.equals(body)) {
        callback.informUpgradeRequiredResponse(this);
        return;
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Exception parsing HTTP 400 body.", e);
    }
  }

  public void fetchInfoCollections(InfoCollectionsDelegate callback) throws URISyntaxException {
    if (this.config.infoCollections == null) {
      this.config.infoCollections = new InfoCollections(config.infoURL(), credentials());
    }
    this.config.infoCollections.fetch(callback);
  }

  







  public void uploadKeys(final CollectionKeys keys,
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
        BaseResource.consumeEntity(response); 
        keyUploadDelegate.onKeysUploaded();
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        self.interpretHTTPFailure(response.httpResponse());
        BaseResource.consumeEntity(response); 
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

    CryptoRecord keysRecord;
    try {
      keysRecord = keys.asCryptoRecord();
      keysRecord.setKeyBundle(config.syncKeyBundle);
      keysRecord.encrypt();
    } catch (UnsupportedEncodingException e) {
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    } catch (CryptoException e) {
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    } catch (NoCollectionKeysSetException e) {
      
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    }

    request.put(keysRecord);
  }

  


  public void processMetaGlobal(MetaGlobal global) {
    config.metaGlobal = global;

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
      
      resetAllStages();
      config.purgeCryptoKeys();
      config.syncID = remoteSyncID;
    }
    config.enabledEngineNames = global.getEnabledEngineNames();
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
          Logger.warn(LOG_TAG, "Got exception when restarting sync after freshStart.", e);
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
        session.resetAllStages();
        session.config.purgeCryptoKeys();
        session.config.persistToPrefs();

        MetaGlobal mg = new MetaGlobal(metaURL, credentials);
        mg.setSyncID(newSyncID);
        mg.setStorageVersion(STORAGE_VERSION);

        
        
        
        
        mg.upload(new MetaGlobalDelegate() {

          @Override
          public void handleSuccess(MetaGlobal global, SyncStorageResponse response) {
            session.config.metaGlobal = global;
            Logger.info(LOG_TAG, "New meta/global uploaded with sync ID " + newSyncID);

            
            try {
              session.uploadKeys(CollectionKeys.generateCollectionKeys(), new KeyUploadDelegate() {
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
            } catch (CryptoException e) {
              Log.e(LOG_TAG, "Got exception generating new keys.", e);
              freshStartDelegate.onFreshStartFailed(e);
            }
          }

          @Override
          public void handleMissing(MetaGlobal global, SyncStorageResponse response) {
            
            Logger.warn(LOG_TAG, "Got 'missing' response uploading new meta/global.");
            freshStartDelegate.onFreshStartFailed(new Exception("meta/global missing"));
          }

          @Override
          public void handleFailure(SyncStorageResponse response) {
            
            Logger.warn(LOG_TAG, "Got failure " + response.getStatusCode() + " uploading new meta/global.");
            session.interpretHTTPFailure(response.httpResponse());
            freshStartDelegate.onFreshStartFailed(new HTTPFailureException(response));
          }

          @Override
          public void handleError(Exception e) {
            Logger.warn(LOG_TAG, "Got error uploading new meta/global.", e);
            freshStartDelegate.onFreshStartFailed(e);
          }
        });
      }

      @Override
      public void onWipeFailed(Exception e) {
        Logger.warn(LOG_TAG, "Wipe failed.");
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
      Logger.warn(LOG_TAG, "Invalid URI in wipeServer.");
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
        BaseResource.consumeEntity(response);
        wipeDelegate.onWiped(response.normalizedWeaveTimestamp());
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        Logger.warn(LOG_TAG, "Got request failure " + response.getStatusCode() + " in wipeServer.");
        
        self.interpretHTTPFailure(response.httpResponse());
        BaseResource.consumeEntity(response); 
        wipeDelegate.onWipeFailed(new HTTPFailureException(response));
      }

      @Override
      public void handleRequestError(Exception ex) {
        Logger.warn(LOG_TAG, "Got exception in wipeServer.", ex);
        wipeDelegate.onWipeFailed(ex);
      }

      @Override
      public String credentials() {
        return credentials.credentials();
      }
    };
    request.delete();
  }

  public void wipeAllStages() {
    Logger.info(LOG_TAG, "Wiping all stages.");
    
    this.wipeStagesByEnum(Stage.getNamedStages());
  }

  public static void wipeStages(Collection<GlobalSyncStage> stages) {
    for (GlobalSyncStage stage : stages) {
      try {
        Logger.info(LOG_TAG, "Wiping " + stage);
        stage.wipeLocal();
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Ignoring wipe failure for stage " + stage, e);
      }
    }
  }

  public void wipeStagesByEnum(Collection<Stage> stages) {
    GlobalSession.wipeStages(this.getSyncStagesByEnum(stages));
  }

  public void wipeStagesByName(Collection<String> names) {
    GlobalSession.wipeStages(this.getSyncStagesByName(names));
  }

  public void resetAllStages() {
    Logger.info(LOG_TAG, "Resetting all stages.");
    
    this.resetStagesByEnum(Stage.getNamedStages());
  }

  public static void resetStages(Collection<GlobalSyncStage> stages) {
    for (GlobalSyncStage stage : stages) {
      try {
        Logger.info(LOG_TAG, "Resetting " + stage);
        stage.resetLocal();
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Ignoring reset failure for stage " + stage, e);
      }
    }
  }

  public void resetStagesByEnum(Collection<Stage> stages) {
    GlobalSession.resetStages(this.getSyncStagesByEnum(stages));
  }

  public void resetStagesByName(Collection<String> names) {
    Collection<GlobalSyncStage> stages = new ArrayList<GlobalSyncStage>();
    for (String name : names) {
      try {
        GlobalSyncStage stage = this.getSyncStageByName(name);
        stages.add(stage);
      } catch (NoSuchStageException e) {
        Logger.warn(LOG_TAG, "Cannot reset stage " + name + ": no such stage.");
      }
    }
    GlobalSession.resetStages(stages);
  }

  



  public void requiresUpgrade() {
    Logger.info(LOG_TAG, "Client outdated storage version; requires update.");
    
  }

  














  public boolean engineIsEnabled(String engineName, EngineSettings engineSettings) throws MetaGlobalException {
    
    if (this.config.enabledEngineNames == null) {
      Logger.error(LOG_TAG, "No enabled engines in config. Giving up.");
      if (this.config.metaGlobal == null) {
        throw new MetaGlobalNotSetException();
      }
      throw new MetaGlobalMissingEnginesException();
    }

    if (!(this.config.enabledEngineNames.contains(engineName))) {
      Logger.debug(LOG_TAG, "Engine " + engineName + " not enabled: no meta/global entry.");
      return false;
    }

    if (this.config.metaGlobal == null) {
      Logger.warn(LOG_TAG, "No meta/global; using historical enabled engine names.");
      return true;
    }

    
    
    if (engineSettings != null) {
      
      this.config.metaGlobal.verifyEngineSettings(engineName, engineSettings);
    }

    return true;
  }

  public ClientsDataDelegate getClientsDelegate() {
    return this.clientsDelegate;
  }

  


  protected final AtomicLong largestBackoffObserved = new AtomicLong(-1);

  



  protected void installAsHttpResponseObserver() {
    Logger.debug(LOG_TAG, "Installing " + this + " as BaseResource HttpResponseObserver.");
    BaseResource.setHttpResponseObserver(this);
    largestBackoffObserved.set(-1);
  }

  


  protected void uninstallAsHttpResponseObserver() {
    Logger.debug(LOG_TAG, "Uninstalling " + this + " as BaseResource HttpResponseObserver.");
    BaseResource.setHttpResponseObserver(null);
  }

  


  @Override
  public void observeHttpResponse(HttpResponse response) {
    long responseBackoff = (new SyncResponse(response)).totalBackoffInMilliseconds(); 
    if (responseBackoff <= 0) {
      return;
    }

    Logger.debug(LOG_TAG, "Observed " + responseBackoff + " millisecond backoff request.");
    while (true) {
      long existingBackoff = largestBackoffObserved.get();
      if (existingBackoff >= responseBackoff) {
        return;
      }
      if (largestBackoffObserved.compareAndSet(existingBackoff, responseBackoff)) {
        return;
      }
    }
  }
}
