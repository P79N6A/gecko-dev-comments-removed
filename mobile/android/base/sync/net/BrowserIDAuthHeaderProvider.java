



package org.mozilla.gecko.sync.net;








public class BrowserIDAuthHeaderProvider extends AbstractBearerTokenAuthHeaderProvider {
  public BrowserIDAuthHeaderProvider(String assertion) {
    super(assertion);
  }

  @Override
  protected String getPrefix() {
    return "BrowserID";
  }
}
