



package org.mozilla.gecko.sync.stage;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.concurrent.ExecutorService;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.MetaGlobalException;
import org.mozilla.gecko.sync.NoCollectionKeysSetException;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.middleware.Crypto5MiddlewareRepository;
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
import org.mozilla.gecko.sync.synchronizer.Synchronizer;
import org.mozilla.gecko.sync.synchronizer.SynchronizerDelegate;

import android.content.Context;








public abstract class ServerSyncStage implements
    GlobalSyncStage,
    SynchronizerDelegate {

  protected static final String LOG_TAG = "ServerSyncStage";

  protected final GlobalSession session;

  public ServerSyncStage(GlobalSession session) {
    if (session == null) {
      throw new IllegalArgumentException("session must not be null.");
    }
    this.session = session;
  }

  





  protected boolean isEnabled() throws MetaGlobalException {
    
    
    return session.engineIsEnabled(this.getEngineName(), null);
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

    Synchronizer synchronizer = new Synchronizer();
    synchronizer.repositoryA = remote;
    synchronizer.repositoryB = this.getLocalRepository();

    SynchronizerConfiguration config = this.getConfig();
    synchronizer.load(config);

    
    
    return synchronizer;
  }

  @Override
  public void resetLocal() {
    
    SynchronizerConfiguration config;
    try {
      config = this.getConfig();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Unable to reset " + this + ": fetching config failed.", e);
      return;
    }

    config.localBundle.setTimestamp(0L);
    config.remoteBundle.setTimestamp(0L);
    Logger.info(LOG_TAG, "Reset timestamps for " + this);
    persistConfig(config);
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
  public void wipeLocal() throws Exception {
    
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

  @Override
  public void execute() throws NoSuchStageException {
    final String name = getEngineName();
    Logger.debug(LOG_TAG, "Starting execute for " + name);

    try {
      if (!this.isEnabled()) {
        Logger.info(LOG_TAG, "Stage " + name + " disabled; skipping.");
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

  @Override
  public void onSynchronized(Synchronizer synchronizer) {
    Logger.debug(LOG_TAG, "onSynchronized.");

    SynchronizerConfiguration synchronizerConfiguration = synchronizer.save();
    if (synchronizerConfiguration != null) {
      persistConfig(synchronizerConfiguration);
    } else {
      Logger.warn(LOG_TAG, "Didn't get configuration from synchronizer after success");
    }

    Logger.info(LOG_TAG, "Advancing session.");
    session.advance();
  }

  @Override
  public void onSynchronizeFailed(Synchronizer synchronizer,
                                  Exception lastException, String reason) {
    Logger.debug(LOG_TAG, "onSynchronizeFailed: " + reason);

    
    if (lastException instanceof HTTPFailureException) {
      session.handleHTTPError(((HTTPFailureException)lastException).response, reason);
    } else {
      session.abort(lastException, reason);
    }
  }

  @Override
  public void onSynchronizeAborted(Synchronizer synchronize) {
    Logger.info(LOG_TAG, "onSynchronizeAborted.");

    session.abort(null, "Synchronization was aborted.");
  }
}
