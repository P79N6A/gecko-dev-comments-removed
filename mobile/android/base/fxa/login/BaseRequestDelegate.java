



package org.mozilla.gecko.fxa.login;

import org.mozilla.gecko.background.fxa.FxAccountClient10;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.fxa.login.FxAccountLoginStateMachine.ExecuteDelegate;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.AccountNeedsVerification;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.LocalError;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.RemoteError;

public abstract class BaseRequestDelegate<T> implements FxAccountClient10.RequestDelegate<T> {
  protected final ExecuteDelegate delegate;
  protected final State state;

  public BaseRequestDelegate(State state, ExecuteDelegate delegate) {
    this.delegate = delegate;
    this.state = state;
  }

  @Override
  public void handleFailure(FxAccountClientRemoteException e) {
    
    
    
    
    
    if (e.isUpgradeRequired()) {
      delegate.handleTransition(new RemoteError(e), new Separated(state.email, state.uid, state.verified));
      return;
    }
    if (e.isInvalidAuthentication()) {
      delegate.handleTransition(new RemoteError(e), new Separated(state.email, state.uid, state.verified));
      return;
    }
    if (e.isUnverified()) {
      delegate.handleTransition(new AccountNeedsVerification(), state);
      return;
    }
    delegate.handleTransition(new RemoteError(e), state);
  }

  @Override
  public void handleError(Exception e) {
    delegate.handleTransition(new LocalError(e), state);
  }
}
