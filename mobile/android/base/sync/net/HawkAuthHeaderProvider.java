



package org.mozilla.gecko.sync.net;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.Utils;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;









public class HawkAuthHeaderProvider implements AuthHeaderProvider {
  public static final String LOG_TAG = HawkAuthHeaderProvider.class.getSimpleName();

  public static final int HAWK_HEADER_VERSION = 1;

  protected static final int NONCE_LENGTH_IN_BYTES = 8;
  protected static final String HMAC_SHA256_ALGORITHM = "hmacSHA256";

  protected final String id;
  protected final byte[] key;
  protected final boolean includePayloadHash;
  protected final long skewSeconds;

  
























  public HawkAuthHeaderProvider(String id, byte[] key, boolean includePayloadHash, long skewSeconds) {
    if (id == null) {
      throw new IllegalArgumentException("id must not be null");
    }
    if (key == null) {
      throw new IllegalArgumentException("key must not be null");
    }
    this.id = id;
    this.key = key;
    this.includePayloadHash = includePayloadHash;
    this.skewSeconds = skewSeconds;
  }

  


  @SuppressWarnings("static-method")
  protected long now() {
    return System.currentTimeMillis();
  }

  



  protected long getTimestampSeconds() {
    return (now() / 1000) + skewSeconds;
  }

  @Override
  public Header getAuthHeader(HttpRequestBase request, BasicHttpContext context, DefaultHttpClient client) throws GeneralSecurityException {
    long timestamp = getTimestampSeconds();
    String nonce = Base64.encodeBase64String(Utils.generateRandomBytes(NONCE_LENGTH_IN_BYTES));
    String extra = "";

    try {
      return getAuthHeader(request, context, client, timestamp, nonce, extra, this.includePayloadHash);
    } catch (Exception e) {
      
      throw new GeneralSecurityException(e);
    }
  }

  







  protected Header getAuthHeader(HttpRequestBase request, BasicHttpContext context, DefaultHttpClient client,
      long timestamp, String nonce, String extra, boolean includePayloadHash)
          throws InvalidKeyException, NoSuchAlgorithmException, IOException {
    if (timestamp < 0) {
      throw new IllegalArgumentException("timestamp must contain only [0-9].");
    }

    if (nonce == null) {
      throw new IllegalArgumentException("nonce must not be null.");
    }
    if (nonce.length() == 0) {
      throw new IllegalArgumentException("nonce must not be empty.");
    }

    String payloadHash = null;
    if (includePayloadHash) {
      payloadHash = getPayloadHashString(request);
    } else {
      Logger.debug(LOG_TAG, "Configured to not include payload hash for this request.");
    }

    String app = null;
    String dlg = null;
    String requestString = getRequestString(request, "header", timestamp, nonce, payloadHash, extra, app, dlg);
    String macString = getSignature(requestString.getBytes("UTF-8"), this.key);

    StringBuilder sb = new StringBuilder();
    sb.append("Hawk id=\"");
    sb.append(this.id);
    sb.append("\", ");
    sb.append("ts=\"");
    sb.append(timestamp);
    sb.append("\", ");
    sb.append("nonce=\"");
    sb.append(nonce);
    sb.append("\", ");
    if (payloadHash != null) {
      sb.append("hash=\"");
      sb.append(payloadHash);
      sb.append("\", ");
    }
    if (extra != null && extra.length() > 0) {
      sb.append("ext=\"");
      sb.append(escapeExtraHeaderAttribute(extra));
      sb.append("\", ");
    }
    sb.append("mac=\"");
    sb.append(macString);
    sb.append("\"");

    return new BasicHeader("Authorization", sb.toString());
  }

  














  protected static String getPayloadHashString(HttpRequestBase request)
      throws UnsupportedEncodingException, NoSuchAlgorithmException, IOException, IllegalArgumentException {
    final boolean shouldComputePayloadHash = request instanceof HttpEntityEnclosingRequest;
    if (!shouldComputePayloadHash) {
      Logger.debug(LOG_TAG, "Not computing payload verification hash for non-enclosing request.");
      return null;
    }
    if (!(request instanceof HttpEntityEnclosingRequest)) {
      throw new IllegalArgumentException("Cannot compute payload verification hash for enclosing request without an entity");
    }
    final HttpEntity entity = ((HttpEntityEnclosingRequest) request).getEntity();
    if (entity == null) {
      throw new IllegalArgumentException("Cannot compute payload verification hash for enclosing request with a null entity");
    }
    return Base64.encodeBase64String(getPayloadHash(entity));
  }

  










