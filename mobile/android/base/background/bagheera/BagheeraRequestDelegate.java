



package org.mozilla.gecko.background.bagheera;

import ch.boye.httpclientandroidlib.HttpResponse;

public interface BagheeraRequestDelegate {
  void handleSuccess(int status, HttpResponse response);
  void handleError(Exception e);
  void handleFailure(int status, HttpResponse response);
}
