



package org.mozilla.gecko.sync.stage;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.CommandProcessor;
import org.mozilla.gecko.sync.CommandProcessor.Command;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.HTTPFailureException;
import org.mozilla.gecko.sync.NoCollectionKeysSetException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.SyncStorageCollectionRequest;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageResponse;
import org.mozilla.gecko.sync.net.WBOCollectionRequestDelegate;
import org.mozilla.gecko.sync.net.WBORequestDelegate;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabaseAccessor;
import org.mozilla.gecko.sync.repositories.android.RepoUtils;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.sync.repositories.domain.ClientRecordFactory;
import org.mozilla.gecko.sync.repositories.domain.VersionConstants;

import ch.boye.httpclientandroidlib.HttpStatus;

public class SyncClientsEngineStage extends AbstractSessionManagingSyncStage {
  private static final String LOG_TAG = "SyncClientsEngineStage";

  public static final String COLLECTION_NAME       = "clients";
  public static final String STAGE_NAME            = COLLECTION_NAME;
  public static final int CLIENTS_TTL_REFRESH      = 604800000;   
  public static final int MAX_UPLOAD_FAILURE_COUNT = 5;

  protected final ClientRecordFactory factory = new ClientRecordFactory();
  protected ClientUploadDelegate clientUploadDelegate;
  protected ClientDownloadDelegate clientDownloadDelegate;

  
  protected ClientsDatabaseAccessor db;

  protected volatile boolean shouldWipe;
  protected volatile boolean shouldUploadLocalRecord;     
  protected final AtomicInteger uploadAttemptsCount = new AtomicInteger();
  protected final List<ClientRecord> toUpload = new ArrayList<ClientRecord>();

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
    public AuthHeaderProvider getAuthHeaderProvider() {
      return session.getAuthHeaderProvider();
    }

