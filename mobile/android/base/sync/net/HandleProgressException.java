




































package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.sync.SyncException;

public class HandleProgressException extends SyncException {
  private static final long serialVersionUID = -4444933937013161059L;

  public HandleProgressException(Exception ex) {
    super(ex);
  }
}
