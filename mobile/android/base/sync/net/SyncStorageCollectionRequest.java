



package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;

import org.mozilla.gecko.background.common.log.Logger;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;









public class SyncStorageCollectionRequest extends SyncStorageRequest {
  private static final String LOG_TAG = "CollectionRequest";

  public SyncStorageCollectionRequest(URI uri) {
    super(uri);
  }

  protected volatile boolean aborting = false;

  



  public void abort() {
    aborting = true;
    try {
      this.resource.request.abort();
    } catch (Exception e) {
      
      Logger.warn(LOG_TAG, "Got exception in abort: " + e);
    }
  }

  @Override
  protected BaseResourceDelegate makeResourceDelegate(SyncStorageRequest request) {
    return new SyncCollectionResourceDelegate((SyncStorageCollectionRequest) request);
  }

  
  public class SyncCollectionResourceDelegate extends
      SyncStorageResourceDelegate {

    private static final String CONTENT_TYPE_INCREMENTAL = "application/newlines";
    private static final int FETCH_BUFFER_SIZE = 16 * 1024;   

    SyncCollectionResourceDelegate(SyncStorageCollectionRequest request) {
      super(request);
    }

    @Override
    public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
      super.addHeaders(request, client);
      request.setHeader("Accept", CONTENT_TYPE_INCREMENTAL);
      
    }

    @Override
    public void handleHttpResponse(HttpResponse response) {
      if (aborting) {
        return;
      }

      if (response.getStatusLine().getStatusCode() != 200) {
        super.handleHttpResponse(response);
        return;
      }

      HttpEntity entity = response.getEntity();
      Header contentType = entity.getContentType();
      if (!contentType.getValue().startsWith(CONTENT_TYPE_INCREMENTAL)) {
        
        super.handleHttpResponse(response);
        return;
      }

      
      
      
      
      

      
      SyncStorageCollectionRequestDelegate delegate = (SyncStorageCollectionRequestDelegate) this.request.delegate;
      InputStream content = null;
      BufferedReader br = null;
      try {
        content = entity.getContent();
        br = new BufferedReader(new InputStreamReader(content), FETCH_BUFFER_SIZE);
        String line;

        
        while (!aborting &&
               null != (line = br.readLine())) {
          try {
            delegate.handleRequestProgress(line);
          } catch (Exception ex) {
            delegate.handleRequestError(new HandleProgressException(ex));
            BaseResource.consumeEntity(entity);
            return;
          }
        }
        if (aborting) {
          
          return;
        }
      } catch (IOException ex) {
        if (!aborting) {
          delegate.handleRequestError(ex);
        }
        BaseResource.consumeEntity(entity);
        return;
      } finally {
        
        if (br != null) {
          try {
            br.close();
          } catch (IOException e) {
            
          }
        }
      }
      
      BaseResource.consumeEntity(entity);
      delegate.handleRequestSuccess(new SyncStorageResponse(response));
    }
  }
}
