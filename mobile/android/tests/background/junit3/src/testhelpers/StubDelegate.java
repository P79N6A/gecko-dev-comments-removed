


package org.mozilla.gecko.background.testhelpers;

import org.mozilla.gecko.background.healthreport.upload.SubmissionClient.Delegate;

public class StubDelegate implements Delegate {
  @Override
  public void onSoftFailure(long localTime, String id, String reason, Exception e) { }
  @Override
  public void onHardFailure(long localTime, String id, String reason, Exception e) { }
  @Override
  public void onSuccess(long localTime, String id) { }
}
