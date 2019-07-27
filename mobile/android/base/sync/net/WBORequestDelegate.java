



package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.sync.KeyBundleProvider;
import org.mozilla.gecko.sync.crypto.KeyBundle;

public abstract class WBORequestDelegate
implements SyncStorageRequestDelegate, KeyBundleProvider {
  @Override
  public abstract KeyBundle keyBundle();
}
