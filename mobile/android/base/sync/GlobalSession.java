



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;

import org.json.simple.JSONArray;
import org.json.simple.parser.ParseException;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.BaseGlobalSessionCallback;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.delegates.FreshStartDelegate;
import org.mozilla.gecko.sync.delegates.JSONRecordFetchDelegate;
import org.mozilla.gecko.sync.delegates.KeyUploadDelegate;
import org.mozilla.gecko.sync.delegates.MetaGlobalDelegate;
import org.mozilla.gecko.sync.delegates.NodeAssignmentCallback;
import org.mozilla.gecko.sync.delegates.WipeServerDelegate;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
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
import org.mozilla.gecko.sync.stage.UploadMetaGlobalStage;

import android.content.Context;
import ch.boye.httpclientandroidlib.HttpResponse;

public class GlobalSession implements HttpResponseObserver {
  private static final String LOG_TAG = "GlobalSession";

  public static final long STORAGE_VERSION = 5;

  public SyncConfiguration config = null;

  protected Map<Stage, GlobalSyncStage> stages;
  public Stage currentState = Stage.idle;

  public final BaseGlobalSessionCallback callback;
  protected final Context context;
  protected final ClientsDataDelegate clientsDelegate;
  protected final NodeAssignmentCallback nodeAssignmentCallback;

  



  public final Map<String, EngineSettings> enginesToUpdate = new HashMap<String, EngineSettings>();

   


  public KeyBundle keyBundleForCollection(String collection) throws NoCollectionKeysSetException {
    return config.getCollectionKeys().keyBundleForCollection(collection);
  }

  


  public AuthHeaderProvider getAuthHeaderProvider() {
    return config.getAuthHeaderProvider();
  }

  public URI wboURI(String collection, String id) throws URISyntaxException {
    return config.wboURI(collection, id);
  }

  public GlobalSession(SyncConfiguration config,
                       BaseGlobalSessionCallback callback,
                       Context context,
                       ClientsDataDelegate clientsDelegate, NodeAssignmentCallback nodeAssignmentCallback)
    throws SyncConfigurationException, IllegalArgumentException, IOException, ParseException, NonObjectJSONException {

    if (callback == null) {
      throw new IllegalArgumentException("Must provide a callback to GlobalSession constructor.");
    }

    this.callback        = callback;
    this.context         = context;
    this.clientsDelegate = clientsDelegate;
    this.nodeAssignmentCallback = nodeAssignmentCallback;

    this.config = config;
    registerCommands();
    prepareStages();

    if (config.stagesToSync == null) {
      Logger.info(LOG_TAG, "No stages to sync specified; defaulting to all valid engine names.");
      config.stagesToSync = Collections.unmodifiableCollection(SyncConfiguration.validEngineNames());
    }

    
  }

  




