



package org.mozilla.gecko.background.healthreport.prune;




public interface PrunePolicyStorage {
  public void pruneEvents(final int count);
  public void pruneEnvironments(final int count);

  public int deleteDataBefore(final long time);

  public void cleanup();

  public int getEventCount();
  public int getEnvironmentCount();

  



  public void close();
}
