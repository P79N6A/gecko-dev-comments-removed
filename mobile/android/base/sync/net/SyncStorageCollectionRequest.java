



package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;









public class SyncStorageCollectionRequest extends SyncStorageRequest {
  public SyncStorageCollectionRequest(URI uri) {
    super(uri);
  }

  @Override
  protected SyncResourceDelegate makeResourceDelegate(SyncStorageRequest request) {
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

        
        while (null != (line = br.readLine())) {
          try {
            delegate.handleRequestProgress(line);
          } catch (Exception ex) {
            delegate.handleRequestError(new HandleProgressException(ex));
            BaseResource.consumeEntity(entity);
            return;
          }
        }
      } catch (IOException ex) {
        delegate.handleRequestError(ex);
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
