



package org.mozilla.gecko.sync.net;

import java.net.URI;

import ch.boye.httpclientandroidlib.HttpEntity;

public interface Resource {
  public abstract URI getURI();
  public abstract String getURIString();
  public abstract String getHostname();
  public abstract void get();
  public abstract void delete();
  public abstract void post(HttpEntity body);
  public abstract void patch(HttpEntity body);
  public abstract void put(HttpEntity body);
}