  protected static String escapeExtraHeaderAttribute(String extra) {
    return extra.replaceAll("\\\\", "\\\\").replaceAll("\"", "\\\"");
  }

  











  protected static String escapeExtraString(String extra) {
    return extra.replaceAll("\\\\", "\\\\").replaceAll("\n", "\\n");
  }

  





  protected static String getBaseContentType(Header contentTypeHeader) {
    if (contentTypeHeader == null) {
      throw new IllegalArgumentException("contentTypeHeader must not be null.");
    }
    String contentType = contentTypeHeader.getValue();
    if (contentType == null) {
      throw new IllegalArgumentException("contentTypeHeader value must not be null.");
    }
    int index = contentType.indexOf(";");
    if (index < 0) {
      return contentType.trim();
    }
    return contentType.substring(0, index).trim();
  }

  













  protected static byte[] getPayloadHash(HttpEntity entity) throws UnsupportedEncodingException, IOException, NoSuchAlgorithmException {
    if (!entity.isRepeatable()) {
      throw new IllegalArgumentException("entity must be repeatable");
    }
    final MessageDigest digest = MessageDigest.getInstance("SHA-256");
    digest.update(("hawk." + HAWK_HEADER_VERSION + ".payload\n").getBytes("UTF-8"));
    digest.update(getBaseContentType(entity.getContentType()).getBytes("UTF-8"));
    digest.update("\n".getBytes("UTF-8"));
    InputStream stream = entity.getContent();
    try {
      int numRead;
      byte[] buffer = new byte[4096];
      while (-1 != (numRead = stream.read(buffer))) {
        if (numRead > 0) {
          digest.update(buffer, 0, numRead);
        }
      }
      digest.update("\n".getBytes("UTF-8")); 
      return digest.digest();
    } finally {
      stream.close();
    }
  }

  






  protected static String getRequestString(HttpUriRequest request, String type, long timestamp, String nonce, String hash, String extra, String app, String dlg) {
    String method = request.getMethod().toUpperCase(Locale.US);

    URI uri = request.getURI();
    String host = uri.getHost();

    String path = uri.getRawPath();
    if (uri.getRawQuery() != null) {
      path += "?";
      path += uri.getRawQuery();
    }
    if (uri.getRawFragment() != null) {
      path += "#";
      path += uri.getRawFragment();
    }

    int port = uri.getPort();
    String scheme = uri.getScheme();
    if (port != -1) {
    } else if ("http".equalsIgnoreCase(scheme)) {
      port = 80;
    } else if ("https".equalsIgnoreCase(scheme)) {
      port = 443;
    } else {
      throw new IllegalArgumentException("Unsupported URI scheme: " + scheme + ".");
    }

    StringBuilder sb = new StringBuilder();
    sb.append("hawk.");
    sb.append(HAWK_HEADER_VERSION);
    sb.append('.');
    sb.append(type);
    sb.append('\n');
    sb.append(timestamp);
    sb.append('\n');
    sb.append(nonce);
    sb.append('\n');
    sb.append(method);
    sb.append('\n');
    sb.append(path);
    sb.append('\n');
    sb.append(host);
    sb.append('\n');
    sb.append(port);
    sb.append('\n');
    if (hash != null) {
      sb.append(hash);
    }
    sb.append("\n");
    if (extra != null && extra.length() > 0) {
      sb.append(escapeExtraString(extra));
    }
    sb.append("\n");
    if (app != null) {
      sb.append(app);
      sb.append("\n");
      if (dlg != null) {
        sb.append(dlg);
      }
      sb.append("\n");
    }

    return sb.toString();
  }

  protected static byte[] hmacSha256(byte[] message, byte[] key)
      throws NoSuchAlgorithmException, InvalidKeyException {

    SecretKeySpec keySpec = new SecretKeySpec(key, HMAC_SHA256_ALGORITHM);

    Mac hasher = Mac.getInstance(HMAC_SHA256_ALGORITHM);
    hasher.init(keySpec);
    hasher.update(message);

    return hasher.doFinal();
  }

  









  protected static String getSignature(byte[] requestString, byte[] key)
      throws InvalidKeyException, NoSuchAlgorithmException, UnsupportedEncodingException {
    return Base64.encodeBase64String(hmacSha256(requestString, key));
  }
}
