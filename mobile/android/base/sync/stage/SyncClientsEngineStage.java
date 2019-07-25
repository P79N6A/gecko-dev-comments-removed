



package org.mozilla.gecko.sync.stage;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.CommandProcessor;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.NoCollectionKeysSetException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.SyncStorageCollectionRequest;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.net.WBOCollectionRequestDelegate;
import org.mozilla.gecko.sync.net.WBORequestDelegate;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabaseAccessor;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.sync.repositories.domain.ClientRecordFactory;
import org.mozilla.gecko.sync.repositories.domain.VersionConstants;

import ch.boye.httpclientandroidlib.HttpStatus;

public class SyncClientsEngineStage implements GlobalSyncStage {
  public static final String LOG_TAG = "SyncClientsEngineStage";
  public static final String COLLECTION_NAME = "clients";
  public static final int CLIENTS_TTL_REFRESH = 604800000; 
  public static final int MAX_UPLOAD_FAILURE_COUNT = 5;

  protected final GlobalSession session;
  protected final ClientRecordFactory factory = new ClientRecordFactory();
  protected ClientUploadDelegate clientUploadDelegate;
  protected ClientDownloadDelegate clientDownloadDelegate;

  
  protected ClientsDatabaseAccessor db;

  protected volatile boolean shouldWipe;
  protected volatile boolean commandsProcessedShouldUpload;
  protected final AtomicInteger uploadAttemptsCount = new AtomicInteger();

  public SyncClientsEngineStage(GlobalSession session) {
    if (session == null) {
      throw new IllegalArgumentException("session must not be null.");
    }
    this.session = session;
  }

  protected int getClientsCount() {
    return getClientsDatabaseAccessor().clientsCount();
  }

  protected synchronized ClientsDatabaseAccessor getClientsDatabaseAccessor() {
    if (db == null) {
      db = new ClientsDatabaseAccessor(session.getContext());
    }
    return db;
  }

  protected synchronized void closeDataAccessor() {
    if (db == null) {
      return;
    }
    db.close();
    db = null;
  }

  










  public class ClientDownloadDelegate extends WBOCollectionRequestDelegate {

    
    final ClientsDataDelegate clientsDelegate = session.getClientsDelegate();
    boolean localAccountGUIDDownloaded = false;

    @Override
    public String credentials() {
      return session.credentials();
    }

    @Override
    public String ifUnmodifiedSince() {
      
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {

      
      
      session.config.persistServerClientsTimestamp(response.normalizedWeaveTimestamp());
      BaseResource.consumeEntity(response);

      
      
      if (!localAccountGUIDDownloaded) {
        Logger.info(LOG_TAG, "Local client GUID does not exist on the server. Upload timestamp will be reset.");
        session.config.persistServerClientRecordTimestamp(0);
      }
      localAccountGUIDDownloaded = false;

      final int clientsCount;
      try {
        clientsCount = getClientsCount();
      } finally {
        
        
        closeDataAccessor();
      }

      Logger.debug(LOG_TAG, "Database contains " + clientsCount + " clients.");
      Logger.debug(LOG_TAG, "Server response asserts " + response.weaveRecords() + " records.");

      
      clientUploadDelegate = new ClientUploadDelegate();
      clientsDelegate.setClientsCount(clientsCount);
      checkAndUpload();
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      BaseResource.consumeEntity(response); 
      localAccountGUIDDownloaded = false;

      try {
        Logger.info(LOG_TAG, "Client upload failed. Aborting sync.");
        session.abort(new HTTPFailureException(response), "Client download failed.");
      } finally {
        
        closeDataAccessor();
      }
    }

    @Override
    public void handleRequestError(Exception ex) {
      localAccountGUIDDownloaded = false;
      try {
        Logger.info(LOG_TAG, "Client upload error. Aborting sync.");
        session.abort(ex, "Failure fetching client record.");
      } finally {
        
        closeDataAccessor();
      }
    }

    @Override
    public void handleWBO(CryptoRecord record) {
      ClientRecord r;
      try {
        r = (ClientRecord) factory.createRecord(record.decrypt());
        if (clientsDelegate.isLocalGUID(r.guid)) {
          Logger.info(LOG_TAG, "Local client GUID exists on server and was downloaded");

          localAccountGUIDDownloaded = true;
          session.config.persistServerClientRecordTimestamp(r.lastModified);
          processCommands(r.commands);
        }
        RepoUtils.logClient(r);
      } catch (Exception e) {
        session.abort(e, "Exception handling client WBO.");
        return;
      }
      wipeAndStore(r);
    }

    @Override
    public KeyBundle keyBundle() {
      try {
        return session.keyBundleForCollection(COLLECTION_NAME);
      } catch (NoCollectionKeysSetException e) {
        session.abort(e, "No collection keys set.");
        return null;
      }
    }
  }

  public class ClientUploadDelegate extends WBORequestDelegate {
    protected static final String LOG_TAG = "ClientUploadDelegate";

    @Override
    public String credentials() {
      return session.credentials();
    }

