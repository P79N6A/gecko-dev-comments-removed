



package org.mozilla.gecko.sync.stage;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.Map;
import java.util.concurrent.ExecutorService;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.CredentialsSource;
import org.mozilla.gecko.sync.EngineSettings;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.MetaGlobalException;
import org.mozilla.gecko.sync.NoCollectionKeysSetException;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.WipeServerDelegate;
import org.mozilla.gecko.sync.middleware.Crypto5MiddlewareRepository;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.SyncStorageRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;
import org.mozilla.gecko.sync.repositories.Server11Repository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.synchronizer.ServerLocalSynchronizer;
import org.mozilla.gecko.sync.synchronizer.Synchronizer;
import org.mozilla.gecko.sync.synchronizer.SynchronizerDelegate;
import org.mozilla.gecko.sync.synchronizer.SynchronizerSession;

import android.content.Context;








public abstract class ServerSyncStage extends AbstractSessionManagingSyncStage implements SynchronizerDelegate {

  protected static final String LOG_TAG = "ServerSyncStage";

  protected long stageStartTimestamp = -1;
  protected long stageCompleteTimestamp = -1;

  





  protected boolean isEnabled() throws MetaGlobalException {
    EngineSettings engineSettings = null;
    try {
       engineSettings = getEngineSettings();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Unable to get engine settings for " + this + ": fetching config failed.", e);
      
    }

    
    
    
    boolean enabledInMetaGlobal = session.engineIsEnabled(this.getEngineName(), engineSettings);

    
    checkAndUpdateUserSelectedEngines(enabledInMetaGlobal);

    
    if (!enabledInMetaGlobal) {
      Logger.debug(LOG_TAG, "Stage " + this.getEngineName() + " disabled by server meta/global.");
      return false;
    }

    
    if (session.config.stagesToSync == null) {
      return true;
    }
    boolean enabledThisSync = session.config.stagesToSync.contains(this.getEngineName()); 
    if (!enabledThisSync) {
      Logger.debug(LOG_TAG, "Stage " + this.getEngineName() + " disabled just for this sync.");
    }
    return enabledThisSync;
  }

  










  protected void checkAndUpdateUserSelectedEngines(boolean enabledInMetaGlobal) throws MetaGlobalException {
    Map<String, Boolean> selectedEngines = session.config.userSelectedEngines;
    String thisEngine = this.getEngineName();

    if (selectedEngines != null && selectedEngines.containsKey(thisEngine)) {
      boolean enabledInSelection = selectedEngines.get(thisEngine);
      if (enabledInMetaGlobal != enabledInSelection) {
        
        Logger.debug(LOG_TAG, "Engine state has been changed by user. Throwing exception.");
        throw new MetaGlobalException.MetaGlobalEngineStateChangedException(enabledInSelection);
      }
    }
  }

  protected EngineSettings getEngineSettings() throws NonObjectJSONException, IOException, ParseException {
    Integer version = getStorageVersion();
    if (version == null) {
      Logger.warn(LOG_TAG, "null storage version for " + this + "; using version 0.");
      version = Integer.valueOf(0);
    }

    SynchronizerConfiguration config = this.getConfig();
    if (config == null) {
      return new EngineSettings(null, version.intValue());
    }
    return new EngineSettings(config.syncID, version.intValue());
  }

  protected abstract String getCollection();
  protected abstract String getEngineName();
  protected abstract Repository getLocalRepository();
  protected abstract RecordFactory getRecordFactory();

  
  protected Repository getRemoteRepository() throws URISyntaxException {
    return new Server11Repository(session.config.getClusterURLString(),
                                  session.config.username,
                                  getCollection(),
                                  session);
  }

  





  protected Repository wrappedServerRepo() throws NoCollectionKeysSetException, URISyntaxException {
    String collection = this.getCollection();
    KeyBundle collectionKey = session.keyBundleForCollection(collection);
    Crypto5MiddlewareRepository cryptoRepo = new Crypto5MiddlewareRepository(getRemoteRepository(), collectionKey);
    cryptoRepo.recordFactory = getRecordFactory();
    return cryptoRepo;
  }

