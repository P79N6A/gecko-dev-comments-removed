



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;

import ch.boye.httpclientandroidlib.HttpResponse;




public abstract class ReadingListResponse extends MozResponse {
  static interface ResponseFactory<T extends ReadingListResponse> {
    public T getResponse(HttpResponse r);
  }

  public ReadingListResponse(HttpResponse res) {
    super(res);
  }

  public long getLastModified() {
    return getLongHeader("Last-Modified");
  }
}