



package org.mozilla.gecko.background.fxa;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URLEncoder;
import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.Executor;

import javax.crypto.Mac;

import org.json.simple.JSONObject;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientMalformedResponseException;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.HKDF;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.BaseResourceDelegate;
import org.mozilla.gecko.sync.net.HawkAuthHeaderProvider;
import org.mozilla.gecko.sync.net.Resource;
import org.mozilla.gecko.sync.net.SyncResponse;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpHeaders;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;

















public class FxAccountClient10 {
  protected static final String LOG_TAG = FxAccountClient10.class.getSimpleName();

  protected static final String ACCEPT_HEADER = "application/json;charset=utf-8";

  public static final String JSON_KEY_EMAIL = "email";
  public static final String JSON_KEY_KEYFETCHTOKEN = "keyFetchToken";
  public static final String JSON_KEY_SESSIONTOKEN = "sessionToken";
  public static final String JSON_KEY_UID = "uid";
  public static final String JSON_KEY_VERIFIED = "verified";
  public static final String JSON_KEY_ERROR = "error";
  public static final String JSON_KEY_MESSAGE = "message";
  public static final String JSON_KEY_INFO = "info";
  public static final String JSON_KEY_CODE = "code";
  public static final String JSON_KEY_ERRNO = "errno";

  protected static final String[] requiredErrorStringFields = { JSON_KEY_ERROR, JSON_KEY_MESSAGE, JSON_KEY_INFO };
  protected static final String[] requiredErrorLongFields = { JSON_KEY_CODE, JSON_KEY_ERRNO };

  





  protected final String serverURI;

  protected final Executor executor;

  public FxAccountClient10(String serverURI, Executor executor) {
    if (serverURI == null) {
      throw new IllegalArgumentException("Must provide a server URI.");
    }
    if (executor == null) {
      throw new IllegalArgumentException("Must provide a non-null executor.");
    }
    this.serverURI = serverURI.endsWith("/") ? serverURI : serverURI + "/";
    if (!this.serverURI.endsWith("/")) {
      throw new IllegalArgumentException("Constructed serverURI must end with a trailing slash: " + this.serverURI);
    }
    this.executor = executor;
  }

  protected BaseResource getBaseResource(String path, Map<String, String> queryParameters) throws UnsupportedEncodingException, URISyntaxException {
    if (queryParameters == null || queryParameters.isEmpty()) {
      return getBaseResource(path);
    }
    final String[] array = new String[2 * queryParameters.size()];
    int i = 0;
    for (Entry<String, String> entry : queryParameters.entrySet()) {
      array[i++] = entry.getKey();
      array[i++] = entry.getValue();
    }
    return getBaseResource(path, array);
  }

  











  protected BaseResource getBaseResource(String path, String... queryParameters) throws URISyntaxException, UnsupportedEncodingException {
    final StringBuilder sb = new StringBuilder(serverURI);
    sb.append(path);
    if (queryParameters != null) {
      int i = 0;
      while (i < queryParameters.length) {
        sb.append(i > 0 ? "&" : "?");
        final String key = queryParameters[i++];
        final String val = queryParameters[i++];
        sb.append(URLEncoder.encode(key, "UTF-8"));
        sb.append("=");
        sb.append(URLEncoder.encode(val, "UTF-8"));
      }
    }
    return new BaseResource(new URI(sb.toString()));
  }

  



  public interface RequestDelegate<T> {
    public void handleError(Exception e);
    public void handleFailure(FxAccountClientRemoteException e);
    public void handleSuccess(T result);
  }

  


  public interface CreateDelegate {
    public JSONObject getCreateBody() throws FxAccountClientException;
  }

  



  public interface AuthDelegate {
    public JSONObject getAuthStartBody() throws FxAccountClientException;
    public void onAuthStartResponse(ExtendedJSONObject body) throws FxAccountClientException;
    public JSONObject getAuthFinishBody() throws FxAccountClientException;

