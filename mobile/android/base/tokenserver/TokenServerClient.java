



package org.mozilla.gecko.tokenserver;

import java.io.IOException;
import java.net.URI;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.SkewHandler;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.UnexpectedJSONException.BadRequiredFieldJSONException;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.BaseResourceDelegate;
import org.mozilla.gecko.sync.net.BrowserIDAuthHeaderProvider;
import org.mozilla.gecko.sync.net.SyncResponse;
import org.mozilla.gecko.tokenserver.TokenServerException.TokenServerConditionsRequiredException;
import org.mozilla.gecko.tokenserver.TokenServerException.TokenServerInvalidCredentialsException;
import org.mozilla.gecko.tokenserver.TokenServerException.TokenServerMalformedRequestException;
import org.mozilla.gecko.tokenserver.TokenServerException.TokenServerMalformedResponseException;
import org.mozilla.gecko.tokenserver.TokenServerException.TokenServerUnknownServiceException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHeaders;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.message.BasicHeader;












public class TokenServerClient {
  protected static final String LOG_TAG = "TokenServerClient";

  public static final String JSON_KEY_API_ENDPOINT = "api_endpoint";
  public static final String JSON_KEY_CONDITION_URLS = "condition_urls";
  public static final String JSON_KEY_DURATION = "duration";
  public static final String JSON_KEY_ERRORS = "errors";
  public static final String JSON_KEY_ID = "id";
  public static final String JSON_KEY_KEY = "key";
  public static final String JSON_KEY_UID = "uid";

  public static final String HEADER_CONDITIONS_ACCEPTED = "X-Conditions-Accepted";
  public static final String HEADER_CLIENT_STATE = "X-Client-State";

  protected final Executor executor;
  protected final URI uri;

  public TokenServerClient(URI uri, Executor executor) {
    if (uri == null) {
      throw new IllegalArgumentException("uri must not be null");
    }
    if (executor == null) {
      throw new IllegalArgumentException("executor must not be null");
    }
    this.uri = uri;
    this.executor = executor;
  }

