



package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.KeyBundleProvider;






public abstract class WBOCollectionRequestDelegate
extends SyncStorageCollectionRequestDelegate
implements KeyBundleProvider {

  @Override
  public abstract KeyBundle keyBundle();
  public abstract void handleWBO(CryptoRecord record);

  @Override
  public void handleRequestProgress(String progress) {
    try {
      CryptoRecord record = CryptoRecord.fromJSONRecord(progress);
      record.keyBundle = this.keyBundle();
      this.handleWBO(record);
    } catch (Exception e) {
      this.handleRequestError(e);
      
    }
  }
}
