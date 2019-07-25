




































package org.mozilla.gecko.sync.stage;

import java.io.IOException;
import java.net.URISyntaxException;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.MetaGlobalException;
import org.mozilla.gecko.sync.NoCollectionKeysSetException;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SynchronizerConfiguration;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.middleware.Crypto5MiddlewareRepository;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.Server11Repository;
import org.mozilla.gecko.sync.synchronizer.Synchronizer;
import org.mozilla.gecko.sync.synchronizer.SynchronizerDelegate;

import android.util.Log;








public abstract class ServerSyncStage implements
    GlobalSyncStage,
    SynchronizerDelegate {

  protected GlobalSession session;
  protected String LOG_TAG = "ServerSyncStage";

  





  protected boolean isEnabled() throws MetaGlobalException {
    return session.engineIsEnabled(this.getEngineName());
  }
  protected abstract String getCollection();
  protected abstract String getEngineName();
  protected abstract Repository getLocalRepository();
  protected abstract RecordFactory getRecordFactory();

  









  protected Repository wrappedServerRepo() throws NoCollectionKeysSetException, URISyntaxException {
    String collection = this.getCollection();
    KeyBundle collectionKey = session.keyForCollection(collection);
    Server11Repository serverRepo = new Server11Repository(session.config.getClusterURLString(),
                                                           session.config.username,
                                                           collection,
                                                           session);
    Crypto5MiddlewareRepository cryptoRepo = new Crypto5MiddlewareRepository(serverRepo, collectionKey);
    cryptoRepo.recordFactory = getRecordFactory();
    return cryptoRepo;
  }

  protected String bundlePrefix() {
    return this.getCollection() + ".";
  }

  public Synchronizer getConfiguredSynchronizer(GlobalSession session) throws NoCollectionKeysSetException, URISyntaxException, NonObjectJSONException, IOException, ParseException {
    Repository remote = wrappedServerRepo();

    Synchronizer synchronizer = new Synchronizer();
    synchronizer.repositoryA = remote;
    synchronizer.repositoryB = this.getLocalRepository();

    SynchronizerConfiguration config = new SynchronizerConfiguration(session.config.getBranch(bundlePrefix()));
    synchronizer.load(config);

    
    
    return synchronizer;
  }

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    Log.d(LOG_TAG, "Starting execute.");

    this.session = session;
    try {
      if (!this.isEnabled()) {
        Log.i(LOG_TAG, "Stage disabled; skipping.");
        session.advance();
        return;
      }
    } catch (MetaGlobalException e) {
      session.abort(e, "Inappropriate meta/global; refusing to execute " + this.getEngineName() + " stage.");
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
    Log.d(LOG_TAG, "Invoking synchronizer.");
    synchronizer.synchronize(session.getContext(), this);
    Log.d(LOG_TAG, "Reached end of execute.");
  }

  @Override
  public void onSynchronized(Synchronizer synchronizer) {
    Log.d(LOG_TAG, "onSynchronized.");
    synchronizer.save().persist(session.config.getBranch(bundlePrefix()));
    session.advance();
  }

  @Override
  public void onSynchronizeFailed(Synchronizer synchronizer,
                                  Exception lastException, String reason) {
    Log.i(LOG_TAG, "onSynchronizeFailed: " + reason);
    session.abort(lastException, reason);
  }

  @Override
  public void onSynchronizeAborted(Synchronizer synchronize) {
    Log.i(LOG_TAG, "onSynchronizeAborted.");
    session.abort(null, "Synchronization was aborted.");
  }
}
