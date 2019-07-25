




































package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.SyncException;

public class InactiveSessionException extends SyncException {

  private static final long serialVersionUID = 537241160815940991L;

  public InactiveSessionException(Exception ex) {
    super(ex);
  }

}
