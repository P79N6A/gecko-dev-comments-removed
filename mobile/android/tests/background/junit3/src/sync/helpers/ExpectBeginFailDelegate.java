


package org.mozilla.gecko.background.sync.helpers;

import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;

public class ExpectBeginFailDelegate extends DefaultBeginDelegate {

  @Override
  public void onBeginFailed(Exception ex) {
    if (!(ex instanceof InvalidSessionTransitionException)) {
      performNotify("Expected InvalidSessionTransititionException but got ", ex);
    }
  }
}
