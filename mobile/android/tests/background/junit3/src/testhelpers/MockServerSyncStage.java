


package org.mozilla.gecko.background.testhelpers;


public class MockServerSyncStage extends BaseMockServerSyncStage {
  @Override
  public void execute() {
    session.advance();
  }
}
