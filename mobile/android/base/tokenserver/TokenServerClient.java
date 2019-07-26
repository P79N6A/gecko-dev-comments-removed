



package org.mozilla.gecko.tokenserver;

import java.io.IOException;
import java.net.URI;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonArrayJSONException;
import org.mozilla.gecko.sync.NonObjectJSONException;
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

  protected void invokeHandleError(final TokenServerClientDelegate delegate, final Exception e) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleError(e);
      }
    });
  }

  public TokenServerToken processResponse(HttpResponse response)
      throws TokenServerException {
    SyncResponse res = new SyncResponse(response);
    int statusCode = res.getStatusCode();

    Logger.debug(LOG_TAG, "Got token response with status code " + statusCode + ".");

    
    
    String contentType = response.getEntity().getContentType().getValue();
    if (contentType != "application/json" && !contentType.startsWith("application/json;")) {
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

    
    for (String k : new String[] { JSON_KEY_ID, JSON_KEY_KEY, JSON_KEY_API_ENDPOINT }) {
      Object value = result.get(k);
      if (value == null) {
        throw new TokenServerMalformedResponseException(null, "Expected key not present in result: " + k);
      }
      if (!(value instanceof String)) {
        throw new TokenServerMalformedResponseException(null, "Value for key not a string in result: " + k);
      }
    }

    
    for (String k : new String[] { JSON_KEY_UID }) {
      Object value = result.get(k);
      if (value == null) {
        throw new TokenServerMalformedResponseException(null, "Expected key not present in result: " + k);
      }
      if (!(value instanceof Long)) {
        throw new TokenServerMalformedResponseException(null, "Value for key not a string in result: " + k);
      }
    }

    Logger.debug(LOG_TAG, "Successful token response: " + result.getString(JSON_KEY_ID));

    return new TokenServerToken(result.getString(JSON_KEY_ID),
        result.getString(JSON_KEY_KEY),
        result.get(JSON_KEY_UID).toString(),
        result.getString(JSON_KEY_API_ENDPOINT));
  }

  public void getTokenFromBrowserIDAssertion(final String assertion, final boolean conditionsAccepted,
      final TokenServerClientDelegate delegate) {
    BaseResource r = new BaseResource(uri);

    r.delegate = new BaseResourceDelegate(r) {
      @Override
      public void handleHttpResponse(HttpResponse response) {
        try {
          TokenServerToken token = processResponse(response);
          invokeHandleSuccess(delegate, token);
        } catch (TokenServerException e) {
          invokeHandleFailure(delegate, e);
        }
      }

      @Override
      public void handleTransportException(GeneralSecurityException e) {
        invokeHandleError(delegate, e);
      }

      @Override
      public void handleHttpProtocolException(ClientProtocolException e) {
        invokeHandleError(delegate, e);
      }

      @Override
      public void handleHttpIOException(IOException e) {
        invokeHandleError(delegate, e);
      }

      @Override
      public AuthHeaderProvider getAuthHeaderProvider() {
        return new BrowserIDAuthHeaderProvider(assertion);
      }

      @Override
      public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
        String host = request.getURI().getHost();
        request.setHeader(new BasicHeader("Host", host));

        if (conditionsAccepted) {
          request.addHeader("X-Conditions-Accepted", "1");
        }
      }
    };

    r.get();
  }
}