    @Override
    public String ifUnmodifiedSince() {
      
      return null;
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {

      
      
      session.config.persistServerClientsTimestamp(response.normalizedWeaveTimestamp());
      BaseResource.consumeEntity(response);

      
      wipeAndStore(null);

      
      
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

      
      
      if (toUpload.size() > 0) {
        uploadRemoteRecords();
        return;
      }
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
          Logger.info(LOG_TAG, "Local client GUID exists on server and was downloaded.");
          localAccountGUIDDownloaded = true;
          handleDownloadedLocalRecord(r);
        } else {
          
          wipeAndStore(r);
          addCommands(r);
        }
        RepoUtils.logClient(r);
      } catch (Exception e) {
        session.abort(e, "Exception handling client WBO.");
        return;
      }
    }

    @Override
    public KeyBundle keyBundle() {
      try {
        return session.keyBundleForCollection(COLLECTION_NAME);
      } catch (NoCollectionKeysSetException e) {
        return null;
      }
    }
  }

  public class ClientUploadDelegate extends WBORequestDelegate {
    protected static final String LOG_TAG = "ClientUploadDelegate";
    public Long currentlyUploadingRecordTimestamp;
    public boolean currentlyUploadingLocalRecord;

    @Override
    public AuthHeaderProvider getAuthHeaderProvider() {
      return session.getAuthHeaderProvider();
    }

    private void setUploadDetails(boolean isLocalRecord) {
      
      currentlyUploadingRecordTimestamp = session.config.getPersistedServerClientsTimestamp();
      currentlyUploadingLocalRecord = isLocalRecord;
    }

    @Override
    public String ifUnmodifiedSince() {
      Long timestampInMilliseconds = currentlyUploadingRecordTimestamp;

      
      if (timestampInMilliseconds <= 0) {
        return null;
      }

      return Utils.millisecondsToDecimalSecondsString(timestampInMilliseconds);
    }

    @Override
    public void handleRequestSuccess(SyncStorageResponse response) {
      Logger.debug(LOG_TAG, "Upload succeeded.");
      uploadAttemptsCount.set(0);

      
      
      final long responseTimestamp = response.normalizedWeaveTimestamp();
      Logger.trace(LOG_TAG, "Timestamp from header is: " + responseTimestamp);

      if (responseTimestamp == -1) {
        final String message = "Response did not contain a valid timestamp.";
        session.abort(new RuntimeException(message), message);
        return;
      }

      BaseResource.consumeEntity(response);
      session.config.persistServerClientsTimestamp(responseTimestamp);

      
      
      if (!currentlyUploadingLocalRecord) {
        
        clearRecordsToUpload();
        checkAndUpload();
        return;
      }

      
      
      shouldUploadLocalRecord = false;
      session.config.persistServerClientRecordTimestamp(responseTimestamp);
      session.advance();
    }

    @Override
    public void handleRequestFailure(SyncStorageResponse response) {
      int statusCode = response.getStatusCode();

      
      
      if (!shouldUploadLocalRecord ||
          statusCode == HttpStatus.SC_PRECONDITION_FAILED ||
          uploadAttemptsCount.incrementAndGet() > MAX_UPLOAD_FAILURE_COUNT) {

        Logger.debug(LOG_TAG, "Client upload failed. Aborting sync.");
        if (!currentlyUploadingLocalRecord) {
          toUpload.clear(); 
        }
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
        return null;
      }
    }
  }

  @Override
  public void execute() throws NoSuchStageException {
    
    boolean disabledThisSync = session.config.stagesToSync != null &&
                               !session.config.stagesToSync.contains(STAGE_NAME);
    if (disabledThisSync) {
      Logger.debug(LOG_TAG, "Stage " + STAGE_NAME + " disabled just for this sync.");
      session.advance();
      return;
    }

    if (shouldDownload()) {
      downloadClientRecords();   
    } else {
      
    }
  }

  @Override
  protected void resetLocal() {
    
    session.config.persistServerClientRecordTimestamp(0L);   
    session.config.persistServerClientsTimestamp(0L);

    session.getClientsDelegate().setClientsCount(0);
    try {
      getClientsDatabaseAccessor().wipeDB();
    } finally {
      closeDataAccessor();
    }
  }

  @Override
  protected void wipeLocal() throws Exception {
    
    this.resetLocal();
  }

  public Integer getStorageVersion() {
    return VersionConstants.CLIENTS_ENGINE_VERSION;
  }

  protected String getLocalClientVersion() {
    return GlobalConstants.MOZ_APP_VERSION;
  }

  protected ClientRecord newLocalClientRecord(ClientsDataDelegate delegate) {
    final String ourGUID = delegate.getAccountGUID();
    final String ourName = delegate.getClientName();

    ClientRecord r = new ClientRecord(ourGUID);
    r.name = ourName;
    r.version = getLocalClientVersion();
    return r;
  }

  
  protected boolean shouldDownload() {
    
    return true;
  }

  protected boolean shouldUpload() {
    if (shouldUploadLocalRecord) {
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

  protected void handleDownloadedLocalRecord(ClientRecord r) {
    session.config.persistServerClientRecordTimestamp(r.lastModified);

    if (!getLocalClientVersion().equals(r.version)) {
      shouldUploadLocalRecord = true;
    }
    processCommands(r.commands);
  }

  protected void processCommands(JSONArray commands) {
    if (commands == null ||
        commands.size() == 0) {
      return;
    }

    shouldUploadLocalRecord = true;
    CommandProcessor processor = CommandProcessor.getProcessor();

    for (Object o : commands) {
      processor.processCommand(session, new ExtendedJSONObject((JSONObject) o));
    }
  }

  @SuppressWarnings("unchecked")
  protected void addCommands(ClientRecord record) throws NullCursorException {
    Logger.trace(LOG_TAG, "Adding commands to " + record.guid);
    List<Command> commands = db.fetchCommandsForClient(record.guid);

    if (commands == null || commands.size() == 0) {
      Logger.trace(LOG_TAG, "No commands to add.");
      return;
    }

    for (Command command : commands) {
      JSONObject jsonCommand = command.asJSONObject();
      if (record.commands == null) {
        record.commands = new JSONArray();
      }
      record.commands.add(jsonCommand);
    }
    toUpload.add(record);
  }

  @SuppressWarnings("unchecked")
  protected void uploadRemoteRecords() {
    Logger.trace(LOG_TAG, "In uploadRemoteRecords. Uploading " + toUpload.size() + " records" );

    for (ClientRecord r : toUpload) {
      Logger.trace(LOG_TAG, ">> Uploading record " + r.guid + ": " + r.name);
    }

    if (toUpload.size() == 1) {
      ClientRecord record = toUpload.get(0);
      Logger.debug(LOG_TAG, "Only 1 remote record to upload.");
      Logger.debug(LOG_TAG, "Record last modified: " + record.lastModified);
      CryptoRecord cryptoRecord = encryptClientRecord(record);
      if (cryptoRecord != null) {
        clientUploadDelegate.setUploadDetails(false);
        this.uploadClientRecord(cryptoRecord);
      }
      return;
    }

    JSONArray cryptoRecords = new JSONArray();
    for (ClientRecord record : toUpload) {
      Logger.trace(LOG_TAG, "Record " + record.guid + " is being uploaded" );

      CryptoRecord cryptoRecord = encryptClientRecord(record);
      cryptoRecords.add(cryptoRecord.toJSONObject());
    }
    Logger.debug(LOG_TAG, "Uploading records: " + cryptoRecords.size());
    clientUploadDelegate.setUploadDetails(false);
    this.uploadClientRecords(cryptoRecords);
  }

  protected void checkAndUpload() {
    if (!shouldUpload()) {
      Logger.debug(LOG_TAG, "Not uploading client record.");
      session.advance();
      return;
    }

    final ClientRecord localClient = newLocalClientRecord(session.getClientsDelegate());
    clientUploadDelegate.setUploadDetails(true);
    CryptoRecord cryptoRecord = encryptClientRecord(localClient);
    if (cryptoRecord != null) {
      this.uploadClientRecord(cryptoRecord);
    }
  }

  protected CryptoRecord encryptClientRecord(ClientRecord recordToUpload) {
    
    final String encryptionFailure = "Couldn't encrypt new client record.";

    try {
      CryptoRecord cryptoRecord = recordToUpload.getEnvelope();
      cryptoRecord.keyBundle = clientUploadDelegate.keyBundle();
      if (cryptoRecord.keyBundle == null) {
        session.abort(new NoCollectionKeysSetException(), "No collection keys set.");
        return null;
      }
      return cryptoRecord.encrypt();
    } catch (UnsupportedEncodingException e) {
      session.abort(e, encryptionFailure + " Unsupported encoding.");
    } catch (CryptoException e) {
      session.abort(e, encryptionFailure);
    }
    return null;
  }

  public void clearRecordsToUpload() {
    try {
      getClientsDatabaseAccessor().wipeCommandsTable();
      toUpload.clear();
    } finally {
      closeDataAccessor();
    }
  }

  protected void downloadClientRecords() {
    shouldWipe = true;
    clientDownloadDelegate = makeClientDownloadDelegate();

    try {
      final URI getURI = session.config.collectionURI(COLLECTION_NAME, true);
      final SyncStorageCollectionRequest request = new SyncStorageCollectionRequest(getURI);
      request.delegate = clientDownloadDelegate;

      Logger.trace(LOG_TAG, "Downloading client records.");
      request.get();
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

  protected void uploadClientRecords(JSONArray records) {
    Logger.trace(LOG_TAG, "Uploading " + records.size() + " client records.");
    try {
      final URI postURI = session.config.collectionURI(COLLECTION_NAME, false);
      final SyncStorageRecordRequest request = new SyncStorageRecordRequest(postURI);
      request.delegate = clientUploadDelegate;
      request.post(records);
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    } catch (Exception e) {
      session.abort(e, "Unable to parse body.");
    }
  }

  


  protected void uploadClientRecord(CryptoRecord record) {
    Logger.debug(LOG_TAG, "Uploading client record " + record.guid);
    try {
      final URI postURI = session.config.collectionURI(COLLECTION_NAME);
      final SyncStorageRecordRequest request = new SyncStorageRecordRequest(postURI);
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
    final ClientsDatabaseAccessor db = getClientsDatabaseAccessor();
    if (shouldWipe) {
      db.wipeClientsTable();
      shouldWipe = false;
    }
    if (record != null) {
      db.store(record);
    }
  }
}
