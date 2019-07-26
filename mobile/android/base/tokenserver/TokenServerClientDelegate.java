



package org.mozilla.gecko.tokenserver;


public interface TokenServerClientDelegate {
  void handleSuccess(TokenServerToken token);
  void handleFailure(TokenServerException e);
  void handleError(Exception e);

  


  void handleBackoff(int backoffSeconds);

  public String getUserAgent();
}
