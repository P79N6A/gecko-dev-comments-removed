



package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URLEncoder;
import java.util.concurrent.Executor;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.net.BaseResource;

import ch.boye.httpclientandroidlib.HttpResponse;

public class FxAccountClient20 extends FxAccountClient10 implements FxAccountClient {
  protected static final String[] LOGIN_RESPONSE_REQUIRED_STRING_FIELDS = new String[] { JSON_KEY_UID, JSON_KEY_SESSIONTOKEN };
  protected static final String[] LOGIN_RESPONSE_REQUIRED_STRING_FIELDS_KEYS = new String[] { JSON_KEY_UID, JSON_KEY_SESSIONTOKEN, JSON_KEY_KEYFETCHTOKEN, };
  protected static final String[] LOGIN_RESPONSE_REQUIRED_BOOLEAN_FIELDS = new String[] { JSON_KEY_VERIFIED };

  public FxAccountClient20(String serverURI, Executor executor) {
    super(serverURI, executor);
  }

  






  public static class LoginResponse {
    public final String remoteEmail;
    public final String uid;
    public final byte[] sessionToken;
    public final boolean verified;
    public final byte[] keyFetchToken;

    public LoginResponse(String remoteEmail, String uid, boolean verified, byte[] sessionToken, byte[] keyFetchToken) {
      this.remoteEmail = remoteEmail;
      this.uid = uid;
      this.verified = verified;
      this.sessionToken = sessionToken;
      this.keyFetchToken = keyFetchToken;
    }
  }

  
  public void login(final byte[] emailUTF8, final byte[] quickStretchedPW, final boolean getKeys,
      final RequestDelegate<LoginResponse> delegate) {
    BaseResource resource;
    JSONObject body;
    final String path = getKeys ? "account/login?keys=true" : "account/login";
    try {
      resource = new BaseResource(new URI(serverURI + path));
      body = new FxAccount20LoginDelegate(emailUTF8, quickStretchedPW).getCreateBody();
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<LoginResponse>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          final String[] requiredStringFields = getKeys ? LOGIN_RESPONSE_REQUIRED_STRING_FIELDS_KEYS : LOGIN_RESPONSE_REQUIRED_STRING_FIELDS;
          body.throwIfFieldsMissingOrMisTyped(requiredStringFields, String.class);

          final String[] requiredBooleanFields = LOGIN_RESPONSE_REQUIRED_BOOLEAN_FIELDS;
          body.throwIfFieldsMissingOrMisTyped(requiredBooleanFields, Boolean.class);

          String uid = body.getString(JSON_KEY_UID);
          boolean verified = body.getBoolean(JSON_KEY_VERIFIED);
          byte[] sessionToken = Utils.hex2Byte(body.getString(JSON_KEY_SESSIONTOKEN));
          byte[] keyFetchToken = null;
          if (getKeys) {
            keyFetchToken = Utils.hex2Byte(body.getString(JSON_KEY_KEYFETCHTOKEN));
          }
          LoginResponse loginResponse = new LoginResponse(new String(emailUTF8, "UTF-8"), uid, verified, sessionToken, keyFetchToken);

          delegate.handleSuccess(loginResponse);
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };

    post(resource, body, delegate);
  }

  





  protected URI getCreateAccountURI(final boolean getKeys, final String service) throws UnsupportedEncodingException, URISyntaxException {
    if (service == null) {
      throw new IllegalArgumentException("service must not be null");
    }
    final StringBuilder sb = new StringBuilder(serverURI); 
    sb.append("account/create?service=");
    
    sb.append(URLEncoder.encode(service, "UTF-8"));
    if (getKeys) {
      sb.append("&keys=true");
    }
    return new URI(sb.toString());
  }

