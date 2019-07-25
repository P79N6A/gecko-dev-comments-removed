




































package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestBase;
import ch.boye.httpclientandroidlib.impl.client.DefaultHttpClient;
import ch.boye.httpclientandroidlib.util.EntityUtils;








public abstract class SyncResourceDelegate implements ResourceDelegate {

  protected Resource resource;
  public SyncResourceDelegate(Resource resource) {
    this.resource = resource;
  }

  @Override
  public int connectionTimeout() {
    return 30 * 1000;             
  }
  @Override
  public int socketTimeout() {
    return 5 * 60 * 1000;         
  }

  @Override
  public String getCredentials() {
    return null;
  }

  @Override
  public void addHeaders(HttpRequestBase request, DefaultHttpClient client) {
  }

  


  public static void consumeEntity(HttpEntity entity) {
    try {
      EntityUtils.consume(entity);
    } catch (Exception e) {
      
    }
  }

  public static void consumeReader(BufferedReader reader) {
    try {
      while ((reader.readLine()) != null) {
      }
    } catch (IOException e) {
      return;
    }
  }
}