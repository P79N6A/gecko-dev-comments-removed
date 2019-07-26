



package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;

import javax.net.ssl.SSLContext;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.client.AuthCache;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpDelete;
import ch.boye.httpclientandroidlib.client.methods.HttpGet;
import ch.boye.httpclientandroidlib.client.methods.HttpPost;
import ch.boye.httpclientandroidlib.client.methods.HttpPut;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.client.protocol.ClientContext;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.scheme.PlainSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.conn.ssl.SSLSocketFactory;
import ch.boye.httpclientandroidlib.entity.StringEntity;
import ch.boye.httpclientandroidlib.impl.client.BasicAuthCache;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.impl.conn.tsccm.ThreadSafeClientConnManager;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.EntityUtils;







public class BaseResource implements Resource {
  private static final String ANDROID_LOOPBACK_IP = "10.0.2.2";

  private static final int MAX_TOTAL_CONNECTIONS     = 20;
  private static final int MAX_CONNECTIONS_PER_ROUTE = 10;

  private boolean retryOnFailedRequest = true;

  public static boolean rewriteLocalhost = true;

  private static final String LOG_TAG = "BaseResource";

  protected URI uri;
  protected BasicHttpContext context;
  protected DefaultHttpClient client;
  public    ResourceDelegate delegate;
  protected HttpRequestBase request;
  public String charset = "utf-8";

  protected static WeakReference<HttpResponseObserver> httpResponseObserver = null;

  public BaseResource(String uri) throws URISyntaxException {
    this(uri, rewriteLocalhost);
  }

  public BaseResource(URI uri) {
    this(uri, rewriteLocalhost);
  }

  public BaseResource(String uri, boolean rewrite) throws URISyntaxException {
    this(new URI(uri), rewrite);
  }

  public BaseResource(URI uri, boolean rewrite) {
    if (rewrite && uri.getHost().equals("localhost")) {
      
      Logger.debug(LOG_TAG, "Rewriting " + uri + " to point to " + ANDROID_LOOPBACK_IP + ".");
      try {
        this.uri = new URI(uri.getScheme(), uri.getUserInfo(), ANDROID_LOOPBACK_IP, uri.getPort(), uri.getPath(), uri.getQuery(), uri.getFragment());
      } catch (URISyntaxException e) {
        Logger.error(LOG_TAG, "Got error rewriting URI for Android emulator.", e);
      }
    } else {
      this.uri = uri;
    }
  }

  public static synchronized HttpResponseObserver getHttpResponseObserver() {
    if (httpResponseObserver == null) {
      return null;
    }
    return httpResponseObserver.get();
  }

  public static synchronized void setHttpResponseObserver(HttpResponseObserver newHttpResponseObserver) {
    if (httpResponseObserver != null) {
      httpResponseObserver.clear();
    }
    httpResponseObserver = new WeakReference<HttpResponseObserver>(newHttpResponseObserver);
  }

  public URI getURI() {
    return this.uri;
  }

  



  private static void addAuthCacheToContext(HttpUriRequest request, HttpContext context) {
    AuthCache authCache = new BasicAuthCache();                
    context.setAttribute(ClientContext.AUTH_CACHE, authCache);
  }

  




  protected void prepareClient() throws KeyManagementException, NoSuchAlgorithmException, GeneralSecurityException {
    context = new BasicHttpContext();

    
    
    client = new DefaultHttpClient(getConnectionManager());

    
    
    AuthHeaderProvider authHeaderProvider = delegate.getAuthHeaderProvider();
    if (authHeaderProvider != null) {
      Header authHeader = authHeaderProvider.getAuthHeader(request, context, client);
      if (authHeader != null) {
        request.addHeader(authHeader);
        Logger.debug(LOG_TAG, "Added auth header.");
      }
    }

    addAuthCacheToContext(request, context);

    HttpParams params = client.getParams();
    HttpConnectionParams.setConnectionTimeout(params, delegate.connectionTimeout());
    HttpConnectionParams.setSoTimeout(params, delegate.socketTimeout());
    HttpConnectionParams.setStaleCheckingEnabled(params, false);
    HttpProtocolParams.setContentCharset(params, charset);
    HttpProtocolParams.setVersion(params, HttpVersion.HTTP_1_1);
    delegate.addHeaders(request, client);
  }

  private static Object connManagerMonitor = new Object();
  private static ClientConnectionManager connManager;

  
  private static ClientConnectionManager enableTLSConnectionManager() throws KeyManagementException, NoSuchAlgorithmException  {
    SSLContext sslContext = SSLContext.getInstance("TLS");
    sslContext.init(null, null, new SecureRandom());
    SSLSocketFactory sf = new TLSSocketFactory(sslContext);
    SchemeRegistry schemeRegistry = new SchemeRegistry();
    schemeRegistry.register(new Scheme("https", 443, sf));
    schemeRegistry.register(new Scheme("http", 80, new PlainSocketFactory()));
    ThreadSafeClientConnManager cm = new ThreadSafeClientConnManager(schemeRegistry);

    cm.setMaxTotal(MAX_TOTAL_CONNECTIONS);
    cm.setDefaultMaxPerRoute(MAX_CONNECTIONS_PER_ROUTE);
    connManager = cm;
    return cm;
  }

  public static ClientConnectionManager getConnectionManager() throws KeyManagementException, NoSuchAlgorithmException
                                                         {
    
    synchronized (connManagerMonitor) {
      if (connManager != null) {
        return connManager;
      }
      return enableTLSConnectionManager();
    }
  }

  


