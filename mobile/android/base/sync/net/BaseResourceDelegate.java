



package org.mozilla.gecko.sync.net;

import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;








public abstract class BaseResourceDelegate implements ResourceDelegate {
  public static int connectionTimeoutInMillis = 1000 * 30;     
  public static int socketTimeoutInMillis     = 1000 * 2 * 60; 

  protected Resource resource;
  public BaseResourceDelegate(Resource resource) {
    this.resource = resource;
  }

  @Override
  public int connectionTimeout() {
    return connectionTimeoutInMillis;
  }

  @Override
  public int socketTimeout() {
    return socketTimeoutInMillis;
  }

  @Override
  public AuthHeaderProvider getAuthHeaderProvider() {
    return null;
  }

  @Override
  public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
  }
}
