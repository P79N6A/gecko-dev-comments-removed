



package org.mozilla.gecko.background.bagheera;

import ch.boye.httpclientandroidlib.HttpResponse;

public interface BagheeraRequestDelegate {
  void handleSuccess(int status, String namespace, String id, HttpResponse response);
  void handleError(Exception e);
  void handleFailure(int status, String namespace, HttpResponse response);

  public String getUserAgent();
}