  public static void closeExpiredConnections() {
    ClientConnectionManager connectionManager;
    synchronized (connManagerMonitor) {
      connectionManager = connManager;
    }
    if (connectionManager == null) {
      return;
    }
    Logger.trace(LOG_TAG, "Closing expired connections.");
    connectionManager.closeExpiredConnections();
  }

  public static void shutdownConnectionManager() {
    ClientConnectionManager connectionManager;
    synchronized (connManagerMonitor) {
      connectionManager = connManager;
      connManager = null;
    }
    if (connectionManager == null) {
      return;
    }
    Logger.debug(LOG_TAG, "Shutting down connection manager.");
    connectionManager.shutdown();
  }

  private void execute() {
    HttpResponse response;
    try {
      response = client.execute(request, context);
      Logger.debug(LOG_TAG, "Response: " + response.getStatusLine().toString());
    } catch (ClientProtocolException e) {
      delegate.handleHttpProtocolException(e);
      return;
    } catch (IOException e) {
      Logger.debug(LOG_TAG, "I/O exception returned from execute.");
      if (!retryOnFailedRequest) {
        delegate.handleHttpIOException(e);
      } else {
        retryRequest();
      }
      return;
    } catch (Exception e) {
      
      
      if (!retryOnFailedRequest) {
        
        final IOException ex = new IOException();
        ex.initCause(e);
        delegate.handleHttpIOException(ex);
      } else {
        retryRequest();
      }
      return;
    }

    
    HttpResponseObserver observer = getHttpResponseObserver();
    if (observer != null) {
      observer.observeHttpResponse(response);
    }
    delegate.handleHttpResponse(response);
  }

  private void retryRequest() {
    
    retryOnFailedRequest = false;
    Logger.debug(LOG_TAG, "Retrying request...");
    this.execute();
  }

  private void go(HttpRequestBase request) {
   if (delegate == null) {
      throw new IllegalArgumentException("No delegate provided.");
    }
    this.request = request;
    try {
      this.prepareClient();
    } catch (KeyManagementException e) {
      Logger.error(LOG_TAG, "Couldn't prepare client.", e);
      delegate.handleTransportException(e);
      return;
    } catch (NoSuchAlgorithmException e) {
      Logger.error(LOG_TAG, "Couldn't prepare client.", e);
      delegate.handleTransportException(e);
      return;
    } catch (GeneralSecurityException e) {
      Logger.error(LOG_TAG, "Couldn't prepare client.", e);
      delegate.handleTransportException(e);
      return;
    } catch (Exception e) {
      
      
      delegate.handleTransportException(new GeneralSecurityException(e));
      return;
    }
    this.execute();
  }

  @Override
  public void get() {
    Logger.debug(LOG_TAG, "HTTP GET " + this.uri.toASCIIString());
    this.go(new HttpGet(this.uri));
  }

  



  public void getBlocking() {
    
    
    this.get();
  }

  @Override
  public void delete() {
    Logger.debug(LOG_TAG, "HTTP DELETE " + this.uri.toASCIIString());
    this.go(new HttpDelete(this.uri));
  }

  @Override
  public void post(HttpEntity body) {
    Logger.debug(LOG_TAG, "HTTP POST " + this.uri.toASCIIString());
    HttpPost request = new HttpPost(this.uri);
    request.setEntity(body);
    this.go(request);
  }

  @Override
  public void put(HttpEntity body) {
    Logger.debug(LOG_TAG, "HTTP PUT " + this.uri.toASCIIString());
    HttpPut request = new HttpPut(this.uri);
    request.setEntity(body);
    this.go(request);
  }

  protected static StringEntity stringEntityWithContentTypeApplicationJSON(String s) throws UnsupportedEncodingException {
    StringEntity e = new StringEntity(s, "UTF-8");
    e.setContentType("application/json");
    return e;
  }

  



  protected static StringEntity jsonEntity(JSONObject body) throws UnsupportedEncodingException {
    return stringEntityWithContentTypeApplicationJSON(body.toJSONString());
  }

  



  protected static StringEntity jsonEntity(ExtendedJSONObject body) throws UnsupportedEncodingException {
    return stringEntityWithContentTypeApplicationJSON(body.toJSONString());
  }

  



  protected static HttpEntity jsonEntity(JSONArray toPOST) throws UnsupportedEncodingException {
    return stringEntityWithContentTypeApplicationJSON(toPOST.toJSONString());
  }

  







  public static void consumeEntity(HttpEntity entity) {
    try {
      EntityUtils.consume(entity);
    } catch (IOException e) {
      
    }
  }

  









  public static void consumeEntity(HttpResponse response) {
    if (response == null) {
      return;
    }
    try {
      EntityUtils.consume(response.getEntity());
    } catch (IOException e) {
    }
  }

  









  public static void consumeEntity(SyncStorageResponse response) {
    if (response.httpResponse() == null) {
      return;
    }
    consumeEntity(response.httpResponse());
  }

  







  public static void consumeReader(BufferedReader reader) {
    try {
      reader.close();
    } catch (IOException e) {
      
    }
  }

  public void post(JSONArray jsonArray) throws UnsupportedEncodingException {
    post(jsonEntity(jsonArray));
  }

  public void put(JSONObject jsonObject) throws UnsupportedEncodingException {
    put(jsonEntity(jsonObject));
  }

  public void post(ExtendedJSONObject o) throws UnsupportedEncodingException {
    post(jsonEntity(o));
  }
}
