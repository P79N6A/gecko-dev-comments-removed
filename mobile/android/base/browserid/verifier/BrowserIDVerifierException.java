



package org.mozilla.gecko.browserid.verifier;

public class BrowserIDVerifierException extends Exception {
  private static final long serialVersionUID = 2228946910754889975L;

  public BrowserIDVerifierException(String detailMessage) {
    super(detailMessage);
  }

  public BrowserIDVerifierException(Throwable throwable) {
    super(throwable);
  }

  public static class BrowserIDVerifierMalformedResponseException extends BrowserIDVerifierException {
    private static final long serialVersionUID = 115377527009652839L;

    public BrowserIDVerifierMalformedResponseException(String detailMessage) {
      super(detailMessage);
    }

    public BrowserIDVerifierMalformedResponseException(Throwable throwable) {
      super(throwable);
    }
  }

  public static class BrowserIDVerifierErrorResponseException extends BrowserIDVerifierException {
    private static final long serialVersionUID = 115377527009652840L;

    public BrowserIDVerifierErrorResponseException(String detailMessage) {
      super(detailMessage);
    }

    public BrowserIDVerifierErrorResponseException(Throwable throwable) {
      super(throwable);
    }
  }
}
