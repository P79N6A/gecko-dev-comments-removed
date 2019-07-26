



package org.mozilla.gecko.sync.net;

import java.io.IOException;
import java.security.GeneralSecurityException;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;










public interface ResourceDelegate {
  
  AuthHeaderProvider getAuthHeaderProvider();
  void addHeaders(HttpRequestBase request, DefaultHttpClient client);

  

  






  void handleHttpResponse(HttpResponse response);
  void handleHttpProtocolException(ClientProtocolException e);
  void handleHttpIOException(IOException e);

  
  void handleTransportException(GeneralSecurityException e);

  
  int connectionTimeout();
  int socketTimeout();
}
