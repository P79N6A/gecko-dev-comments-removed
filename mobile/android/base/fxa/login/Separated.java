



package org.mozilla.gecko.fxa.login;

import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine.ExecuteDelegate;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.PasswordRequired;


public class Separated extends State {
  public Separated(String email, String uid, boolean verified) {
    super(StateLabel.Separated, email, uid, verified);
  }

  @Override
  public void execute(final ExecuteDelegate delegate) {
    delegate.handleTransition(new PasswordRequired(), this);
  }

  @Override
  public Action getNeededAction() {
    return Action.NeedsPassword;
  }
}