  protected static void registerCommands() {
    final CommandProcessor processor = CommandProcessor.getProcessor();

    processor.registerCommand("resetEngine", new CommandRunner(1) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        HashSet<String> names = new HashSet<String>();
        names.add(args.get(0));
        session.resetStagesByName(names);
      }
    });

    processor.registerCommand("resetAll", new CommandRunner(0) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        session.resetAllStages();
      }
    });

    processor.registerCommand("wipeEngine", new CommandRunner(1) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        HashSet<String> names = new HashSet<String>();
        names.add(args.get(0));
        session.wipeStagesByName(names);
      }
    });

    processor.registerCommand("wipeAll", new CommandRunner(0) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        session.wipeAllStages();
      }
    });

    processor.registerCommand("displayURI", new CommandRunner(3) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        CommandProcessor.displayURI(args, session.getContext());
      }
    });
  }

  protected void prepareStages() {
    HashMap<Stage, GlobalSyncStage> stages = new HashMap<Stage, GlobalSyncStage>();

    stages.put(Stage.checkPreconditions,      new CheckPreconditionsStage());
    stages.put(Stage.ensureClusterURL,        new EnsureClusterURLStage(nodeAssignmentCallback));
    stages.put(Stage.fetchInfoCollections,    new FetchInfoCollectionsStage());
    stages.put(Stage.fetchMetaGlobal,         new FetchMetaGlobalStage());
    stages.put(Stage.ensureKeysStage,         new EnsureCrypto5KeysStage());
    stages.put(Stage.syncClientsEngine,       new SyncClientsEngineStage());

    stages.put(Stage.syncTabs,                new FennecTabsServerSyncStage());
    stages.put(Stage.syncPasswords,           new PasswordsServerSyncStage());
    stages.put(Stage.syncBookmarks,           new AndroidBrowserBookmarksServerSyncStage());
    stages.put(Stage.syncHistory,             new AndroidBrowserHistoryServerSyncStage());
    stages.put(Stage.syncFormHistory,         new FormHistoryServerSyncStage());

    stages.put(Stage.uploadMetaGlobal,        new UploadMetaGlobalStage());
    stages.put(Stage.completed,               new CompletedStage());

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
      nextStage.execute(this);
    } catch (Exception ex) {
      Logger.warn(LOG_TAG, "Caught exception " + ex + " running stage " + next);
      this.abort(ex, "Uncaught exception in stage.");
      return;
    }
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
    if (callback.shouldBackOffStorage()) {
      this.callback.handleAborted(this, "Told to back off.");
      return;
    }
    this.start();
  }

  


  protected void cleanUp() {
    uninstallAsHttpResponseObserver();
    this.stages = null;
  }

  public void completeSync() {
    cleanUp();
    this.currentState = GlobalSyncStage.Stage.idle;
    this.callback.handleSuccess(this);
  }

  






  public void recordForMetaGlobalUpdate(String engineName, EngineSettings engineSettings) {
    enginesToUpdate.put(engineName, engineSettings);
  }

  






  public void removeEngineFromMetaGlobal(String engineName) {
    enginesToUpdate.put(engineName, null);
  }

  public boolean hasUpdatedMetaGlobal() {
    if (enginesToUpdate.isEmpty()) {
      Logger.info(LOG_TAG, "Not uploading updated meta/global record since there are no engines requesting upload.");
      return false;
    }

    if (Logger.shouldLogVerbose(LOG_TAG)) {
      Logger.trace(LOG_TAG, "Uploading updated meta/global record since there are engine changes to meta/global.");
      Logger.trace(LOG_TAG, "Engines requesting update [" + Utils.toCommaSeparatedString(enginesToUpdate.keySet()) + "]");
    }

    return true;
  }

  public void updateMetaGlobalInPlace() {
    config.metaGlobal.declined = this.declinedEngineNames();
    ExtendedJSONObject engines = config.metaGlobal.getEngines();
    for (Entry<String, EngineSettings> pair : enginesToUpdate.entrySet()) {
      if (pair.getValue() == null) {
        engines.remove(pair.getKey());
      } else {
        engines.put(pair.getKey(), pair.getValue().toJSONObject());
      }
    }

    enginesToUpdate.clear();
  }

  




  public void uploadUpdatedMetaGlobal() {
    updateMetaGlobalInPlace();

    Logger.debug(LOG_TAG, "Uploading updated meta/global record.");
    final Object monitor = new Object();

    Runnable doUpload = new Runnable() {
      @Override
      public void run() {
        config.metaGlobal.upload(new MetaGlobalDelegate() {
          @Override
          public void handleSuccess(MetaGlobal global, SyncStorageResponse response) {
            Logger.info(LOG_TAG, "Successfully uploaded updated meta/global record.");
            
            config.enabledEngineNames = config.metaGlobal.getEnabledEngineNames();
            
            config.userSelectedEngines = null;

            synchronized (monitor) {
              monitor.notify();
            }
          }

          @Override
          public void handleMissing(MetaGlobal global, SyncStorageResponse response) {
            Logger.warn(LOG_TAG, "Got 404 missing uploading updated meta/global record; shouldn't happen.  Ignoring.");
            synchronized (monitor) {
              monitor.notify();
            }
          }

          @Override
          public void handleFailure(SyncStorageResponse response) {
            Logger.warn(LOG_TAG, "Failed to upload updated meta/global record; ignoring.");
            synchronized (monitor) {
              monitor.notify();
            }
          }

          @Override
          public void handleError(Exception e) {
            Logger.warn(LOG_TAG, "Got exception trying to upload updated meta/global record; ignoring.", e);
            synchronized (monitor) {
              monitor.notify();
            }
          }
        });
      }
    };

    final Thread upload = new Thread(doUpload);
    synchronized (monitor) {
      try {
        upload.start();
        monitor.wait();
        Logger.debug(LOG_TAG, "Uploaded updated meta/global record.");
      } catch (InterruptedException e) {
        Logger.error(LOG_TAG, "Uploading updated meta/global interrupted; continuing.");
      }
    }
  }


  public void abort(Exception e, String reason) {
    Logger.warn(LOG_TAG, "Aborting sync: " + reason, e);
    cleanUp();
    long existingBackoff = largestBackoffObserved.get();
    if (existingBackoff > 0) {
      callback.requestBackoff(existingBackoff);
    }
    if (!(e instanceof HTTPFailureException)) {
      
      if (this.hasUpdatedMetaGlobal()) {
        this.uploadUpdatedMetaGlobal(); 
      }
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

  public void fetchInfoCollections(JSONRecordFetchDelegate callback) throws URISyntaxException {
    final JSONRecordFetcher fetcher = new JSONRecordFetcher(config.infoCollectionsURL(), getAuthHeaderProvider());
    fetcher.fetch(callback);
  }

  







  public void uploadKeys(final CollectionKeys keys,
                         final KeyUploadDelegate keyUploadDelegate) {
    SyncStorageRecordRequest request;
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
        Logger.debug(LOG_TAG, "Keys uploaded.");
        BaseResource.consumeEntity(response); 
        keyUploadDelegate.onKeysUploaded();
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        Logger.debug(LOG_TAG, "Failed to upload keys.");
        GlobalSession.this.interpretHTTPFailure(response.httpResponse());
        BaseResource.consumeEntity(response); 
        keyUploadDelegate.onKeyUploadFailed(new HTTPFailureException(response));
      }

      @Override
      public void handleRequestError(Exception ex) {
        Logger.warn(LOG_TAG, "Got exception trying to upload keys", ex);
        keyUploadDelegate.onKeyUploadFailed(ex);
      }

      @Override
      public AuthHeaderProvider getAuthHeaderProvider() {
        return GlobalSession.this.getAuthHeaderProvider();
      }
    };

    
    CryptoRecord keysRecord;
    try {
      keysRecord = keys.asCryptoRecord();
      keysRecord.setKeyBundle(config.syncKeyBundle);
      keysRecord.encrypt();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception trying creating crypto record from keys", e);
      keyUploadDelegate.onKeyUploadFailed(e);
      return;
    }

    request.put(keysRecord);
  }

  


  public void processMetaGlobal(MetaGlobal global) {
    config.metaGlobal = global;

    Long storageVersion = global.getStorageVersion();
    if (storageVersion == null) {
      Logger.warn(LOG_TAG, "Malformed remote meta/global: could not retrieve remote storage version.");
      freshStart();
      return;
    }
    if (storageVersion < STORAGE_VERSION) {
      Logger.warn(LOG_TAG, "Outdated server: reported " +
          "remote storage version " + storageVersion + " < " +
          "local storage version " + STORAGE_VERSION);
      freshStart();
      return;
    }
    if (storageVersion > STORAGE_VERSION) {
      Logger.warn(LOG_TAG, "Outdated client: reported " +
          "remote storage version " + storageVersion + " > " +
          "local storage version " + STORAGE_VERSION);
      requiresUpgrade();
      return;
    }
    String remoteSyncID = global.getSyncID();
    if (remoteSyncID == null) {
      Logger.warn(LOG_TAG, "Malformed remote meta/global: could not retrieve remote syncID.");
      freshStart();
      return;
    }
    String localSyncID = config.syncID;
    if (!remoteSyncID.equals(localSyncID)) {
      Logger.warn(LOG_TAG, "Remote syncID different from local syncID: resetting client and assuming remote syncID.");
      resetAllStages();
      config.purgeCryptoKeys();
      config.syncID = remoteSyncID;
    }
    
    Logger.debug(LOG_TAG, "Comparing local engine selection timestamp [" + config.userSelectedEnginesTimestamp + "] to server meta/global timestamp [" + config.persistedMetaGlobal().lastModified() + "].");
    if (config.userSelectedEnginesTimestamp < config.persistedMetaGlobal().lastModified()) {
      
      config.userSelectedEngines = null;
    }
    
    config.enabledEngineNames = global.getEnabledEngineNames();
    if (config.enabledEngineNames == null) {
      Logger.warn(LOG_TAG, "meta/global reported no enabled engine names!");
    } else {
      if (Logger.shouldLogVerbose(LOG_TAG)) {
        Logger.trace(LOG_TAG, "Persisting enabled engine names '" +
            Utils.toCommaSeparatedString(config.enabledEngineNames) + "' from meta/global.");
      }
    }

    
    
    
    
    
    final HashSet<String> allDeclined = new HashSet<String>();

    final Set<String> newRemoteDeclined = global.getDeclinedEngineNames();
    final Set<String> oldLocalDeclined = config.declinedEngineNames;

    allDeclined.addAll(newRemoteDeclined);
    allDeclined.addAll(oldLocalDeclined);

    if (config.userSelectedEngines != null) {
      for (Entry<String, Boolean> selection : config.userSelectedEngines.entrySet()) {
        if (selection.getValue()) {
          allDeclined.remove(selection.getKey());
        }
      }
    }

    config.declinedEngineNames = allDeclined;
    if (config.declinedEngineNames.isEmpty()) {
      Logger.debug(LOG_TAG, "meta/global reported no declined engine names, and we have none declined locally.");
    } else {
      if (Logger.shouldLogVerbose(LOG_TAG)) {
        Logger.trace(LOG_TAG, "Persisting declined engine names '" +
            Utils.toCommaSeparatedString(config.declinedEngineNames) + "' from meta/global.");
      }
    }

    config.persistToPrefs();
    advance();
  }

  public void processMissingMetaGlobal(MetaGlobal global) {
    freshStart();
  }

  


  public void freshStart() {
    final GlobalSession globalSession = this;
    freshStart(this, new FreshStartDelegate() {

      @Override
      public void onFreshStartFailed(Exception e) {
        globalSession.abort(e, "Fresh start failed.");
      }

      @Override
      public void onFreshStart() {
        try {
          Logger.warn(LOG_TAG, "Fresh start succeeded; restarting global session.");
          globalSession.config.persistToPrefs();
          globalSession.restart();
        } catch (Exception e) {
          Logger.warn(LOG_TAG, "Got exception when restarting sync after freshStart.", e);
          globalSession.abort(e, "Got exception after freshStart.");
        }
      }
    });
  }

  












  protected static void freshStart(final GlobalSession session, final FreshStartDelegate freshStartDelegate) {
    Logger.debug(LOG_TAG, "Fresh starting.");

    final MetaGlobal mg = session.generateNewMetaGlobal();

    session.wipeServer(session.getAuthHeaderProvider(), new WipeServerDelegate() {

      @Override
      public void onWiped(long timestamp) {
        Logger.debug(LOG_TAG, "Successfully wiped server.  Resetting all stages and purging cached meta/global and crypto/keys records.");

        session.resetAllStages();
        session.config.purgeMetaGlobal();
        session.config.purgeCryptoKeys();
        session.config.persistToPrefs();

        Logger.info(LOG_TAG, "Uploading new meta/global with sync ID " + mg.syncID + ".");

        
        
        
        
        mg.upload(new MetaGlobalDelegate() {
          @Override
          public void handleSuccess(MetaGlobal uploadedGlobal, SyncStorageResponse uploadResponse) {
            Logger.info(LOG_TAG, "Uploaded new meta/global with sync ID " + uploadedGlobal.syncID + ".");

            
            CollectionKeys keys = null;
            try {
              keys = session.generateNewCryptoKeys();
            } catch (CryptoException e) {
              Logger.warn(LOG_TAG, "Got exception generating new keys; failing fresh start.", e);
              freshStartDelegate.onFreshStartFailed(e);
            }
            if (keys == null) {
              Logger.warn(LOG_TAG, "Got null keys from generateNewKeys; failing fresh start.");
              freshStartDelegate.onFreshStartFailed(null);
            }

            
            Logger.info(LOG_TAG, "Uploading new crypto/keys.");
            session.uploadKeys(keys, new KeyUploadDelegate() {
              @Override
              public void onKeysUploaded() {
                Logger.info(LOG_TAG, "Uploaded new crypto/keys.");
                freshStartDelegate.onFreshStart();
              }

              @Override
              public void onKeyUploadFailed(Exception e) {
                Logger.warn(LOG_TAG, "Got exception uploading new keys.", e);
                freshStartDelegate.onFreshStartFailed(e);
              }
            });
          }

          @Override
          public void handleMissing(MetaGlobal global, SyncStorageResponse response) {
            
            Logger.warn(LOG_TAG, "Got 'missing' response uploading new meta/global.");
            freshStartDelegate.onFreshStartFailed(new Exception("meta/global missing while uploading."));
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  protected void wipeServer(final AuthHeaderProvider authHeaderProvider, final WipeServerDelegate wipeDelegate) {
    SyncStorageRequest request;
    final GlobalSession self = this;

    try {
      request = new SyncStorageRequest(config.storageURL());
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
      public AuthHeaderProvider getAuthHeaderProvider() {
        return GlobalSession.this.getAuthHeaderProvider();
      }
    };
    request.delete();
  }

  public void wipeAllStages() {
    Logger.info(LOG_TAG, "Wiping all stages.");
    
    this.wipeStagesByEnum(Stage.getNamedStages());
  }

  public void wipeStages(Collection<GlobalSyncStage> stages) {
    for (GlobalSyncStage stage : stages) {
      try {
        Logger.info(LOG_TAG, "Wiping " + stage);
        stage.wipeLocal(this);
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Ignoring wipe failure for stage " + stage, e);
      }
    }
  }

  public void wipeStagesByEnum(Collection<Stage> stages) {
    wipeStages(this.getSyncStagesByEnum(stages));
  }

  public void wipeStagesByName(Collection<String> names) {
    wipeStages(this.getSyncStagesByName(names));
  }

  public void resetAllStages() {
    Logger.info(LOG_TAG, "Resetting all stages.");
    
    this.resetStagesByEnum(Stage.getNamedStages());
  }

  public void resetStages(Collection<GlobalSyncStage> stages) {
    for (GlobalSyncStage stage : stages) {
      try {
        Logger.info(LOG_TAG, "Resetting " + stage);
        stage.resetLocal(this);
      } catch (Exception e) {
        Logger.error(LOG_TAG, "Ignoring reset failure for stage " + stage, e);
      }
    }
  }

  public void resetStagesByEnum(Collection<Stage> stages) {
    resetStages(this.getSyncStagesByEnum(stages));
  }

  public void resetStagesByName(Collection<String> names) {
    resetStages(this.getSyncStagesByName(names));
  }

  










  @SuppressWarnings("unchecked")
  protected JSONArray declinedEngineNames() {
    final JSONArray declined = new JSONArray();
    for (String engine : config.declinedEngineNames) {
      declined.add(engine);
    };

    return declined;
  }

  










  protected Set<String> enabledEngineNames() {
    if (config.enabledEngineNames != null) {
      return config.enabledEngineNames;
    }

    
    Set<String> validEngineNames = SyncConfiguration.validEngineNames();

    
    
    if (config.userSelectedEngines == null) {
      return validEngineNames;
    }

    
    
    
    
    
    Set<String> validAndSelectedEngineNames = new HashSet<String>();
    for (String engineName : validEngineNames) {
      if (config.userSelectedEngines.containsKey(engineName) &&
          !config.userSelectedEngines.get(engineName)) {
        continue;
      }
      validAndSelectedEngineNames.add(engineName);
    }
    return validAndSelectedEngineNames;
  }

  




  @SuppressWarnings("static-method")
  public CollectionKeys generateNewCryptoKeys() throws CryptoException {
    return CollectionKeys.generateCollectionKeys();
  }

  



  public MetaGlobal generateNewMetaGlobal() {
    final String newSyncID   = Utils.generateGuid();
    final String metaURL     = this.config.metaURL();

    ExtendedJSONObject engines = new ExtendedJSONObject();
    for (String engineName : enabledEngineNames()) {
      EngineSettings engineSettings = null;
      try {
        GlobalSyncStage globalStage = this.getSyncStageByName(engineName);
        Integer version = globalStage.getStorageVersion();
        if (version == null) {
          continue; 
        }
        engineSettings = new EngineSettings(Utils.generateGuid(), version.intValue());
      } catch (NoSuchStageException e) {
        
        
        engineSettings = new EngineSettings(Utils.generateGuid(), 0);
      }
      engines.put(engineName, engineSettings.toJSONObject());
    }

    MetaGlobal metaGlobal = new MetaGlobal(metaURL, this.getAuthHeaderProvider());
    metaGlobal.setSyncID(newSyncID);
    metaGlobal.setStorageVersion(STORAGE_VERSION);
    metaGlobal.setEngines(engines);

    
    
    metaGlobal.setDeclinedEngineNames(this.declinedEngineNames());

    return metaGlobal;
  }

  



  public void requiresUpgrade() {
    Logger.info(LOG_TAG, "Client outdated storage version; requires update.");
    
    this.abort(null, "Requires upgrade");
  }

  

















  public boolean isEngineRemotelyEnabled(String engineName, EngineSettings engineSettings) throws MetaGlobalException {
    if (this.config.metaGlobal == null) {
      throw new MetaGlobalNotSetException();
    }

    
    if (this.config.enabledEngineNames == null) {
      Logger.error(LOG_TAG, "No enabled engines in config. Giving up.");
      throw new MetaGlobalMissingEnginesException();
    }

    if (!(this.config.enabledEngineNames.contains(engineName))) {
      Logger.debug(LOG_TAG, "Engine " + engineName + " not enabled: no meta/global entry.");
      return false;
    }

    
    
    if (engineSettings != null) {
      
      this.config.metaGlobal.verifyEngineSettings(engineName, engineSettings);
    }

    return true;
  }


  










  public boolean isEngineLocallyEnabled(String stageName) {
    if (config.stagesToSync == null) {
      return true;
    }
    return config.stagesToSync.contains(stageName);
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
