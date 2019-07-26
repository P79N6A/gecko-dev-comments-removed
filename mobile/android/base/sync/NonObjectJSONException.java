



package org.mozilla.gecko.sync;

public class NonObjectJSONException extends UnexpectedJSONException {
  private static final long serialVersionUID = 2214238763035650087L;

  public NonObjectJSONException(String detailMessage) {
    super(detailMessage);
  }
}
