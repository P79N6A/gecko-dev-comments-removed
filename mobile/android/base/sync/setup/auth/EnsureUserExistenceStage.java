



package org.mozilla.gecko.sync.setup.auth;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.BaseResourceDelegate;
import org.mozilla.gecko.sync.setup.Constants;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;

public class EnsureUserExistenceStage implements AuthenticatorStage {
  private final String LOG_TAG = "EnsureUserExistence";

  public interface EnsureUserExistenceStageDelegate {
    public void handleSuccess();
    public void handleFailure(AuthenticationResult result);
    public void handleError(Exception e);
  }
  @Override
  public void execute(final AccountAuthenticator aa) throws URISyntaxException,
      UnsupportedEncodingException {
    final EnsureUserExistenceStageDelegate callbackDelegate = new EnsureUserExistenceStageDelegate() {

      @Override
      public void handleSuccess() {
        
        Logger.debug(LOG_TAG, "handleSuccess()");
        aa.runNextStage();
      }

      @Override
      public void handleFailure(AuthenticationResult result) {
        aa.abort(result, new Exception("Failure in EnsureUser"));
      }

      @Override
      public void handleError(Exception e) {
        Logger.info(LOG_TAG, "Error checking for user existence.");
        aa.abort(AuthenticationResult.FAILURE_SERVER, e);
      }

    };

    String userRequestUrl = aa.nodeServer + Constants.AUTH_NODE_PATHNAME + Constants.AUTH_NODE_VERSION + aa.username;
    final BaseResource httpResource = new BaseResource(userRequestUrl);
    httpResource.delegate = new BaseResourceDelegate(httpResource) {

      @Override
      public void handleHttpResponse(HttpResponse response) {
        int statusCode = response.getStatusLine().getStatusCode();
        switch(statusCode) {
        case 200:
          try {
            InputStream content = response.getEntity().getContent();
            BufferedReader reader = new BufferedReader(new InputStreamReader(content, "UTF-8"), 1024);
            String inUse = reader.readLine();
            BaseResource.consumeReader(reader);
            reader.close();
            
            if (inUse.equals("1")) { 
              callbackDelegate.handleSuccess();
            } else { 
              Logger.info(LOG_TAG, "No such user.");
              callbackDelegate.handleFailure(AuthenticationResult.FAILURE_USERNAME);
            }
          } catch (Exception e) {
            Logger.error(LOG_TAG, "Failure in content parsing.", e);
            callbackDelegate.handleFailure(AuthenticationResult.FAILURE_OTHER);
          }
          break;
        default: 
          callbackDelegate.handleFailure(AuthenticationResult.FAILURE_OTHER);
        }
        Logger.debug(LOG_TAG, "Consuming entity.");
        BaseResource.consumeEntity(response.getEntity());
      }

      @Override
      public void handleHttpProtocolException(ClientProtocolException e) {
        callbackDelegate.handleError(e);
      }

      @Override
      public void handleHttpIOException(IOException e) {
        callbackDelegate.handleError(e);
      }

      @Override
      public void handleTransportException(GeneralSecurityException e) {
        callbackDelegate.handleError(e);
      }

    };
    
    AccountAuthenticator.runOnThread(new Runnable() {

      @Override
      public void run() {
        httpResource.get();
      }
    });
  }

}