    @Override
    public String ifUnmodifiedSince() {
      
      Long timestampInMilliseconds = session.config.getPersistedServerClientsTimestamp();

      
      if (timestampInMilliseconds == 0) {
        return null;
      }

      return Utils.millisecondsToDecimalSecondsString(timestampInMilliseconds);
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Logger.debug(LOG_TAG, "Upload succeeded.");
      try {
        commandsProcessedShouldUpload = false;
        uploadAttemptsCount.set(0);

        
        
        long timestamp = response.normalizedWeaveTimestamp();
        session.config.persistServerClientRecordTimestamp(timestamp);
        session.config.persistServerClientsTimestamp(timestamp);
        BaseResource.consumeEntity(response);

        Logger.debug(LOG_TAG, "Timestamp is " + timestamp);
      } catch (Exception e) {
        session.abort(e, "Unable to fetch timestamp.");
        return;
      }
      session.advance();
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      int statusCode = response.getStatusCode();

      
      
      if (!commandsProcessedShouldUpload ||
          statusCode == HttpStatus.SC_PRECONDITION_FAILED ||
          uploadAttemptsCount.incrementAndGet() > MAX_UPLOAD_FAILURE_COUNT) {
        Logger.debug(LOG_TAG, "Client upload failed. Aborting sync.");
        BaseResource.consumeEntity(response); 
        session.abort(new HTTPFailureException(response), "Client upload failed.");
        return;
      }
      Logger.trace(LOG_TAG, "Retrying uploadâ€¦");
      
      
      
      
      checkAndUpload();
    }

    @Override
    public void handleRequestError(Exception ex) {
      Logger.info(LOG_TAG, "Client upload error. Aborting sync.");
      session.abort(ex, "Client upload failed.");
    }

    @Override
    public KeyBundle keyBundle() {
      try {
        return session.keyBundleForCollection(COLLECTION_NAME);
      } catch (NoCollectionKeysSetException e) {
        session.abort(e, "No collection keys set.");
        return null;
      }
    }
  }

  @Override
  public void execute() throws NoSuchStageException {
    if (shouldDownload()) {
      downloadClientRecords();   
    } else {
      
    }
  }

  @Override
  public void resetLocal() {
    
    session.config.persistServerClientRecordTimestamp(0L);   
    session.config.persistServerClientsTimestamp(0L);

    session.getClientsDelegate().setClientsCount(0);
    try {
      getClientsDatabaseAccessor().wipe();
    } finally {
      closeDataAccessor();
    }
  }

  @Override
  public void wipeLocal() throws Exception {
    
    this.resetLocal();
  }

  public Integer getStorageVersion() {
    return VersionConstants.CLIENTS_ENGINE_VERSION;
  }

  protected ClientRecord newLocalClientRecord(ClientsDataDelegate delegate) {
    final String ourGUID = delegate.getAccountGUID();
    final String ourName = delegate.getClientName();

    ClientRecord r = new ClientRecord(ourGUID);
    r.name = ourName;
    return r;
  }

  
  protected boolean shouldDownload() {
    
    return true;
  }

  protected boolean shouldUpload() {
    if (commandsProcessedShouldUpload) {
      return true;
    }

    long lastUpload = session.config.getPersistedServerClientRecordTimestamp();   
    if (lastUpload == 0) {
      return true;
    }

    
    
    
    long now = System.currentTimeMillis();
    long age = now - lastUpload;
    return age >= CLIENTS_TTL_REFRESH;
  }

  protected void processCommands(JSONArray commands) {
    if (commands == null ||
        commands.size() == 0) {
      return;
    }

    commandsProcessedShouldUpload = true;
    CommandProcessor processor = CommandProcessor.getProcessor();

    
    for (int i = 0; i < commands.size(); i++) {
      processor.processCommand(new ExtendedJSONObject((JSONObject) commands.get(i)));
    }
  }

  protected void checkAndUpload() {
    if (!shouldUpload()) {
      Logger.debug(LOG_TAG, "Not uploading client record.");
      session.advance();
      return;
    }

    
    final String encryptionFailure = "Couldn't encrypt new client record.";
    final ClientRecord localClient = newLocalClientRecord(session.getClientsDelegate());
    try {
      CryptoRecord cryptoRecord = localClient.getEnvelope();
      cryptoRecord.keyBundle = clientUploadDelegate.keyBundle();
      cryptoRecord.encrypt();
      this.uploadClientRecord(cryptoRecord);
    } catch (UnsupportedEncodingException e) {
      session.abort(e, encryptionFailure + " Unsupported encoding.");
    } catch (CryptoException e) {
      session.abort(e, encryptionFailure);
    }
  }

  protected void downloadClientRecords() {
    shouldWipe = true;
    clientDownloadDelegate = makeClientDownloadDelegate();

    try {
      URI getURI = session.config.collectionURI(COLLECTION_NAME, true);

      SyncStorageCollectionRequest request = new SyncStorageCollectionRequest(getURI);
      request.delegate = clientDownloadDelegate;

      Logger.trace(LOG_TAG, "Downloading client records.");
      request.get();
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

  


  protected void uploadClientRecord(CryptoRecord record) {
    Logger.debug(LOG_TAG, "Uploading client record " + record.guid);
    try {
      URI postURI = session.config.collectionURI(COLLECTION_NAME);
      SyncStorageRecordRequest request = new SyncStorageRecordRequest(postURI);
      request.delegate = clientUploadDelegate;
      request.post(record);
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

  protected ClientDownloadDelegate makeClientDownloadDelegate() {
    return new ClientDownloadDelegate();
  }

  protected void wipeAndStore(ClientRecord record) {
    ClientsDatabaseAccessor db = getClientsDatabaseAccessor();
    if (shouldWipe) {
      db.wipe();
      shouldWipe = false;
    }
    db.store(record);
  }
}
