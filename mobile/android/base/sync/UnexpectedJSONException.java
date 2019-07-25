




































package org.mozilla.gecko.sync;

public class UnexpectedJSONException extends Exception {
  private static final long serialVersionUID = 4797570033096443169L;

  public Object obj;
  public UnexpectedJSONException(Object object) {
    obj = object;
  }
}
