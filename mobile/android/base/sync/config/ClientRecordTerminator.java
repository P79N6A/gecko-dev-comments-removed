



package org.mozilla.gecko.sync.config;

import java.net.URI;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.SyncStorageRecordRequest;
import org.mozilla.gecko.sync.net.SyncStorageRequestDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;







public class ClientRecordTerminator {
  public static final String LOG_TAG = "ClientRecTerminator";

  protected ClientRecordTerminator() {
    super(); 
  }

  public static void deleteClientRecord(final SyncConfiguration config, final String clientGUID)
    throws Exception {

    final String collection = "clients";
    final URI wboURI = config.wboURI(collection, clientGUID);

    
    final SyncStorageRecordRequest r = new SyncStorageRecordRequest(wboURI);
    r.delegate = new SyncStorageRequestDelegate() {
      @Override
      public AuthHeaderProvider getAuthHeaderProvider() {
        return config.getAuthHeaderProvider();
      }

      @Override
      public String ifUnmodifiedSince() {
        return null;
      }

      @Override
      public void handleRequestSuccess(SyncStorageResponse response) {
        Logger.info(LOG_TAG, "Deleted client record with GUID " + clientGUID + " from server.");
        BaseResource.consumeEntity(response);
      }

      @Override
      public void handleRequestFailure(SyncStorageResponse response) {
        Logger.warn(LOG_TAG, "Failed to delete client record with GUID " + clientGUID + " from server.");
        try {
          Logger.warn(LOG_TAG, "Server error message was: " + response.getErrorMessage());
        } catch (Exception e) {
          
        }
        BaseResource.consumeEntity(response);
      }

      @Override
      public void handleRequestError(Exception ex) {
        
        
        Logger.error(LOG_TAG, "Got exception trying to delete client record with GUID " + clientGUID + " from server; ignoring.", ex);
      }
    };

    r.delete();
  }
}
