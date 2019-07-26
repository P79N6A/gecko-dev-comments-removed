


package org.mozilla.gecko.background.sync.helpers;

public class ExpectStoreCompletedDelegate extends DefaultStoreDelegate {

  @Override
  public void onRecordStoreSucceeded(String guid) {
    
  }

  @Override
  public void onStoreCompleted(long storeEnd) {
    performNotify();
  }
}
