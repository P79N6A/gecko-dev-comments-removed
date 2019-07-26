



package org.mozilla.gecko.fxa.authenticator;

















public interface FxAccountLoginDelegate {
  public void handleError(FxAccountLoginException e);
  public void handleSuccess(String assertion);
}
