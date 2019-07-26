


package org.mozilla.gecko.background.testhelpers;

import org.mozilla.gecko.sync.stage.AbstractNonRepositorySyncStage;

public class MockAbstractNonRepositorySyncStage extends AbstractNonRepositorySyncStage {
  @Override
  public void execute() {
    session.advance();
  }
}
