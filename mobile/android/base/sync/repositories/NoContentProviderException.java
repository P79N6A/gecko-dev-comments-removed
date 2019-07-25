



package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.SyncException;

import android.net.Uri;







public class NoContentProviderException extends SyncException {
  private static final long serialVersionUID = 1L;

  public final Uri requestedProvider;
  public NoContentProviderException(Uri requested) {
    super();
    this.requestedProvider = requested;
  }
}
