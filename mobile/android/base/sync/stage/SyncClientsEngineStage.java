



package org.mozilla.gecko.sync.stage;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.simple.JSONArray;
import org.mozilla.gecko.sync.CryptoRecord;
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

import ch.boye.httpclientandroidlib.HttpStatus;

public class SyncClientsEngineStage implements GlobalSyncStage {
  public static final String LOG_TAG = "SyncClientsEngineStage";
  public static final String COLLECTION_NAME = "clients";
  public static final int CLIENTS_TTL_REFRESH = 604800000; 
  public static final int MAX_UPLOAD_FAILURE_COUNT = 5;

  protected GlobalSession session;
  protected final ClientRecordFactory factory = new ClientRecordFactory();
  protected ClientUploadDelegate clientUploadDelegate;
  protected ClientDownloadDelegate clientDownloadDelegate;
  protected ClientsDatabaseAccessor db;

  protected volatile boolean shouldWipe;
  protected volatile boolean commandsProcessedShouldUpload;
  protected final AtomicInteger uploadAttemptsCount = new AtomicInteger();

  










  public class ClientDownloadDelegate extends WBOCollectionRequestDelegate {

    
    final ClientsDataDelegate clientsDelegate = session.getClientsDelegate();

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
      BaseResource.consumeEntity(response); 
      final int clientsCount;
      try {
        clientsCount = db.clientsCount();
      } finally {
        
        
        db.close();
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
      try {
        Logger.info(LOG_TAG, "Client upload failed. Aborting sync.");
        session.abort(new HTTPFailureException(response), "Client download failed.");
      } finally {
        
        db.close();
      }
    }

    @Override
    public void handleRequestError(Exception ex) {
      try {
        Logger.info(LOG_TAG, "Client upload error. Aborting sync.");
        session.abort(ex, "Failure fetching client record.");
      } finally {
        
        db.close();
      }
    }

    @Override
    public void handleWBO(CryptoRecord record) {
      ClientRecord r;
      try {
        r = (ClientRecord) factory.createRecord(record.decrypt());
        if (clientsDelegate.isLocalGUID(r.guid)) {
          
          
          
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
        return session.keyForCollection(COLLECTION_NAME);
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
      Long timestampInMilliseconds = session.config.getPersistedServerClientRecordTimestamp();

      
      if (timestampInMilliseconds == 0) {
        return null;
      }

      return Utils.millisecondsToDecimalSecondsString(timestampInMilliseconds);
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Logger.debug(LOG_TAG, "Upload succeeded.");
      commandsProcessedShouldUpload = false;
      uploadAttemptsCount.set(0);
      session.config.persistServerClientRecordTimestamp(response.normalizedWeaveTimestamp());

      BaseResource.consumeEntity(response);
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
        return session.keyForCollection(COLLECTION_NAME);
      } catch (NoCollectionKeysSetException e) {
        session.abort(e, "No collection keys set.");
        return null;
      }
    }
  }

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    this.session = session;
    init();

    if (shouldDownload()) {
      downloadClientRecords();   
    } else {
      
    }
  }

  protected ClientRecord newLocalClientRecord(ClientsDataDelegate delegate) {
    final String ourGUID = delegate.getAccountGUID();
    final String ourName = delegate.getClientName();

    ClientRecord r = new ClientRecord(ourGUID);
    r.name = ourName;
    return r;    
  }

  protected void init() {
    db = new ClientsDatabaseAccessor(session.getContext());
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
      URI putURI = session.config.wboURI(COLLECTION_NAME, record.guid);

      SyncStorageRecordRequest request = new SyncStorageRecordRequest(putURI);
      request.delegate = clientUploadDelegate;
      request.put(record);
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

  protected ClientDownloadDelegate makeClientDownloadDelegate() {
    return new ClientDownloadDelegate();
  }

  protected void wipeAndStore(ClientRecord record) {
    if (shouldWipe) {
      db.wipe();
      shouldWipe = false;
    }
    db.store(record);
  }
}
