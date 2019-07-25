



package org.mozilla.gecko.sync.synchronizer;







public class ServerLocalSynchronizer extends Synchronizer {
  @Override
  public SynchronizerSession newSynchronizerSession() {
    return new ServerLocalSynchronizerSession(this, this);
  }
}
