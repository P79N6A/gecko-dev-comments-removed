



package org.mozilla.gecko.sync;

public class NonArrayJSONException extends UnexpectedJSONException {
  private static final long serialVersionUID = 5582918057432365749L;

  public NonArrayJSONException(String detailMessage) {
    super(detailMessage);
  }
}
