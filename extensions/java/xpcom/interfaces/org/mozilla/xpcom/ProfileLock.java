



































package org.mozilla.xpcom;

public class ProfileLock {

  private long lock = 0;

  public ProfileLock(long aLockObject) {
    lock = aLockObject;
  }

  public void release() {
    releaseNative(lock);
    lock = 0;
  }

  private native void releaseNative(long aLockObject);

  public boolean isValid() {
    return lock != 0;
  }

  protected void finalize() throws Throwable {
    release();
    super.finalize();
  }

}
