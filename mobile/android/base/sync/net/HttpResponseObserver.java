



package org.mozilla.gecko.sync.net;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;

public interface HttpResponseObserver {
  







  public void observeHttpResponse(HttpUriRequest request, HttpResponse response);
}
