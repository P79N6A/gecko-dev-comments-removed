



package org.mozilla.gecko.fxa.login;

import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine.ExecuteDelegate;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.PasswordRequired;

public class MigratedFromSync11 extends State {
  public final String password;

  public MigratedFromSync11(String email, String uid, boolean verified, String password) {
    super(StateLabel.MigratedFromSync11, email, uid, verified);
    
    this.password = password;
  }

  @Override
  public void execute(final ExecuteDelegate delegate) {
    delegate.handleTransition(new PasswordRequired(), this);
  }

  @Override
  public Action getNeededAction() {
    return Action.NeedsFinishMigrating;
  }
}
