



package org.mozilla.gecko.fxa.login;

import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine.ExecuteDelegate;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.LogMessage;


public class Doghouse extends State {
  public Doghouse(String email, String uid, boolean verified) {
    super(StateLabel.Doghouse, email, uid, verified);
  }

  @Override
  public void execute(final ExecuteDelegate delegate) {
    delegate.handleTransition(new LogMessage("Upgraded Firefox clients might know what to do here."), this);
  }

  @Override
  public Action getNeededAction() {
    return Action.NeedsUpgrade;
  }
}