  protected String bundlePrefix() {
    return this.getCollection() + ".";
  }

  protected SynchronizerConfiguration getConfig() throws NonObjectJSONException, IOException, ParseException {
    return new SynchronizerConfiguration(session.config.getBranch(bundlePrefix()));
  }

  protected void persistConfig(SynchronizerConfiguration synchronizerConfiguration) {
    synchronizerConfiguration.persist(session.config.getBranch(bundlePrefix()));
  }

  public Synchronizer getConfiguredSynchronizer(GlobalSession session) throws NoCollectionKeysSetException, URISyntaxException, NonObjectJSONException, IOException, ParseException {
    Repository remote = wrappedServerRepo();

    Synchronizer synchronizer = new ServerLocalSynchronizer();
    synchronizer.repositoryA = remote;
    synchronizer.repositoryB = this.getLocalRepository();
    synchronizer.load(getConfig());

    return synchronizer;
  }

  


  @Override
  protected void resetLocal() {
    resetLocalWithSyncID(null);
  }

  



  protected void resetLocalWithSyncID(String syncID) {
    
    SynchronizerConfiguration config;
    try {
      config = this.getConfig();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Unable to reset " + this + ": fetching config failed.", e);
      return;
    }

    if (syncID != null) {
      config.syncID = syncID;
      Logger.info(LOG_TAG, "Setting syncID for " + this + " to '" + syncID + "'.");
    }
    config.localBundle.setTimestamp(0L);
    config.remoteBundle.setTimestamp(0L);
    persistConfig(config);
    Logger.info(LOG_TAG, "Reset timestamps for " + this);
  }

  
  private class WipeWaiter {
    public boolean sessionSucceeded = true;
    public boolean wipeSucceeded = true;
    public Exception error;

    public void notify(Exception e, boolean sessionSucceeded) {
      this.sessionSucceeded = sessionSucceeded;
      this.wipeSucceeded = false;
      this.error = e;
      this.notify();
    }
  }

  





  @Override
  protected void wipeLocal() throws Exception {
    
    this.resetLocal();

    final WipeWaiter monitor = new WipeWaiter();
    final Context context = session.getContext();
    final Repository r = this.getLocalRepository();

    final Runnable doWipe = new Runnable() {
      @Override
      public void run() {
        r.createSession(new RepositorySessionCreationDelegate() {

          @Override
          public void onSessionCreated(final RepositorySession session) {
            try {
              session.begin(new RepositorySessionBeginDelegate() {

                @Override
                public void onBeginSucceeded(final RepositorySession session) {
                  session.wipe(new RepositorySessionWipeDelegate() {
                    @Override
                    public void onWipeSucceeded() {
                      try {
                        session.finish(new RepositorySessionFinishDelegate() {

                          @Override
                          public void onFinishSucceeded(RepositorySession session,
                                                        RepositorySessionBundle bundle) {
                            
                            synchronized (monitor) {
                              monitor.notify();
                            }
                          }

                          @Override
                          public void onFinishFailed(Exception ex) {
                            
                            synchronized (monitor) {
                              monitor.notify(ex, true);
                            }
                          }

                          @Override
                          public RepositorySessionFinishDelegate deferredFinishDelegate(ExecutorService executor) {
                            return this;
                          }
                        });
                      } catch (InactiveSessionException e) {
                        
                        synchronized (monitor) {
                          monitor.notify(e, true);
                        }
                      }
                    }

                    @Override
                    public void onWipeFailed(Exception ex) {
                      session.abort();
                      synchronized (monitor) {
                        monitor.notify(ex, true);
                      }
                    }

                    @Override
                    public RepositorySessionWipeDelegate deferredWipeDelegate(ExecutorService executor) {
                      return this;
                    }
                  });
                }

                @Override
                public void onBeginFailed(Exception ex) {
                  session.abort();
                  synchronized (monitor) {
                    monitor.notify(ex, true);
                  }
                }

                @Override
                public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService executor) {
                  return this;
                }
              });
            } catch (InvalidSessionTransitionException e) {
              session.abort();
              synchronized (monitor) {
                monitor.notify(e, true);
              }
            }
          }

          @Override
          public void onSessionCreateFailed(Exception ex) {
            synchronized (monitor) {
              monitor.notify(ex, false);
            }
          }

          @Override
          public RepositorySessionCreationDelegate deferredCreationDelegate() {
            return this;
          }
        }, context);
      }
    };

