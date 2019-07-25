




































package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.CryptoRecord;






public abstract class WBOCollectionRequestDelegate extends
    SyncStorageCollectionRequestDelegate {

  public abstract void handleWBO(CryptoRecord record);
  public abstract KeyBundle keyBundle();

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