    public byte[] getSharedBytes() throws FxAccountClientException;
  }

  


  public static class TwoTokens {
    public final byte[] keyFetchToken;
    public final byte[] sessionToken;
    public TwoTokens(byte[] keyFetchToken, byte[] sessionToken) {
      this.keyFetchToken = keyFetchToken;
      this.sessionToken = sessionToken;
    }
  }

  


  public static class TwoKeys {
    public final byte[] kA;
    public final byte[] wrapkB;
    public TwoKeys(byte[] kA, byte[] wrapkB) {
      this.kA = kA;
      this.wrapkB = wrapkB;
    }
  }

  protected <T> void invokeHandleError(final RequestDelegate<T> delegate, final Exception e) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        delegate.handleError(e);
      }
    });
  }

  







  protected abstract class ResourceDelegate<T> extends BaseResourceDelegate {
    protected abstract void handleSuccess(final int status, HttpResponse response, final ExtendedJSONObject body);

    protected final RequestDelegate<T> delegate;

    protected final byte[] tokenId;
    protected final byte[] reqHMACKey;
    protected final SkewHandler skewHandler;

    


    public ResourceDelegate(final Resource resource, final RequestDelegate<T> delegate) {
      this(resource, delegate, null, null);
    }

    





    public ResourceDelegate(final Resource resource, final RequestDelegate<T> delegate, final byte[] tokenId, final byte[] reqHMACKey) {
      super(resource);
      this.delegate = delegate;
      this.reqHMACKey = reqHMACKey;
      this.tokenId = tokenId;
      this.skewHandler = SkewHandler.getSkewHandlerForResource(resource);
    }

    @Override
    public AuthHeaderProvider getAuthHeaderProvider() {
      if (tokenId != null && reqHMACKey != null) {
        
        final boolean includePayloadVerificationHash = true;
        return new HawkAuthHeaderProvider(Utils.byte2Hex(tokenId), reqHMACKey, includePayloadVerificationHash, skewHandler.getSkewInSeconds());
      }
      return super.getAuthHeaderProvider();
    }

    @Override
    public String getUserAgent() {
      return FxAccountConstants.USER_AGENT;
    }

    @Override
    public void handleHttpResponse(HttpResponse response) {
      try {
        final int status = validateResponse(response);
        skewHandler.updateSkew(response, now());
        invokeHandleSuccess(status, response);
      } catch (FxAccountClientRemoteException e) {
        if (!skewHandler.updateSkew(response, now())) {
          
          skewHandler.resetSkew();
        }
        invokeHandleFailure(e);
      }
    }

    protected void invokeHandleFailure(final FxAccountClientRemoteException e) {
      executor.execute(new Runnable() {
        @Override
        public void run() {
          delegate.handleFailure(e);
        }
      });
    }

    protected void invokeHandleSuccess(final int status, final HttpResponse response) {
      executor.execute(new Runnable() {
        @Override
        public void run() {
          try {
            ExtendedJSONObject body = new SyncResponse(response).jsonObjectBody();
            ResourceDelegate.this.handleSuccess(status, response, body);
          } catch (Exception e) {
            delegate.handleError(e);
          }
        }
      });
    }

    @Override
    public void handleHttpProtocolException(final ClientProtocolException e) {
      invokeHandleError(delegate, e);
    }

    @Override
    public void handleHttpIOException(IOException e) {
      invokeHandleError(delegate, e);
    }

    @Override
    public void handleTransportException(GeneralSecurityException e) {
      invokeHandleError(delegate, e);
    }

    @Override
    public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
      super.addHeaders(request, client);

      
      final Locale locale = Locale.getDefault();
      request.addHeader(HttpHeaders.ACCEPT_LANGUAGE, Utils.getLanguageTag(locale));
      request.addHeader(HttpHeaders.ACCEPT, ACCEPT_HEADER);
    }
  }

  protected <T> void post(BaseResource resource, final JSONObject requestBody, final RequestDelegate<T> delegate) {
    try {
      if (requestBody == null) {
        resource.post((HttpEntity) null);
      } else {
        resource.post(requestBody);
      }
    } catch (UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }
  }

  @SuppressWarnings("static-method")
  public long now() {
    return System.currentTimeMillis();
  }

  








  public static int validateResponse(HttpResponse response) throws FxAccountClientRemoteException {
    final int status = response.getStatusLine().getStatusCode();
    if (status == 200) {
      return status;
    }
    int code;
    int errno;
    String error;
    String message;
    String info;
    ExtendedJSONObject body;
    try {
      body = new SyncStorageResponse(response).jsonObjectBody();
      body.throwIfFieldsMissingOrMisTyped(requiredErrorStringFields, String.class);
      body.throwIfFieldsMissingOrMisTyped(requiredErrorLongFields, Long.class);
      code = body.getLong(JSON_KEY_CODE).intValue();
      errno = body.getLong(JSON_KEY_ERRNO).intValue();
      error = body.getString(JSON_KEY_ERROR);
      message = body.getString(JSON_KEY_MESSAGE);
      info = body.getString(JSON_KEY_INFO);
    } catch (Exception e) {
      throw new FxAccountClientMalformedResponseException(response);
    }
    throw new FxAccountClientRemoteException(response, code, errno, error, message, info, body);
  }

  public void createAccount(final String email, final byte[] stretchedPWBytes,
      final String srpSalt, final String mainSalt,
      final RequestDelegate<String> delegate) {
    try {
      createAccount(new FxAccount10CreateDelegate(email, stretchedPWBytes, srpSalt, mainSalt), delegate);
    } catch (final Exception e) {
      invokeHandleError(delegate, e);
      return;
    }
  }

  protected void createAccount(final CreateDelegate createDelegate, final RequestDelegate<String> delegate) {
    JSONObject body = null;
    try {
      body = createDelegate.getCreateBody();
    } catch (FxAccountClientException e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("account/create");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<String>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        String uid = body.getString("uid");
        if (uid == null) {
          delegate.handleError(new FxAccountClientException("uid must be a non-null string"));
          return;
        }
        delegate.handleSuccess(uid);
      }
    };
    post(resource, body, delegate);
  }

  protected void authStart(final AuthDelegate authDelegate, final RequestDelegate<AuthDelegate> delegate) {
    JSONObject body;
    try {
      body = authDelegate.getAuthStartBody();
    } catch (FxAccountClientException e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("auth/start");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<AuthDelegate>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          authDelegate.onAuthStartResponse(body);
          delegate.handleSuccess(authDelegate);
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    post(resource, body, delegate);
  }

  protected void authFinish(final AuthDelegate authDelegate, RequestDelegate<byte[]> delegate) {
    JSONObject body;
    try {
      body = authDelegate.getAuthFinishBody();
    } catch (FxAccountClientException e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("auth/finish");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<byte[]>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          byte[] authToken = new byte[32];
          unbundleBody(body, authDelegate.getSharedBytes(), FxAccountUtils.KW("auth/finish"), authToken);
          delegate.handleSuccess(authToken);
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    post(resource, body, delegate);
  }

  public void login(final String email, final byte[] stretchedPWBytes, final RequestDelegate<byte[]> delegate) {
    login(new FxAccount10AuthDelegate(email, stretchedPWBytes), delegate);
  }

  protected void login(final AuthDelegate authDelegate, final RequestDelegate<byte[]> delegate) {
    authStart(authDelegate, new RequestDelegate<AuthDelegate>() {
      @Override
      public void handleSuccess(AuthDelegate srpSession) {
        authFinish(srpSession, delegate);
      }

      @Override
      public void handleError(final Exception e) {
        invokeHandleError(delegate, e);
        return;
      }

      @Override
      public void handleFailure(final FxAccountClientRemoteException e) {
        executor.execute(new Runnable() {
          @Override
          public void run() {
            delegate.handleFailure(e);
          }
        });
      }
    });
  }

  public void sessionCreate(byte[] authToken, final RequestDelegate<TwoTokens> delegate) {
    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    final byte[] requestKey = new byte[32];
    try {
      HKDF.deriveMany(authToken, new byte[0], FxAccountUtils.KW("authToken"), tokenId, reqHMACKey, requestKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("session/create");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<TwoTokens>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          byte[] keyFetchToken = new byte[32];
          byte[] sessionToken = new byte[32];
          unbundleBody(body, requestKey, FxAccountUtils.KW("session/create"), keyFetchToken, sessionToken);
          delegate.handleSuccess(new TwoTokens(keyFetchToken, sessionToken));
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    post(resource, null, delegate);
  }

  public void sessionDestroy(byte[] sessionToken, final RequestDelegate<Void> delegate) {
    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    try {
      HKDF.deriveMany(sessionToken, new byte[0], FxAccountUtils.KW("sessionToken"), tokenId, reqHMACKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("session/destroy");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<Void>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        delegate.handleSuccess(null);
      }
    };
    post(resource, null, delegate);
  }

  


  protected void unbundleBytes(byte[] bundleBytes, byte[] respHMACKey, byte[] respXORKey, byte[]... rest)
      throws InvalidKeyException, NoSuchAlgorithmException, FxAccountClientException {
    if (bundleBytes.length < 32) {
      throw new IllegalArgumentException("input bundle must include HMAC");
    }
    int len = respXORKey.length;
    if (bundleBytes.length != len + 32) {
      throw new IllegalArgumentException("input bundle and XOR key with HMAC have different lengths");
    }
    int left = len;
    for (byte[] array : rest) {
      left -= array.length;
    }
    if (left != 0) {
      throw new IllegalArgumentException("XOR key and total output arrays have different lengths");
    }

    byte[] ciphertext = new byte[len];
    byte[] HMAC = new byte[32];
    System.arraycopy(bundleBytes, 0, ciphertext, 0, len);
    System.arraycopy(bundleBytes, len, HMAC, 0, 32);

    Mac hmacHasher = HKDF.makeHMACHasher(respHMACKey);
    byte[] computedHMAC = hmacHasher.doFinal(ciphertext);
    if (!Arrays.equals(computedHMAC, HMAC)) {
      throw new FxAccountClientException("Bad message HMAC");
    }

    int offset = 0;
    for (byte[] array : rest) {
      for (int i = 0; i < array.length; i++) {
        array[i] = (byte) (respXORKey[offset + i] ^ ciphertext[offset + i]);
      }
      offset += array.length;
    }
  }

  protected void unbundleBody(ExtendedJSONObject body, byte[] requestKey, byte[] ctxInfo, byte[]... rest) throws Exception {
    int length = 0;
    for (byte[] array : rest) {
      length += array.length;
    }

    if (body == null) {
      throw new FxAccountClientException("body must be non-null");
    }
    String bundle = body.getString("bundle");
    if (bundle == null) {
      throw new FxAccountClientException("bundle must be a non-null string");
    }
    byte[] bundleBytes = Utils.hex2Byte(bundle);

    final byte[] respHMACKey = new byte[32];
    final byte[] respXORKey = new byte[length];
    HKDF.deriveMany(requestKey, new byte[0], ctxInfo, respHMACKey, respXORKey);
    unbundleBytes(bundleBytes, respHMACKey, respXORKey, rest);
  }

  public void keys(byte[] keyFetchToken, final RequestDelegate<TwoKeys> delegate) {
    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    final byte[] requestKey = new byte[32];
    try {
      HKDF.deriveMany(keyFetchToken, new byte[0], FxAccountUtils.KW("keyFetchToken"), tokenId, reqHMACKey, requestKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("account/keys");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<TwoKeys>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          byte[] kA = new byte[FxAccountUtils.CRYPTO_KEY_LENGTH_BYTES];
          byte[] wrapkB = new byte[FxAccountUtils.CRYPTO_KEY_LENGTH_BYTES];
          unbundleBody(body, requestKey, FxAccountUtils.KW("account/keys"), kA, wrapkB);
          delegate.handleSuccess(new TwoKeys(kA, wrapkB));
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    resource.get();
  }

  


  public static class StatusResponse {
    public final String email;
    public final boolean verified;
    public StatusResponse(String email, boolean verified) {
      this.email = email;
      this.verified = verified;
    }
  }

  












  public void status(byte[] sessionToken, final RequestDelegate<StatusResponse> delegate) {
    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    final byte[] requestKey = new byte[32];
    try {
      HKDF.deriveMany(sessionToken, new byte[0], FxAccountUtils.KW("sessionToken"), tokenId, reqHMACKey, requestKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("recovery_email/status");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<StatusResponse>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          String[] requiredStringFields = new String[] { JSON_KEY_EMAIL };
          body.throwIfFieldsMissingOrMisTyped(requiredStringFields, String.class);
          String email = body.getString(JSON_KEY_EMAIL);
          Boolean verified = body.getBoolean(JSON_KEY_VERIFIED);
          delegate.handleSuccess(new StatusResponse(email, verified));
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    resource.get();
  }

  @SuppressWarnings("unchecked")
  public void sign(final byte[] sessionToken, final ExtendedJSONObject publicKey, long durationInMilliseconds, final RequestDelegate<String> delegate) {
    final JSONObject body = new JSONObject();
    body.put("publicKey", publicKey);
    body.put("duration", durationInMilliseconds);

    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    try {
      HKDF.deriveMany(sessionToken, new byte[0], FxAccountUtils.KW("sessionToken"), tokenId, reqHMACKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("certificate/sign");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<String>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        String cert = body.getString("cert");
        if (cert == null) {
          delegate.handleError(new FxAccountClientException("cert must be a non-null string"));
          return;
        }
        delegate.handleSuccess(cert);
      }
    };
    post(resource, body, delegate);
  }

  







  public void resendCode(byte[] sessionToken, final RequestDelegate<Void> delegate) {
    final byte[] tokenId = new byte[32];
    final byte[] reqHMACKey = new byte[32];
    final byte[] requestKey = new byte[32];
    try {
      HKDF.deriveMany(sessionToken, new byte[0], FxAccountUtils.KW("sessionToken"), tokenId, reqHMACKey, requestKey);
    } catch (Exception e) {
      invokeHandleError(delegate, e);
      return;
    }

    BaseResource resource;
    try {
      resource = getBaseResource("recovery_email/resend_code");
    } catch (URISyntaxException | UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<Void>(resource, delegate, tokenId, reqHMACKey) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          delegate.handleSuccess(null);
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    post(resource, new JSONObject(), delegate);
  }

  











  @SuppressWarnings("unchecked")
  public void resendUnlockCode(final byte[] emailUTF8, final RequestDelegate<Void> delegate) {
    final BaseResource resource;
    final JSONObject body = new JSONObject();
    try {
      resource = getBaseResource("account/unlock/resend_code");
      body.put("email", new String(emailUTF8, "UTF-8"));
    } catch (URISyntaxException e) {
      invokeHandleError(delegate, e);
      return;
    } catch (UnsupportedEncodingException e) {
      invokeHandleError(delegate, e);
      return;
    }

    resource.delegate = new ResourceDelegate<Void>(resource, delegate) {
      @Override
      public void handleSuccess(int status, HttpResponse response, ExtendedJSONObject body) {
        try {
          delegate.handleSuccess(null);
          return;
        } catch (Exception e) {
          delegate.handleError(e);
          return;
        }
      }
    };
    post(resource, body, delegate);
  }
}
