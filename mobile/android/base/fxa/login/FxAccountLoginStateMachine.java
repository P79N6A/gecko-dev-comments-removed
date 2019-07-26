



package org.mozilla.gecko.fxa.login;

import java.security.NoSuchAlgorithmException;
import java.util.HashSet;
import java.util.Set;

import org.mozilla.gecko.background.fxa.FxAccountClient;
import org.mozilla.gecko.browserid.BrowserIDKeyPair;
import org.mozilla.gecko.fxa.login.FxAccountLoginTransition.Transition;
import org.mozilla.gecko.fxa.login.State.StateLabel;

public class FxAccountLoginStateMachine {
  public static final String LOG_TAG = FxAccountLoginStateMachine.class.getSimpleName();

  public interface LoginStateMachineDelegate {
    public FxAccountClient getClient();
    public long getCertificateDurationInMilliseconds();
    public long getAssertionDurationInMilliseconds();
    public void handleTransition(Transition transition, State state);
    public void handleFinal(State state);
    public BrowserIDKeyPair generateKeyPair() throws NoSuchAlgorithmException;
  }

  public static class ExecuteDelegate {
    protected final LoginStateMachineDelegate delegate;
    protected final StateLabel desiredStateLabel;
    
    protected final Set<StateLabel> stateLabelsSeen = new HashSet<StateLabel>();

    protected ExecuteDelegate(StateLabel initialStateLabel, StateLabel desiredStateLabel, LoginStateMachineDelegate delegate) {
      this.delegate = delegate;
      this.desiredStateLabel = desiredStateLabel;
      this.stateLabelsSeen.add(initialStateLabel);
    }

    public FxAccountClient getClient() {
      return delegate.getClient();
    }

    public long getCertificateDurationInMilliseconds() {
      return delegate.getCertificateDurationInMilliseconds();
    }

    public long getAssertionDurationInMilliseconds() {
      return delegate.getAssertionDurationInMilliseconds();
    }

    public BrowserIDKeyPair generateKeyPair() throws NoSuchAlgorithmException {
      return delegate.generateKeyPair();
    }

    public void handleTransition(Transition transition, State state) {
      
      delegate.handleTransition(transition, state);

      
      
      StateLabel stateLabel = state.getStateLabel();
      if (stateLabel == desiredStateLabel || stateLabelsSeen.contains(stateLabel)) {
        delegate.handleFinal(state);
        return;
      }

      
      
      stateLabelsSeen.add(stateLabel);
      state.execute(this);
    }
  }

  public void advance(State initialState, final StateLabel desiredStateLabel, final LoginStateMachineDelegate delegate) {
    if (initialState.getStateLabel() == desiredStateLabel) {
      
      delegate.handleFinal(initialState);
      return;
    }
    ExecuteDelegate executeDelegate = new ExecuteDelegate(initialState.getStateLabel(), desiredStateLabel, delegate);
    initialState.execute(executeDelegate);
  }
}
