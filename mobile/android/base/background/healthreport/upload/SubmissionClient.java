



package org.mozilla.gecko.background.healthreport.upload;

import java.util.Collection;

public interface SubmissionClient {
  public interface Delegate {
    







    public void onSoftFailure(long localTime, String id, String reason, Exception e);

    







    public void onHardFailure(long localTime, String id, String reason, Exception e);

    





    public void onSuccess(long localTime, String id);
  }

  public void upload(long localTime, String id, Collection<String> oldIds, Delegate delegate);
  public void delete(long localTime, String id, Delegate delegate);
}
