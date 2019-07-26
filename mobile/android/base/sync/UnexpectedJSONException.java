



package org.mozilla.gecko.sync;

public class UnexpectedJSONException extends Exception {
  private static final long serialVersionUID = 4797570033096443169L;

  public UnexpectedJSONException(String detailMessage) {
    super(detailMessage);
  }

  public static class BadRequiredFieldJSONException extends UnexpectedJSONException {
    private static final long serialVersionUID = -9207736984784497612L;

    public BadRequiredFieldJSONException(String string) {
      super(string);
    }
  }
}
