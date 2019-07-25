



package org.mozilla.gecko.sync.synchronizer;







public class ServerLocalSynchronizer extends Synchronizer {
  public SynchronizerSession getSynchronizerSession() {
    return new ServerLocalSynchronizerSession(this, this);
  }
}
