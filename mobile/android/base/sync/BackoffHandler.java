



package org.mozilla.gecko.sync;


public interface BackoffHandler {
  public long getEarliestNextRequest();

  






  public void setEarliestNextRequest(long next);

  






  public void extendEarliestNextRequest(long next);

  



  public long delayMilliseconds();
}