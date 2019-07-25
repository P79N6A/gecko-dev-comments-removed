




































package org.mozilla.gecko.sync.net;

import ch.boye.httpclientandroidlib.HttpEntity;

public interface Resource {
  public abstract void get();
  public abstract void delete();
  public abstract void post(HttpEntity body);
  public abstract void put(HttpEntity body);
}