    final Thread wiping = new Thread(doWipe);
    synchronized (monitor) {
      wiping.start();
      try {
        monitor.wait();
      } catch (InterruptedException e) {
        Logger.error(LOG_TAG, "Wipe interrupted.");
      }
    }

    if (!monitor.sessionSucceeded) {
      Logger.error(LOG_TAG, "Failed to create session for wipe.");
      throw monitor.error;
    }

    if (!monitor.wipeSucceeded) {
      Logger.error(LOG_TAG, "Failed to wipe session.");
      throw monitor.error;
    }

    Logger.info(LOG_TAG, "Wiping stage complete.");
  }

  


  protected void wipeServer(final CredentialsSource credentials, final WipeServerDelegate wipeDelegate) {
    SyncStorageRequest request;

    try {
      request = new SyncStorageRequest(session.config.collectionURI(getCollection()));
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
        resetLocal();
        wipeDelegate.onWiped(response.normalizedWeaveTimestamp());
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        Logger.warn(LOG_TAG, "Got request failure " + response.getStatusCode() + " in wipeServer.");
        
        session.interpretHTTPFailure(response.httpResponse());
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

  




  public void wipeServer(final GlobalSession session) throws Exception {
    this.session = session;

    final WipeWaiter monitor = new WipeWaiter();

    final Runnable doWipe = new Runnable() {
      @Override
      public void run() {
        wipeServer(session, new WipeServerDelegate() {
          @Override
          public void onWiped(long timestamp) {
            synchronized (monitor) {
              monitor.notify();
            }
          }

          @Override
          public void onWipeFailed(Exception e) {
            synchronized (monitor) {
              monitor.notify(e, false);
            }
          }
        });
      }
    };

    final Thread wiping = new Thread(doWipe);
    synchronized (monitor) {
      wiping.start();
      try {
        monitor.wait();
      } catch (InterruptedException e) {
        Logger.error(LOG_TAG, "Server wipe interrupted.");
      }
    }

    if (!monitor.wipeSucceeded) {
      Logger.error(LOG_TAG, "Failed to wipe server.");
      throw monitor.error;
    }

    Logger.info(LOG_TAG, "Wiping server complete.");
  }

  @Override
  public void execute() throws NoSuchStageException {
    final String name = getEngineName();
    Logger.debug(LOG_TAG, "Starting execute for " + name);

    stageStartTimestamp = System.currentTimeMillis();

    try {
      if (!this.isEnabled()) {
        Logger.info(LOG_TAG, "Skipping stage " + name + ".");
        session.advance();
        return;
      }
    } catch (MetaGlobalException.MetaGlobalMalformedSyncIDException e) {
      
      try {
        session.recordForMetaGlobalUpdate(name, new EngineSettings(Utils.generateGuid(), this.getStorageVersion()));
        Logger.info(LOG_TAG, "Wiping server because malformed engine sync ID was found in meta/global.");
        wipeServer(session);
        Logger.info(LOG_TAG, "Wiped server after malformed engine sync ID found in meta/global.");
      } catch (Exception ex) {
        session.abort(ex, "Failed to wipe server after malformed engine sync ID found in meta/global.");
      }
    } catch (MetaGlobalException.MetaGlobalMalformedVersionException e) {
      
      try {
        session.recordForMetaGlobalUpdate(name, new EngineSettings(Utils.generateGuid(), this.getStorageVersion()));
        Logger.info(LOG_TAG, "Wiping server because malformed engine version was found in meta/global.");
        wipeServer(session);
        Logger.info(LOG_TAG, "Wiped server after malformed engine version found in meta/global.");
      } catch (Exception ex) {
        session.abort(ex, "Failed to wipe server after malformed engine version found in meta/global.");
      }
    } catch (MetaGlobalException.MetaGlobalStaleClientSyncIDException e) {
      
      Logger.warn(LOG_TAG, "Remote engine syncID different from local engine syncID:" +
                           " resetting local engine and assuming remote engine syncID.");
      this.resetLocalWithSyncID(e.serverSyncID);
    } catch (MetaGlobalException.MetaGlobalEngineStateChangedException e) {
      boolean isEnabled = e.isEnabled;
      if (!isEnabled) {
        
        session.removeEngineFromMetaGlobal(name);
      } else {
        
        String newSyncID = Utils.generateGuid();
        session.recordForMetaGlobalUpdate(name, new EngineSettings(newSyncID, this.getStorageVersion()));
        
        this.resetLocalWithSyncID(newSyncID);
      }
      try {
        
        Logger.warn(LOG_TAG, "Wiping server because engine sync state changed.");
        wipeServer(session);
        Logger.warn(LOG_TAG, "Wiped server because engine sync state changed.");
      } catch (Exception ex) {
        session.abort(ex, "Failed to wipe server after engine sync state changed");
      }
      if (!isEnabled) {
        Logger.warn(LOG_TAG, "Stage has been disabled. Advancing to next stage.");
        session.advance();
        return;
      }
    } catch (MetaGlobalException e) {
      session.abort(e, "Inappropriate meta/global; refusing to execute " + name + " stage.");
      return;
    }

    Synchronizer synchronizer;
    try {
      synchronizer = this.getConfiguredSynchronizer(session);
    } catch (NoCollectionKeysSetException e) {
      session.abort(e, "No CollectionKeys.");
      return;
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI syntax for server repository.");
      return;
    } catch (NonObjectJSONException e) {
      session.abort(e, "Invalid persisted JSON for config.");
      return;
    } catch (IOException e) {
      session.abort(e, "Invalid persisted JSON for config.");
      return;
    } catch (ParseException e) {
      session.abort(e, "Invalid persisted JSON for config.");
      return;
    }

    Logger.debug(LOG_TAG, "Invoking synchronizer.");
    synchronizer.synchronize(session.getContext(), this);
    Logger.debug(LOG_TAG, "Reached end of execute.");
  }

  




  protected String getStageDurationString() {
    return Utils.formatDuration(stageStartTimestamp, stageCompleteTimestamp);
  }

  




  @Override
  public void onSynchronized(Synchronizer synchronizer) {
    stageCompleteTimestamp = System.currentTimeMillis();
    Logger.debug(LOG_TAG, "onSynchronized.");

    SynchronizerConfiguration newConfig = synchronizer.save();
    if (newConfig != null) {
      persistConfig(newConfig);
    } else {
      Logger.warn(LOG_TAG, "Didn't get configuration from synchronizer after success.");
    }

    final SynchronizerSession synchronizerSession = synchronizer.getSynchronizerSession();
    int inboundCount = synchronizerSession.getInboundCount();
    int outboundCount = synchronizerSession.getOutboundCount();
    Logger.info(LOG_TAG, "Stage " + getEngineName() +
        " received " + inboundCount + " and sent " + outboundCount +
        " records in " + getStageDurationString() + ".");
    Logger.info(LOG_TAG, "Advancing session.");
    session.advance();
  }

  






  @Override
  public void onSynchronizeFailed(Synchronizer synchronizer,
                                  Exception lastException, String reason) {
    stageCompleteTimestamp = System.currentTimeMillis();
    Logger.warn(LOG_TAG, "Synchronize failed: " + reason, lastException);

    
    
    if (lastException instanceof HTTPFailureException) {
      SyncStorageResponse response = ((HTTPFailureException)lastException).response;
      if (response.retryAfterInSeconds() > 0) {
        session.handleHTTPError(response, reason); 
        return;
      } else {
        session.interpretHTTPFailure(response.httpResponse()); 
      }
    }

    Logger.info(LOG_TAG, "Advancing session even though stage failed (took " + getStageDurationString() +
        "). Timestamps not persisted.");
    session.advance();
  }
}
