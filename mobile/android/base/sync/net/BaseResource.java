



package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLContext;

import org.mozilla.gecko.sync.Logger;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.auth.UsernamePasswordCredentials;
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
import ch.boye.httpclientandroidlib.impl.auth.BasicScheme;
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

  private static final long MAX_IDLE_TIME_SECONDS = 30;

  public static boolean rewriteLocalhost = true;

  private static final String LOG_TAG = "BaseResource";

  protected URI uri;
  protected BasicHttpContext context;
  protected DefaultHttpClient client;
  public    ResourceDelegate delegate;
  protected HttpRequestBase request;
  public String charset = "utf-8";

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

  public URI getURI() {
    return this.uri;
  }

  



  private static void addAuthCacheToContext(HttpUriRequest request, HttpContext context) {
    AuthCache authCache = new BasicAuthCache();                
    context.setAttribute(ClientContext.AUTH_CACHE, authCache);
  }

  



  private static void applyCredentials(String credentials, HttpUriRequest request, HttpContext context) {
    addAuthCacheToContext(request, context);

    Credentials creds = new UsernamePasswordCredentials(credentials);
    Header header = BasicScheme.authenticate(creds, "US-ASCII", false);
    request.addHeader(header);
    Logger.trace(LOG_TAG, "Adding Basic Auth header.");
  }

  




  private void prepareClient() throws KeyManagementException, NoSuchAlgorithmException {
    context = new BasicHttpContext();

    
    
    client = new DefaultHttpClient(getConnectionManager());

    
    
    String credentials = delegate.getCredentials();
    if (credentials != null) {
      BaseResource.applyCredentials(credentials, request, context);
    }

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

  


  public static ClientConnectionManager enablePlainHTTPConnectionManager() {
    synchronized (connManagerMonitor) {
      ThreadSafeClientConnManager cm = new ThreadSafeClientConnManager();
      connManager = cm;
      return cm;
    }
  }

  
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

    Logger.trace(LOG_TAG, "Closing idle connections.");
    connectionManager.closeIdleConnections(MAX_IDLE_TIME_SECONDS, TimeUnit.SECONDS);
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
    try {
      HttpResponse response = client.execute(request, context);
      Logger.debug(LOG_TAG, "Response: " + response.getStatusLine().toString());
      delegate.handleHttpResponse(response);
    } catch (ClientProtocolException e) {
      delegate.handleHttpProtocolException(e);
    } catch (IOException e) {
      delegate.handleHttpIOException(e);
    }
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
    } catch (NoSuchAlgorithmException e) {
      Logger.error(LOG_TAG, "Couldn't prepare client.", e);
      delegate.handleTransportException(e);
    }
    this.execute();
  }

  @Override
  public void get() {
    Logger.debug(LOG_TAG, "HTTP GET " + this.uri.toASCIIString());
    this.go(new HttpGet(this.uri));
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
}