  public void createAccount(final byte[] emailUTF8, final byte[] quickStretchedPW,
      final boolean getKeys,
      final boolean preVerified,
      final String service,
      final RequestDelegate<LoginResponse> delegate) {
    final BaseResource resource;
    final JSONObject body;
    try {
      resource = new BaseResource(getCreateAccountURI(getKeys, service));
      body = new FxAccount20CreateDelegate(emailUTF8, quickStretchedPW, preVerified).getCreateBody();
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    
    resource.delegate = new ResourceDelegate<LoginResponse>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          final String[] requiredStringFields = getKeys ? LOGIN_RESPONSE_REQUIRED_STRING_FIELDS_KEYS : LOGIN_RESPONSE_REQUIRED_STRING_FIELDS;
          body.throwIfFieldsMissingOrMisTyped(requiredStringFields, String.class);

          String uid = body.getString(JSON_KEY_UID);
          boolean verified = false; 
          Boolean tempVerified = body.getBoolean(JSON_KEY_VERIFIED);
          if (tempVerified != null) {
            verified = tempVerified;
          }
          byte[] sessionToken = Utils.hex2Byte(body.getString(JSON_KEY_SESSIONTOKEN));
          byte[] keyFetchToken = null;
          if (getKeys) {
            keyFetchToken = Utils.hex2Byte(body.getString(JSON_KEY_KEYFETCHTOKEN));
          }
          LoginResponse loginResponse = new LoginResponse(new String(emailUTF8, "UTF-8"), uid, verified, sessionToken, keyFetchToken);

          delegate.handleSuccess(loginResponse);
        } catch (Exception e) {
          delegate.handleError(e);
        }
      }
    };

    post(resource, body, delegate);
  }

  @Override
  public void createAccountAndGetKeys(byte[] emailUTF8, PasswordStretcher passwordStretcher, RequestDelegate<LoginResponse> delegate) {
    try {
      byte[] quickStretchedPW = passwordStretcher.getQuickStretchedPW(emailUTF8);
      createAccount(emailUTF8, quickStretchedPW, true, false, "sync", delegate);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
    }
  }

  @Override
  public void loginAndGetKeys(byte[] emailUTF8, PasswordStretcher passwordStretcher, RequestDelegate<LoginResponse> delegate) {
    login(emailUTF8, passwordStretcher, true, delegate);
  }

  























  public void login(final byte[] emailUTF8, final PasswordStretcher stretcher, final boolean getKeys,
      final RequestDelegate<LoginResponse> delegate) {
    byte[] quickStretchedPW;
    try {
      FxAccountUtils.pii(LOG_TAG, "Trying user provided email: '" + new String(emailUTF8, "UTF-8") + "'" );
      quickStretchedPW = stretcher.getQuickStretchedPW(emailUTF8);
    } catch (Exception e) {
      delegate.handleError(e);
      return;
    }

    this.login(emailUTF8, quickStretchedPW, getKeys, new RequestDelegate<LoginResponse>() {
      @Override
      public void handleSuccess(LoginResponse result) {
        delegate.handleSuccess(result);
      }

      @Override
      public void handleError(Exception e) {
        delegate.handleError(e);
      }

      @Override
      public void handleFailure(FxAccountClientRemoteException e) {
        String alternateEmail = e.body.getString(JSON_KEY_EMAIL);
        if (!e.isBadEmailCase() || alternateEmail == null) {
          delegate.handleFailure(e);
          return;
        };

        Logger.info(LOG_TAG, "Server returned alternate email; retrying login with provided email.");
        FxAccountUtils.pii(LOG_TAG, "Trying server provided email: '" + alternateEmail + "'" );

        try {
          
          
          byte[] alternateEmailUTF8 = alternateEmail.getBytes("UTF-8");
          byte[] alternateQuickStretchedPW = stretcher.getQuickStretchedPW(alternateEmailUTF8);
          login(alternateEmailUTF8, alternateQuickStretchedPW, getKeys, delegate);
        } catch (Exception innerException) {
          delegate.handleError(innerException);
          return;
        }
      }
    });
  }
}
