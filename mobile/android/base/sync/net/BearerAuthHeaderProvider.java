



package org.mozilla.gecko.sync.net;







public class BearerAuthHeaderProvider extends AbstractBearerTokenAuthHeaderProvider {
  public BearerAuthHeaderProvider(String token) {
    super(token);
  }

  @Override
  protected String getPrefix() {
    return "Bearer";
  }
}
