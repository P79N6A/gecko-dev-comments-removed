




































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

    SyncCollectionResourceDelegate(SyncStorageCollectionRequest request) {
      super(request);
    }

    @Override
    public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
      super.addHeaders(request, client);
      request.setHeader("Accept", "application/newlines");
      
    }

    @Override
    public void handleHttpResponse(HttpResponse response) {
      if (response.getStatusLine().getStatusCode() != 200) {
        super.handleHttpResponse(response);
        return;
      }

      HttpEntity entity = response.getEntity();
      Header contentType = entity.getContentType();
      System.out.println("content type is " + contentType.getValue());
      if (!contentType.getValue().startsWith("application/newlines")) {
        
        super.handleHttpResponse(response);
        return;
      }

      
      
      
      
      

      
      SyncStorageCollectionRequestDelegate delegate = (SyncStorageCollectionRequestDelegate) this.request.delegate;
      InputStream content = null;
      BufferedReader br = null;
      try {
        content = entity.getContent();
        int bufSize = 1024 * 1024;         
        br = new BufferedReader(new InputStreamReader(content), bufSize);
        String line;

        
        while (null != (line = br.readLine())) {
          try {
            delegate.handleRequestProgress(line);
          } catch (Exception ex) {
            delegate.handleRequestError(new HandleProgressException(ex));
            SyncResourceDelegate.consumeEntity(entity);
            return;
          }
        }
      } catch (IOException ex) {
        delegate.handleRequestError(ex);
        SyncResourceDelegate.consumeEntity(entity);
        return;
      } finally {
        
        if (br != null) {
          try {
            br.close();
          } catch (IOException e) {
            
          }
        }
      }
      
      SyncResourceDelegate.consumeEntity(entity);
      delegate.handleRequestSuccess(new SyncStorageResponse(response));
    }
  }
}