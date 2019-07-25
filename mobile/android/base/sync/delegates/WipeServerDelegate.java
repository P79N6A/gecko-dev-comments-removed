




































package org.mozilla.gecko.sync.delegates;

public interface WipeServerDelegate {
  public void onWiped(long timestamp);
  public void onWipeFailed(Exception e);
}