  protected void invokeHandleSuccess(final TokenServerClientDelegate delegate, final TokenServerToken token) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleSuccess(token);
      }
    });
  }

  protected void invokeHandleFailure(final TokenServerClientDelegate delegate, final TokenServerException e) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleFailure(e);
      }
    });
  }

  












  protected void notifyBackoff(final TokenServerClientDelegate delegate, final int backoffSeconds) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleBackoff(backoffSeconds);
      }
    });
  }

  protected void invokeHandleError(final TokenServerClientDelegate delegate, final Exception e) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleError(e);
      }
    });
  }

  public TokenServerToken processResponse(SyncResponse res) throws TokenServerException {
    int statusCode = res.getStatusCode();

    Logger.debug(LOG_TAG, "Got token response with status code " + statusCode + ".");

    
    
    final Header contentType = res.getContentType();
    if (contentType == null) {
      throw new TokenServerMalformedResponseException(null, "Non-JSON response Content-Type.");
    }

    final String type = contentType.getValue();
    if (!type.equals("application/json") &&
        !type.startsWith("application/json;")) {
      Logger.warn(LOG_TAG, "Got non-JSON response with Content-Type " +
          contentType + ". Misconfigured server?");
      throw new TokenServerMalformedResponseException(null, "Non-JSON response Content-Type.");
    }

    
    
    ExtendedJSONObject result;
    try {
      result = res.jsonObjectBody();
    } catch (Exception e) {
      Logger.debug(LOG_TAG, "Malformed token response.", e);
      throw new TokenServerMalformedResponseException(null, e);
    }

    
    if (res.getStatusCode() != 200) {
      
      
      List<ExtendedJSONObject> errorList = new ArrayList<ExtendedJSONObject>();

      if (result.containsKey(JSON_KEY_ERRORS)) {
        try {
          for (Object error : result.getArray(JSON_KEY_ERRORS)) {
            Logger.warn(LOG_TAG, "" + error);

            if (error instanceof JSONObject) {
              errorList.add(new ExtendedJSONObject((JSONObject) error));
            }
          }
        } catch (NonArrayJSONException e) {
          Logger.warn(LOG_TAG, "Got non-JSON array '" + result.getString(JSON_KEY_ERRORS) + "'.", e);
        }
      }

      if (statusCode == 400) {
        throw new TokenServerMalformedRequestException(errorList, result.toJSONString());
      }

      if (statusCode == 401) {
        throw new TokenServerInvalidCredentialsException(errorList, result.toJSONString());
      }

      
      
      
      
      
      if (statusCode == 403) {
        
        
        

        try {
          if (errorList == null || errorList.isEmpty()) {
            throw new TokenServerMalformedResponseException(errorList, "403 response without proper fields.");
          }

          ExtendedJSONObject error = errorList.get(0);

          ExtendedJSONObject condition_urls = error.getObject(JSON_KEY_CONDITION_URLS);
          if (condition_urls != null) {
            throw new TokenServerConditionsRequiredException(condition_urls);
          }
        } catch (NonObjectJSONException e) {
          Logger.warn(LOG_TAG, "Got non-JSON error object.");
        }

        throw new TokenServerMalformedResponseException(errorList, "403 response without proper fields.");
      }

      if (statusCode == 404) {
        throw new TokenServerUnknownServiceException(errorList);
      }

      
      throw new TokenServerException(errorList);
    }

    try {
      result.throwIfFieldsMissingOrMisTyped(new String[] { JSON_KEY_ID, JSON_KEY_KEY, JSON_KEY_API_ENDPOINT }, String.class);
      result.throwIfFieldsMissingOrMisTyped(new String[] { JSON_KEY_UID }, Long.class);
    } catch (BadRequiredFieldJSONException e ) {
      throw new TokenServerMalformedResponseException(null, e);
    }

    Logger.debug(LOG_TAG, "Successful token response: " + result.getString(JSON_KEY_ID));

    return new TokenServerToken(result.getString(JSON_KEY_ID),
        result.getString(JSON_KEY_KEY),
        result.get(JSON_KEY_UID).toString(),
        result.getString(JSON_KEY_API_ENDPOINT));
  }

  public static class TokenFetchResourceDelegate extends BaseResourceDelegate {
    private final TokenServerClient         client;
    private final TokenServerClientDelegate delegate;
    private final String                    assertion;
    private final String                    clientState;
    private final BaseResource              resource;
    private final boolean                   conditionsAccepted;

    public TokenFetchResourceDelegate(TokenServerClient client,
                                      BaseResource resource,
                                      TokenServerClientDelegate delegate,
                                      String assertion, String clientState,
                                      boolean conditionsAccepted) {
      super(resource);
      this.client = client;
      this.delegate = delegate;
      this.assertion = assertion;
      this.clientState = clientState;
      this.resource = resource;
      this.conditionsAccepted = conditionsAccepted;
    }

    @Override
    public String getUserAgent() {
      return delegate.getUserAgent();
    }

    @Override
    public void handleHttpResponse(HttpResponse response) {
      
      SkewHandler skewHandler = SkewHandler.getSkewHandlerForResource(resource);
      skewHandler.updateSkew(response, System.currentTimeMillis());

      
      
      SyncResponse res = new SyncResponse(response);
      final boolean includeRetryAfter = res.getStatusCode() == 503;
      int backoffInSeconds = res.totalBackoffInSeconds(includeRetryAfter);
      if (backoffInSeconds > -1) {
        client.notifyBackoff(delegate, backoffInSeconds);
      }

      try {
        TokenServerToken token = client.processResponse(res);
        client.invokeHandleSuccess(delegate, token);
      } catch (TokenServerException e) {
        client.invokeHandleFailure(delegate, e);
      }
    }

    @Override
    public void handleTransportException(GeneralSecurityException e) {
      client.invokeHandleError(delegate, e);
    }

    @Override
    public void handleHttpProtocolException(ClientProtocolException e) {
      client.invokeHandleError(delegate, e);
    }

    @Override
    public void handleHttpIOException(IOException e) {
      client.invokeHandleError(delegate, e);
    }

    @Override
    public AuthHeaderProvider getAuthHeaderProvider() {
      return new BrowserIDAuthHeaderProvider(assertion);
    }

    @Override
    public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
      String host = request.getURI().getHost();
      request.setHeader(new BasicHeader(HttpHeaders.HOST, host));
      if (clientState != null) {
        request.setHeader(new BasicHeader(HEADER_CLIENT_STATE, clientState));
      }
      if (conditionsAccepted) {
        request.addHeader(HEADER_CONDITIONS_ACCEPTED, "1");
      }
    }
  }

  public void getTokenFromBrowserIDAssertion(final String assertion,
                                             final boolean conditionsAccepted,
                                             final String clientState,
                                             final TokenServerClientDelegate delegate) {
    final BaseResource resource = new BaseResource(this.uri);
    resource.delegate = new TokenFetchResourceDelegate(this, resource, delegate,
                                                       assertion, clientState,
                                                       conditionsAccepted);
    resource.get();
  }
}
