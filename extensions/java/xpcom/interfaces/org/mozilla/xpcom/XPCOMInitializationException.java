



































package org.mozilla.xpcom;


public class XPCOMInitializationException extends RuntimeException {

  private static final long serialVersionUID = -7067350325909231055L;

  public XPCOMInitializationException(String message) {
    super(message);
  }

  public XPCOMInitializationException(Throwable cause) {
    super(cause);
  }

  public XPCOMInitializationException(String message, Throwable cause) {
    super(message, cause);
  }

}
