



package org.mozilla.gecko.fxa.sync;

import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.BackoffHandler;

public interface SchedulePolicy {
  


  public abstract void onSuccessfulSync(int otherClientsCount);
  public abstract void onHandleFinal(Action needed);
  public abstract void onUpgradeRequired();
  public abstract void onUnauthorized();

  












  public abstract void configureBackoffMillisBeforeSyncing(BackoffHandler rateHandler, BackoffHandler backgroundHandler);

  







  public abstract void configureBackoffMillisOnBackoff(BackoffHandler backoffHandler, long backoffMillis, boolean onlyExtend);
}