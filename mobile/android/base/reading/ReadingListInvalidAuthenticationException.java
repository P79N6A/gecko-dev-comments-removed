



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;

public class ReadingListInvalidAuthenticationException extends Exception {
  private static final long serialVersionUID = 7112459541558266597L;

  public final MozResponse response;

  public ReadingListInvalidAuthenticationException(MozResponse response) {
    super();
    this.response = response;
  }
}
