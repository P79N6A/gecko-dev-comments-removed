



package org.mozilla.gecko.background.fxa;

public class FxAccountClientException extends Exception {
  private static final long serialVersionUID = 7953459541558266597L;

  public FxAccountClientException(String detailMessage) {
    super(detailMessage);
  }

  public FxAccountClientException(Exception e) {
    super(e);
  }
}